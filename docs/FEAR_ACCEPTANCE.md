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

- The menu, v5 client facade, LTAR/asset staging and v113 world preview are implemented.
- The retail v5 server bootstrap can load Worlds\Release\Intro.World00p.
- The client is still a preview: there is no real client-server connection, player, HUD or
  gameplay.
- The world renderer currently uses flat per-material colors; materials, textures, models,
  lighting and post-processing are not at parity.

## Iteration 8 evidence

- Client/server state: the client reaches `GS_PLAYING`, receives InWorld, and
  has player state Alive. The server has a real-shaped client record and a
  player object.
- This is a startup/state milestone, not a visual milestone: the renderer is
  still the flat v113 preview and the player camera is not yet wired.
- The visual metric remains unpassed because the current frame does not match
  retail geometry, materials, lighting, HUD or weapon content.
