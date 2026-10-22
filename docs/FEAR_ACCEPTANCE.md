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

- The menu, v5 client facade, LTAR/asset staging, v113 world parser and the retail v5 server
  bootstrap are implemented.
- The first mission reaches `GS_PLAYING` with a server player object in
  `Worlds\Release\Intro`.
- The port camera is now attached to the server player and rendered through `CPlayerCamera`;
  `CInterfaceMgr` draws the HUD in the same frame.
- The v113 renderer batches surfaces by material, decodes `.Mat00`/DDS data and uploads 455
  material textures. Lighting, lightmaps, `.fxo`, models, weapons and shadows are still absent.
- The local-player character/move-manager path is not initialized, so HUD values and gameplay
  state remain incomplete.

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
