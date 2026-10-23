#!/usr/bin/env python3
"""Measure visual parity between a retail F.E.A.R. frame and a port frame.

Implements the metrics defined in docs/FEAR_ACCEPTANCE.md:

  primary    percentage of pixels whose every RGB channel is within N/255
  secondary  mean absolute error, luma SSIM, mean CIEDE2000

Usage:
  python tools/parity_check.py reference.png candidate.png
  python tools/parity_check.py ref.png cand.png --tolerance 8 --diff diff.png
  python tools/parity_check.py --selftest
"""

import argparse
import json
import math
import sys

import numpy as np
from PIL import Image

# D65 reference white used by the sRGB to CIELAB conversion.
_WHITE = np.array([0.95047, 1.00000, 1.08883], dtype=np.float64)


def load_rgb(path):
    image = Image.open(path)
    if image.mode in ("RGBA", "LA", "P"):
        image = image.convert("RGBA")
        background = Image.new("RGBA", image.size, (0, 0, 0, 255))
        image = Image.alpha_composite(background, image)
    return np.asarray(image.convert("RGB"), dtype=np.float64)


def align(reference, candidate):
    """Crop both frames to the overlapping area, matching on the largest common size."""
    height = min(reference.shape[0], candidate.shape[0])
    width = min(reference.shape[1], candidate.shape[1])
    return reference[:height, :width], candidate[:height, :width]


def channel_match_percent(reference, candidate, tolerance):
    delta = np.abs(reference - candidate)
    matched = np.all(delta <= tolerance, axis=2)
    return 100.0 * float(np.count_nonzero(matched)) / float(matched.size)


def mean_absolute_error(reference, candidate):
    return float(np.mean(np.abs(reference - candidate)))


def _box_sum(values, radius):
    """Sum over a (2*radius+1) square window using an integral image."""
    padded = np.pad(values, ((1, 0), (1, 0)), mode="constant")
    integral = padded.cumsum(axis=0).cumsum(axis=1)
    height, width = values.shape
    y0 = np.clip(np.arange(height) - radius, 0, height)
    y1 = np.clip(np.arange(height) + radius + 1, 0, height)
    x0 = np.clip(np.arange(width) - radius, 0, width)
    x1 = np.clip(np.arange(width) + radius + 1, 0, width)
    total = (
        integral[np.ix_(y1, x1)]
        - integral[np.ix_(y0, x1)]
        - integral[np.ix_(y1, x0)]
        + integral[np.ix_(y0, x0)]
    )
    count = np.outer(y1 - y0, x1 - x0).astype(np.float64)
    return total, count


def _box_mean(values, radius, count):
    total, _ = _box_sum(values, radius)
    return total / count


def luma(image):
    return 0.299 * image[:, :, 0] + 0.587 * image[:, :, 1] + 0.114 * image[:, :, 2]


def ssim(reference, candidate, radius=4, dynamic_range=255.0):
    """Mean structural similarity on luma, using an 8x8 sliding window."""
    x = luma(reference)
    y = luma(candidate)
    count = _box_sum(np.ones_like(x), radius)[1]
    mu_x = _box_mean(x, radius, count)
    mu_y = _box_mean(y, radius, count)
    sigma_x = _box_mean(x * x, radius, count) - mu_x * mu_x
    sigma_y = _box_mean(y * y, radius, count) - mu_y * mu_y
    sigma_xy = _box_mean(x * y, radius, count) - mu_x * mu_y
    c1 = (0.01 * dynamic_range) ** 2
    c2 = (0.03 * dynamic_range) ** 2
    numerator = (2.0 * mu_x * mu_y + c1) * (2.0 * sigma_xy + c2)
    denominator = (mu_x * mu_x + mu_y * mu_y + c1) * (sigma_x + sigma_y + c2)
    return float(np.mean(numerator / denominator))


def srgb_to_lab(image):
    srgb = image / 255.0
    linear = np.where(srgb <= 0.04045, srgb / 12.92, ((srgb + 0.055) / 1.055) ** 2.4)
    matrix = np.array(
        [
            [0.4124564, 0.3575761, 0.1804375],
            [0.2126729, 0.7151522, 0.0721750],
            [0.0193339, 0.1191920, 0.9503041],
        ],
        dtype=np.float64,
    )
    xyz = linear.reshape(-1, 3) @ matrix.T
    xyz = xyz / _WHITE
    epsilon = (6.0 / 29.0) ** 3
    kappa = 3.0 * (6.0 / 29.0) ** 2

    def forward(values):
        return np.where(values > epsilon, np.cbrt(values), values / kappa + 4.0 / 29.0)

    f_xyz = forward(xyz)
    lightness = 116.0 * f_xyz[:, 1] - 16.0
    a = 500.0 * (f_xyz[:, 0] - f_xyz[:, 1])
    b = 200.0 * (f_xyz[:, 1] - f_xyz[:, 2])
    return np.stack([lightness, a, b], axis=1)


def ciede2000(lab1, lab2):
    """Vectorised CIEDE2000 colour difference."""
    lightness_1, a1, b1 = lab1[:, 0], lab1[:, 1], lab1[:, 2]
    lightness_2, a2, b2 = lab2[:, 0], lab2[:, 1], lab2[:, 2]

    c1 = np.hypot(a1, b1)
    c2 = np.hypot(a2, b2)
    c_bar = 0.5 * (c1 + c2)
    c_bar7 = c_bar**7
    g = 0.5 * (1.0 - np.sqrt(c_bar7 / (c_bar7 + 25.0**7)))

    a1p = (1.0 + g) * a1
    a2p = (1.0 + g) * a2
    c1p = np.hypot(a1p, b1)
    c2p = np.hypot(a2p, b2)

    h1p = np.degrees(np.arctan2(b1, a1p)) % 360.0
    h2p = np.degrees(np.arctan2(b2, a2p)) % 360.0
    h1p = np.where((b1 == 0.0) & (a1p == 0.0), 0.0, h1p)
    h2p = np.where((b2 == 0.0) & (a2p == 0.0), 0.0, h2p)

    delta_lightness = lightness_2 - lightness_1
    delta_chroma = c2p - c1p

    chroma_product = c1p * c2p
    delta_hue_angle = h2p - h1p
    delta_hue_angle = np.where(delta_hue_angle > 180.0, delta_hue_angle - 360.0, delta_hue_angle)
    delta_hue_angle = np.where(delta_hue_angle < -180.0, delta_hue_angle + 360.0, delta_hue_angle)
    delta_hue_angle = np.where(chroma_product == 0.0, 0.0, delta_hue_angle)
    delta_hue = 2.0 * np.sqrt(chroma_product) * np.sin(np.radians(delta_hue_angle) / 2.0)

    lightness_bar = 0.5 * (lightness_1 + lightness_2)
    chroma_bar = 0.5 * (c1p + c2p)

    hue_bar = h1p + h2p
    hue_bar = np.where(
        chroma_product == 0.0,
        h1p + h2p,
        np.where(
            np.abs(h1p - h2p) <= 180.0,
            0.5 * (h1p + h2p),
            np.where(h1p + h2p < 360.0, 0.5 * (h1p + h2p + 360.0), 0.5 * (h1p + h2p - 360.0)),
        ),
    )

    t = (
        1.0
        - 0.17 * np.cos(np.radians(hue_bar - 30.0))
        + 0.24 * np.cos(np.radians(2.0 * hue_bar))
        + 0.32 * np.cos(np.radians(3.0 * hue_bar + 6.0))
        - 0.20 * np.cos(np.radians(4.0 * hue_bar - 63.0))
    )
    delta_theta = 30.0 * np.exp(-(((hue_bar - 275.0) / 25.0) ** 2))
    chroma_bar7 = chroma_bar**7
    rotation = 2.0 * np.sqrt(chroma_bar7 / (chroma_bar7 + 25.0**7))
    lightness_term = 1.0 + (0.015 * (lightness_bar - 50.0) ** 2) / np.sqrt(
        20.0 + (lightness_bar - 50.0) ** 2
    )
    chroma_term = 1.0 + 0.045 * chroma_bar
    hue_term = 1.0 + 0.015 * chroma_bar * t
    rotation_term = -np.sin(np.radians(2.0 * delta_theta)) * rotation

    lightness_scaled = delta_lightness / lightness_term
    chroma_scaled = delta_chroma / chroma_term
    hue_scaled = delta_hue / hue_term
    squared = (
        lightness_scaled**2
        + chroma_scaled**2
        + hue_scaled**2
        + rotation_term * chroma_scaled * hue_scaled
    )
    return np.sqrt(np.maximum(squared, 0.0))


def difference_image(reference, candidate):
    delta = np.abs(reference - candidate)
    amplified = np.clip(delta * 4.0, 0.0, 255.0)
    return Image.fromarray(amplified.astype(np.uint8), mode="RGB")


def compare(reference_path, candidate_path, tolerance, diff_path):
    reference = load_rgb(reference_path)
    candidate = load_rgb(candidate_path)
    reference, candidate = align(reference, candidate)
    if reference.size == 0:
        raise SystemExit("parity_check: the two frames do not overlap")

    match_percent = channel_match_percent(reference, candidate, tolerance)
    mae = mean_absolute_error(reference, candidate)
    structural = ssim(reference, candidate)
    delta_e = ciede2000(
        srgb_to_lab(reference).reshape(-1, 3), srgb_to_lab(candidate).reshape(-1, 3)
    )

    if diff_path:
        difference_image(reference, candidate).save(diff_path)

    return {
        "reference": reference_path,
        "candidate": candidate_path,
        "width": int(reference.shape[1]),
        "height": int(reference.shape[0]),
        "tolerance": tolerance,
        "channel_match_percent": round(match_percent, 4),
        "mae": round(mae, 4),
        "ssim": round(structural, 6),
        "delta_e_mean": round(float(np.mean(delta_e)), 4),
        "delta_e_p95": round(float(np.percentile(delta_e, 95)), 4),
        "delta_e_over_5_percent": round(100.0 * float(np.count_nonzero(delta_e > 5.0)) / delta_e.size, 4),
    }


def selftest():
    """Validate the metric implementations against known references."""
    failures = []

    def check(name, value, expected, epsilon):
        if abs(value - expected) > epsilon:
            failures.append("%s: %.6f != %.6f" % (name, value, expected))

    # Identical frames must score perfectly.
    frame = np.zeros((64, 64, 3), dtype=np.float64)
    frame[16:48, 16:48] = 200.0
    check("ssim-identical", ssim(frame, frame), 1.0, 1e-9)
    check("match-identical", channel_match_percent(frame, frame, 8), 100.0, 1e-9)
    check("deltae-identical", float(np.max(ciede2000(srgb_to_lab(frame).reshape(-1, 3),
                                                   srgb_to_lab(frame).reshape(-1, 3)))), 0.0, 1e-9)

    # A frame shifted by one pixel must not be identical but must stay close.
    shifted = np.roll(frame, 1, axis=1)
    check("ssim-shifted-below-one", 1.0 if ssim(frame, shifted) < 1.0 else 0.0, 1.0, 0.0)

    # Sharma, Wu and Dalal CIEDE2000 test pairs (subset).
    sharma = [
        ((50.0, 2.6772, -79.7751), (50.0, 0.0, -82.7485), 2.0425),
        ((50.0, 3.1571, -77.2803), (50.0, 0.0, -82.7485), 2.8615),
        ((50.0, 2.8361, -74.0200), (50.0, 0.0, -82.7485), 3.4412),
        ((50.0, -1.3802, -84.2814), (50.0, 0.0, -82.7485), 1.0000),
        ((50.0, 0.0, 0.0), (50.0, -1.0, 2.0), 2.3669),
        ((60.2574, -34.0099, 36.2677), (60.4626, -34.1751, 39.4387), 1.2644),
        ((2.0776, 0.0795, -1.1350), (0.9033, -0.0636, -0.5514), 0.9082),
    ]
    for index, (first, second, expected) in enumerate(sharma):
        value = float(ciede2000(np.array([first]), np.array([second]))[0])
        check("sharma-%d" % index, value, expected, 1e-4)

    # sRGB to CIELAB against the standard sRGB reference values.
    lab_white = srgb_to_lab(np.array([[[255.0, 255.0, 255.0]]]))[0]
    check("lab-white-l", lab_white[0], 100.0, 1e-3)
    check("lab-white-a", lab_white[1], 0.0, 1e-3)
    check("lab-white-b", lab_white[2], 0.0, 1e-3)
    lab_red = srgb_to_lab(np.array([[[255.0, 0.0, 0.0]]]))[0]
    check("lab-red-l", lab_red[0], 53.2408, 1e-3)
    check("lab-red-a", lab_red[1], 80.0925, 1e-3)
    check("lab-red-b", lab_red[2], 67.2032, 1e-3)
    lab_green = srgb_to_lab(np.array([[[0.0, 255.0, 0.0]]]))[0]
    check("lab-green-l", lab_green[0], 87.7347, 1e-3)
    check("lab-green-a", lab_green[1], -86.1827, 1e-3)
    check("lab-green-b", lab_green[2], 83.1793, 1e-3)

    if failures:
        for failure in failures:
            print("parity_check selftest FAILED: %s" % failure)
        return 1

    print("parity_check selftest passed: %d CIEDE2000 pairs, CIELAB primaries, SSIM" % len(sharma))
    return 0


def main(argv):
    parser = argparse.ArgumentParser(description="Measure F.E.A.R. visual parity between two frames")
    parser.add_argument("reference", nargs="?", help="retail reference frame")
    parser.add_argument("candidate", nargs="?", help="port frame")
    parser.add_argument(
        "--tolerance",
        type=int,
        default=8,
        help="per-channel tolerance in 0-255 units used by the primary metric (default: 8)",
    )
    parser.add_argument("--diff", help="write an amplified absolute difference image")
    parser.add_argument("--json", dest="json_path", help="write the metric result as JSON")
    parser.add_argument("--selftest", action="store_true", help="validate the metric implementations")
    arguments = parser.parse_args(argv)

    if arguments.selftest:
        return selftest()

    if not arguments.reference or not arguments.candidate:
        parser.error("reference and candidate frames are required")

    result = compare(arguments.reference, arguments.candidate, arguments.tolerance, arguments.diff)
    print(json.dumps(result, indent=2))
    if arguments.json_path:
        with open(arguments.json_path, "w", encoding="utf-8") as handle:
            json.dump(result, handle, indent=2)
            handle.write("\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
