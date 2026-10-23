# F.E.A.R. port acceptance criteria

Status: active goal. This document defines the gates for the next milestone. It does not
claim that any gate has passed.

## Required outcome

1. The source port starts from the staged run directory and reaches the main menu.
2. The first single-player mission, Interval 01 - Inception, starts from the menu.
3. The player is spawned at the level start point, the mission scripts and initial HUD run, and
   the player can move and look around with the intended controls.
4. At a matched retail camera and state, the port reaches at least 95% visual parity with the
   retail game.

## Visual parity definition

Visual parity is measured from the back buffer, not from a desktop screenshot.

- Reference: retail F.E.A.R. 1.08.282 running the same mission and the same view.
- Capture resolution and aspect ratio must match between the two clients.
- The camera position, rotation, FOV, render mode, time of day and HUD state must match.
- Primary metric: at least 95.00% of pixels have every RGB channel within 8/255 of the reference.
- Secondary metrics: mean absolute error (MAE), silhouette/coverage difference, and a
  side-by-side plus absolute-difference image for every acceptance frame.
- The threshold applies to a set of representative frames covering the start area, a corridor,
  an outdoor view, a lit interior, a weapon view and a HUD-heavy frame. A single frame is not
  sufficient.
- Frames with a documented dynamic difference (for example a scripted event that cannot yet be
  synchronized) must be recorded separately and cannot be used to raise the aggregate result.
- The reference and port must render with the same texture filtering, resolution, brightness,
  gamma and post-processing settings.

## Comparison tool

`tools/parity_check.py` implements the metrics above and is the only accepted way to quote a
parity number. It reports the primary channel-tolerance percentage, mean absolute error, luma
SSIM and CIEDE2000 statistics, and can write an amplified difference image.

```
python tools/parity_check.py reference.png candidate.png --tolerance 8 --diff diff.png --json result.json
python tools/parity_check.py --selftest
```

The self test validates the CIEDE2000 implementation against the Sharma, Wu and Dalal reference
pairs and the sRGB to CIELAB conversion against the sRGB primaries, so a metric regression cannot
be mistaken for a rendering regression. Both frames are cropped to the overlapping area, so the
two clients do not have to share a resolution for the metrics to be comparable, although the
acceptance rule still requires matched captures.

## First-mission gates

- Menu: Single player -> New Game reaches the loading screen without manual input.
- Loading screen reaches Interval 01 - Inception and completes.
- Server world: the level loads without "Server is missing class" diagnostics.
- Client world: the client receives the start-level/player state and leaves the loading screen.
- Player: a server-side player object exists and is associated with the local client; the client
  camera is driven by that player.
- Movement: forward/back/strafe, jump, crouch, mouse look and firing are accepted and reflected
  in the world.
- HUD: health, armor, weapon/ammo, crosshair, mission text and damage indicators render.
- Persistence: the process remains responsive for a 120 second soak with no crash, assertion or
  memory corruption.

## Provenance rule

Movement, collision, HUD, weapons, object replication and rendering behavior must be recovered
from the shipped engine/game binaries and the available source. New behavior may be written as
port glue, but it must be backed by an address, a decompiled function, a source location or a
measured asset format. A plausible invention is not acceptance evidence.

## Evidence ledger

Each gate records:

- revision (commit and build configuration);
- exact command line, stage path and desktop;
- raw log path;
- reference and port capture paths;
- metric values and the exact comparison command;
- what passed, what failed and what remains unverified.

## Current baseline

* `FEAR_LIGHTS=1` bakes the decoded light list into vertex colours. Measured at
  `mean=16.2`, so it is opt-in only: the ambient term is `(25,25,25)` with
  ~300-unit light radii in a level spanning tens of thousands of units, and the
  broad fill lives in the `.fx` shaders. Default rendering is unchanged at
  `mean=77.1 colors=65893`.
* Material files are decoded and confirmed against the archive: `FEAR_MATDUMP=1`
  prints `shader=<path>.fx` plus every typed parameter, and the diffuse texture is
  now selected by the `tDiffuseMap` name rather than by "first `.dds`".
* Materials carry `tDiffuseMap` / `tEmissiveMap` / `tSpecularMap` / `tNormalMap`
  and no lightmap stage, so world lighting has to come from `g_WorldLights` and
  the `.fx` shaders. Fixed-function lighting is not a substitute and stays off.

- The menu, v5 client facade, LTAR/asset staging, v113 world parser and the retail v5 server
  bootstrap are implemented.
- The first mission reaches `GS_PLAYING` with a server player object in
  `Worlds\Release\Intro`.
- The port camera is now attached to the server player and rendered through `CPlayerCamera`;
  `CInterfaceMgr` draws the HUD in the same frame.
- The v113 renderer batches surfaces by material, decodes `.Mat00`/DDS data and uploads 455
  material textures. Lighting, lightmaps, `.fxo`, models, weapons and shadows are still absent.
- The local-player path is initialized: `CPlayerCamera::InitLocalPlayer`, the character factory
  request and the player model lookup all run, and the camera transform is read from the server
  player every frame. `MODL` parsing is still stubbed, so the character animation set reports
  `no Null weightset on Character` and no view model is drawn. HUD values remain incomplete
  because movement, collisions and the move manager are not wired yet.
- The capture path renders the world into an offscreen `D3DUSAGE_RENDERTARGET` surface instead of
  reading back the presented back buffer, because the v4 host does not expose a readable back
  buffer. The resulting BMP is the frame that parity is measured from.
- The camera projection uses the client's own `ILTClient::SetCameraFOV` values instead of a
  hard-coded constant: `PlayerCamera` reports `fovX = 1.5936 rad (91.3 deg)` and
  `fovY = 1.3090 rad (75.0 deg)` at 1024x768, and the renderer derives the aspect from
  `tan(fovX / 2) / tan(fovY / 2)` rather than assuming 4:3.
- Vertex streams are decoded from the file's own vertex-type table instead of a stride heuristic:
  all 11 declared vertex types are expanded into `(stride, uvOffset, normalOffset, colorOffset)` by
  accumulating property formats in declaration order, and each surface is bound to its `vertexType`
  field. The computed strides (84/60/56/32/24/12) match the recorded per-surface `vertex_size`
  exactly for 2267 of 2268 surfaces. 2096 surfaces carry normals, 250 carry baked RGBA colour.
  Decoded normals and colours are uploaded (`XYZ|NORMAL|DIFFUSE|TEX1`) but lighting is still off.
- Face culling defaults to `D3DCULL_CCW`, measured rather than assumed: `cw` drops mean luma from
  76.9 to 37.7 and non-black coverage to 97.4%, i.e. it discards most of the scene, while `ccw`
  keeps the scene and only removes back faces. `FEAR_CULL=none|cw` remains as an override.
- The v113 object section is parsed in C++ and agrees with the independent Python reference:
  1988/1988 objects, `LightPoint=35`, `LightSpot=12`, `LightDirectional=2`, `LightCube=39`,
  `max_radius=3000`, `AmbientLight=(25,25,25)` and the first `GameStartPoint` at
  `(-1100,-1436,-370)`, which is exactly where the server spawns the player. The lights are not
  yet fed to the renderer.

## Iteration 8 evidence

- Client/server state: the client reaches `GS_PLAYING`, receives InWorld, and
  has player state Alive. The server has a real-shaped client record and a
  player object.
- This is a startup/state milestone, not a visual milestone: the renderer is
  still the flat v113 preview and the player camera is not yet wired.
- The visual metric remains unpassed because the current frame does not match
  retail geometry, materials, lighting, HUD or weapon content.

## Iteration 9 evidence

- Camera: the port reaches `GS_PLAYING`, attaches `CPlayerCamera` to the server player target,
  updates it every frame and renders it through the v5 renderer callback.
- HUD: the interface manager draws into the same 640x480 frame; health/armor widgets are visible
  but not yet populated from complete player state.
- Geometry: 1,979 visible surfaces, 58,355 triangles and 457 used materials are rendered.
- Materials: 455 DDS textures were decoded and uploaded from the parsed `.Mat00` material table.
- Capture: the D3D9 back/front buffer is written to a BMP by `ILTRenderer::MakeScreenShot`.
  `D:\fear-re\tmp\port_materials.bmp` is the current iteration capture.
- Visual metric: not accepted. The frame is textured and shows HUD, but there is no lightmap,
  lighting, `.fxo`, model, weapon or shadow parity with retail.

## Iteration 10 evidence

- Renderer: the v113 world is drawn through a D3D9 HLSL vertex/pixel shader pair
  instead of the fixed-function pipeline. The vertex stage transforms with
  `row_major float4x4 g_WorldViewProj : register(c0)` and forwards world position,
  normal, vertex colour and UV; the pixel stage applies the baked vertex lighting,
  a cotangent-frame tangent-space normal map, per-pixel specular from the 8
  selected of 88 decoded lights, the emissive map and an ACES tone curve.
- Constant layout is pinned explicitly on both stages after a measured register
  collision put the vertex matrix at `c36-c39` while the CPU wrote `c0`; the
  collision silently produced an empty frame.
- Regression status: the fixed-function path is retained as a fallback behind
  `FEAR_SHADER=0` and still measures `mean=76.83 colors=65893`.
- Measured parity against the fixed-function reference frame
  (`run-fear-engine\ref_ff.bmp`, frame 60, 1024x768):
  `mean=77.15 std=59.7 blockstd=55.5 nonblack=97.5% colors=63180`,
  block brightness-ratio `p10=0.917 p50=0.993 p90=1.080`, correlation `0.9849`,
  clipped (`>=250`) `0.00%` against `0.38%` for the reference.
- Not yet accepted as retail parity: the reference is the port's own
  fixed-function frame, not a retail capture. The gate in "Visual parity
  definition" requires `tools/parity_check.py` against retail F.E.A.R. running
  the same mission and view, which is still unavailable, so this iteration
  establishes shading correctness and not the 95% gate.
- Known gaps: `MODL` is still stubbed so no characters, weapons or view model are
  drawn; fog is disabled in the world (`FogEnable=0`); `LightCube` lights are
  evaluated as omnidirectional point lights rather than projected cube maps;
  reflections, shadow volumes and `.fxo` post effects are absent.