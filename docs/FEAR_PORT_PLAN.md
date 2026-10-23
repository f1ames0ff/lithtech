# F.E.A.R. source port plan

Status: the main menu is reached, renders, and supports keyboard/mouse navigation. A new
single-player game reaches the loading screen for "Interval 01 - Inception" and enters a static
v113 world preview that renders the level geometry from the start point; the v5 server and game
logic are still missing. The port code exists under `FEAR/Compat`, and the latest milestone is
recorded in "Iteration 6" at the end of this document. Earlier sections keep their historical
status notes and are superseded by that section.

Revision 4 incorporates the final independent audit. The audit found that the v5 facade cannot be
"outside" the module registry: F.E.A.R. game code resolves `g_pLTClient` through
`define_holder(ILTClient, ...)`, and the registry rejects a version mismatch. The design below
therefore registers every F.E.A.R. interface under the instance name `Fear` and keeps the engine's
`Default` v4 interfaces for the engine. Revision 4 also adds the v4 `CInstanceHandleClient` adapter,
the complete holder list, the `object.lto` export with a v4 `ClassDef`/`PropDef` adapter, the
interface-limit fixes, the stub resource DLLs, and a writable staging directory.

The changes made after the final audit (the instance-handle adapter, the holder list, the
`object.lto` layout, the corrected counts and the rebalanced schedule) were not independently
re-audited; the audit budget was exhausted. They are marked by their explicit acceptance checks and
must be verified during M1a and M4.

Revision 5 removes M0 (legal/provenance gate) by project decision: licensing review is explicitly
out of scope for this plan and no legal gate is part of the schedule. The associated risk is
recorded as accepted in section 6; it has no mitigation and does not block any milestone.

## 1. Goal and scope

Run the retail F.E.A.R. single-player campaign (Steam Ultimate Shooter Edition, data set matching
the public F.E.A.R. SDK 1.08) on the GPL LithTech "Jupiter 69" engine in this repository.

- Platform: Windows, x86, Debug first. Release only after the Debug path is stable.
- Delivery: a `CShell.dll` built from the SDK game code, loaded by the existing engine through its
  normal client-shell path, reading the retail `*.Arch00` archives directly.
- Non-goals for the first playable build: multiplayer, GameSpy, PunkBuster, dedicated server, mod
  tools, editors, 64-bit, Linux, save-game compatibility with retail, graphical parity, F.E.A.R.
  Extraction Point / Perseus Mandate.

## 2. Verified starting point

The following facts were checked directly against the repository, the SDK mirror and the retail
install. They are not assumptions.

### 2.1 Engine

- Builds and links Debug x86 with MSVC 2022 Build Tools and Ninja. Artifacts in `build-win32/OUT`:
  `Lithtech.exe`, `Server.dll`, `SndDrv.dll`, `LTMsg.dll`. Entry point: `tools/build_windows.ps1`.
- Version string `Jupiter 69` (`sdk/inc/iltinfo.h`).
- Interfaces: `IClientShell` 4, `ILTClient` 4, `ILTCommon` 4, `IServerShell` 4.
- `IClientShell` v4 begins with `PreUpdate()` and contains `HandleEvent(SDL_Event)`;
  `OnEngineInitialized(RMode *, LTGUID *)`; `OnModelKey(HLOCALOBJ, ArgList *)`
  (`sdk/inc/iclientshell.h:48,59,73,127,265`).
- `ILTClient` v4 is a class with about 189 function-pointer members plus its own virtuals
  (`runtime/client/src/clientde_impl.cpp`).
- World loader accepts version 85 only (`runtime/world/src/world_shared_bsp.h:34`,
  `runtime/world/src/world_shared_bsp.cpp:378`).
- Model loader accepts `.ltb`, model version 23-25, LTB load version 9
  (`runtime/model/src/model.h:48`, `runtime/model/src/ltb.h:12`).
- Textures are DTX (`runtime/render_a/src/sys/d3d/d3d_texture.cpp`); there is no DDS parser.
- Resources are read through `libs/rezmgr`, which implements the old text-header `.rez` format.
- The client-shell loader requires `cshell.dll` and then `cres.dll`
  (`runtime/kernel/src/sys/win/dsys_interface.cpp:509-574`). The server loader requires
  `sres.dll` (`dsys_interface.cpp:330-338`).
- Resource-tree limits: `MAX_RESTREES` is 20 (`runtime/client/src/clientmgr.h:103`) and
  `MAX_FILETREES_TO_SEARCH` is 32 (`runtime/kernel/io/src/sysfile.h:20`). The retail data set has
  36 archives, so both limits must be raised.
- The engine writes `autoexec.cfg` and `error.log` into the current working directory
  (`runtime/client/src/clientmgr.cpp`, `runtime/client/src/errorlog.cpp:32`). Running with
  `-workingdir <retail>` would write into the retail install.

### 2.2 F.E.A.R. SDK 1.08 and game code

- SDK mirror at `C:\tools\fear-sdk`; `Game/` matches `FEAR/` except for project files.
- `engine/sdk/inc` contains 112 headers, including `platform.h`, `engine.h`, `iltfilemgr.h`,
  `iltrenderer.h`, `iltcustomrender.h`, `iltphysicssim.h`, `iperformancemonitor.h`,
  `idatabasemgr.h`, `resourceextensions.h`.
- Game-side libraries `libs/platform` and `libs/stdlith` are present as source.
- `IClientShell` 5 begins with `OnEngineInitialized(LTGUID *, RMode *)`, has no `HandleEvent`, and
  `OnModelKey` takes an `ANIMTRACKERID` (`engine/sdk/inc/iclientshell.h:31,61,173`).
- `ILTClient` 5 (derived from `ILTCSBase` 5) and `ILTCommon` 1.
- Static count of the v5 client surface: 39 function-pointer members, 33 own virtuals and 87
  `ILTCSBase` virtuals (159 entries). The adapter has to provide both the pointer members and the
  virtual methods, plus `ILTRenderer` (80 virtuals), `ILTTextureMgr` (9), `ILTDrawPrim` (23),
  `ILTCommon` and the server interfaces.
- F.E.A.R. modules obtain their interfaces through the engine's module registry:
  `SETUP_CLIENTSHELL()` expands to `define_holder(ILTClient, g_pLTClient)`; `GlobalsInit.cpp` also
  holds `ILTModelClient`, `ILTDrawPrim`, `ILTTextureMgr`, `ILTTextureString`, `ILTRenderer`, and
  calls `define_interface(CGameClientShell, IClientShell)` (`FEAR/ClientShellDLL/GlobalsInit.cpp`).
  The server does the same with `IServerShell` (`FEAR/ObjectDLL/GlobalsInit.cpp`).
- The registry key is `"Interface.Instance"` (`sdk/inc/ltmodule.h:165`); a version mismatch is
  rejected (`sdk/inc/ltmodule.cpp:440-453`) and `VersionError` breaks in Debug. Registering the v5
  shell as `IClientShell.Default` therefore collides with the engine's v4 registration and would
  leave `g_pLTClient` null.
- Game source: Shared 56k LOC, ClientShellDLL 173k LOC, ObjectDLL 288k LOC, ClientFxDLL 17k LOC,
  LTGUIMgr 13k LOC.
- The game code directly includes 71 SDK headers; 42 of them are absent from the repository's
  `sdk/inc` (65 of the 112 mirror headers are not in the repo). The SDK mirror supplies them,
  except `MeleeWeaponModel.h` and the Xenon-only `endianswitch.h`. Missing implementations:
  GameSpy, PunkBuster, `Shared_CRC.lib`. `AssertMgr` sources exist.
- The existing `FEAR/` CMake files are incomplete and point at the engine's v4 headers; they are
  not a usable build.

### 2.3 Retail data

- Install: `D:\SteamLibrary\steamapps\common\FEAR Ultimate Shooter Edition`.
- No `cshell.dll` on disk. The retail client shell is shipped as `GameClient.dll` inside
  `FEARE.Arch00` and its eight numbered volumes (`FEARE_1..8.Arch00`); the archive also contains
  `GameServer.dll` and `ClientFx.fxd`, and the DLL payload contains `CGameClientShell` and
  `IClientShell.Default` strings. `EngineServer.dll` has no game exports. The port places its own
  `CShell.dll` on disk, so the engine's loader can find it.
- The base `FEARE.Arch00` copy of `GameClient.dll` is an older build: it registers `IClientShell`
  4, `ILTClient` 104 and `ILTServer` 104. The copies in `FEARE_5..8.Arch00` register
  `IClientShell` 5, `ILTClient` 105 and `ILTServer` 105, which match the SDK 1.08 headers
  (`ILTCSBase` 1 + `ILTClient` 5 = 105). The port must load the patched copy; the LIFO archive
  order selects `FEARE_8` when all volumes are mounted.
- 36 `*.Arch00` archives listed in `Default.archcfg`.
- LTAR version 3 is fully parsed:

```
Header, 48 bytes:
  char    magic[4]        "LTAR"
  uint32  version         3
  uint32  namePoolSize
  uint32  dirCount
  uint32  fileCount
  uint32  1
  uint32  0
  uint32  flag
  uint8   guid[16]

Layout:
  name pool      at 48
  record table   at 48 + namePoolSize
  dir table      at 48 + namePoolSize + 32 * fileCount
  file data      at 48 + namePoolSize + 32 * fileCount + 16 * dirCount

Record, 32 bytes:
  uint32 nameOffset      relative to the name pool at 48
  uint64 dataOffset
  uint64 packedSize
  uint64 unpackedSize
  uint32 flags
```

- All 36 archives validate: 17,258 records, no out-of-bounds entries, `packedSize == unpackedSize`
  and `flags == 0` for every record. The data is stored, not compressed. The tail CRC/name table is
  optional.
- Paths require the 16-byte directory table; record names alone do not contain `\`. Two
  `Streets.World00p` records exist, so archive order decides which one wins.
- `FEAR.Arch00` is 3,846,960,665 bytes; 1,743 records live above the 2 GB boundary, so 64-bit
  offsets and streaming are mandatory.
- `Streets.World00p` (record 10570) is at offset 3,817,537,327, size 28,980,611, and begins with
  `0x71` = 113.
- 49 world records are version 113, 45 unique names (duplicates: Asylum, Docks, Factory, Streets);
  the remaining 30 records are 93-111. The engine only knows 85. A v113 world is plaintext with
  version + four section offsets + geometry; strings such as `40oz00.Glass` and `WorldProperties`
  are present.
- Models: 528 `*.Model00p` records (515 unique names); 510 are `MODL` version 33, 2 are `MODL` 31,
  3 are byte-swapped `LDOM` 34, 13 are LTB-like.
- `LTMI` version 1 files are materials: 2,752 `.Mat00` records plus 2 `.Mat01` records.
- Textures are DDS (DXT1/DXT3/DXT5 and raw), thousands of files. Sounds are RIFF. Databases are
  `GADB` v3. Animation trees are `ANMT` v3.
- The same logical path can exist in more than one archive with different content; override
  precedence is not documented.

## 3. Architecture

### 3.1 Chosen design

A single `CShell.dll` contains four layers:

1. **Engine-facing proxy** implementing `IClientShell` **version 4**. It is registered as
   `IClientShell.Default` v4, which is exactly what `Lithtech.exe` looks up. `Server.dll` gets an
   analogous `IServerShell.Default` v4 proxy in the object module.
2. **F.E.A.R.-facing facade** implementing the SDK 1.08 interfaces under the instance name
   **`Fear`**: `IClientShell.Fear` v5, `ILTClient.Fear` v5, `ILTServer.Fear` v5,
   `ILTRenderer.Fear`, `ILTTextureMgr.Fear`, `ILTDrawPrim.Fear`, `ILTCommon.Fear`,
   `ILTModelClient.Fear`, `ILTTextureString.Fear`, `ILTClientContentTransfer.Fear`,
   `ILTServerContentTransfer.Fear`, `ILTCustomRender.Fear`, `IServerShell.Fear` v5, and the
   remaining service interfaces. No `Default` v5 registrations exist, so nothing collides with
   the engine's v4 entries.
3. **Registration shim** for the F.E.A.R. translation units. `fear_holder_shim.h` is force-included
   (`/FI`) into every F.E.A.R. translation unit, or included at the top of the three module PCHs
   `FEAR/ClientShellDLL/Stdafx.h`, `FEAR/ObjectDLL/Stdafx.h` and `FEAR/ClientFxDLL/stdafx.h` plus
   `sdk/inc/ltobjref.cpp`. It includes `ltmodule.h`, then replaces the registry macros:
   - `define_interface(Impl, I)` becomes `instantiate_interface(Impl, I, Fear)`;
   - `define_holder(T, var)` becomes `define_holder_to_instance(T, var, Fear)`;
   - `SETUP_CLIENTSHELL()` expands through the redefined `define_holder`, so `g_pLTClient`
     resolves to `ILTClient.Fear`.
   - `IInstanceHandleClient` is the exception, and it needs a **v4-ABI adapter**, not an alias:
     the v4 `IBase` has a virtual destructor, so slot 2 is `SetInstanceHandle`, while v5 has no
     virtual destructor and slot 1 is `SetInstanceHandle`. Registering the v5 object as `Default`
     would call `GetInstanceHandle` instead. The proxy translation unit compiles a v4
     `CInstanceHandleClient` adapter (v4 header) and registers it as `Default`; the v5 object stays
     under `Fear` only (F.E.A.R. never calls `GetInstanceHandle`).
   The shim does not modify the SDK headers and does not merge v4 and v5 translation units.
4. **Game code** compiled against the SDK 1.08 headers, in its own translation units. Engine v4
   headers and F.E.A.R. v5 headers never appear in the same translation unit.

The engine's `SetMasterDatabase` transfer still applies: the DLL's local registrations (`Default` v4
proxy and `Fear` v5 facade) are merged into the engine database under distinct names, so both are
visible. The proxy constructs the F.E.A.R. shell directly, or resolves `IClientShell.Fear`; it
never relies on a v5 `Default` lookup.

Translation between generations is explicit, not a cast:

- `OnEngineInitialized` argument order is swapped.
- `OnModelKey` gains `ANIMTRACKERID`.
- The engine's `HandleEvent(SDL_Event)` has no v5 equivalent; input is routed through the F.E.A.R.
  `ILTInput` facade instead.
- `OnEvent`, `PreUpdate`, `Update`, `OnEnterWorld`, `OnExitWorld` and the remaining callbacks are
  forwarded by name, not by vtable slot.

Server side: `ObjectDLL` registers `CGameServerShell` as `IServerShell.Fear` v5 and an
`IServerShell.Default` v4 proxy. The proxy forwards the engine's v4 server calls to the v5 shell.
The object module is loaded as `object.lto` through `cb_LoadModule` and must export
`extern "C" __declspec(dllexport) ClassDef **ObjectDLLSetup(int *nDefs, ILTServer *pServer,
int *version)` with `SERVEROBJ_VERSION` 1 (`runtime/shared/src/classbind.cpp:23,64-79`,
`sdk/inc/ltserverobj.h:353`). No `object.lto` exists in retail, so the port supplies one. The v5
`ClassDef`/`PropDef` layout differs from v4 (v5 adds `m_ClassDesc` and `m_GatherResourcesFn`,
widens `m_nProps`, and adds `m_PropDesc`), while `cb_VerifyClassDefProperties` reads the v4 layout.
The object module therefore exports **v4-layout** class definitions produced by an adapter in the
shim TU, not the v5 structs directly.

Static libraries: the game code is compiled into `FEAR_ClientShell` / `FEAR_Object` static libs.
Their `define_interface` registrations must survive linking; use `/WHOLEARCHIVE` on the compat and
game libraries, or the v5 `link_to_implementation` keyed by interface instance
(`C:\tools\fear-sdk\engine\sdk\inc\ltmodule.h:151-199`).

Resource DLLs: `cres.dll` and `sres.dll` are required by the loader. The port builds minimal stub
DLLs that satisfy the loader; the game does not use their contents.

### 3.2 Why not the alternatives

- **Raise the engine to v5 in place.** The engine client calls 67 `ILTClient` methods, 60 of which
  do not exist by name in v5; it calls 29 `IClientShell` methods, 7-8 of which are absent in v5.
  The 2D/surface/renderer API would have to be rewritten against `ILTRenderer` (80 virtuals),
  `ILTTextureMgr` (9) and `ILTDrawPrim` (23). This is a renderer port disguised as a version bump.
- **Register both v4 and v5 as `Default`.** Not possible. The registry key is
  `"Interface.Instance"` and the version check rejects a mismatch; a v5 `Default` registration is
  dropped, and creating a second implementation with the same name is a debug break. The `Fear`
  instance name is what makes coexistence legal.
- **Binary drop-in of retail DLLs.** The retail `GameClient.dll` is v5 and cannot be loaded by the
  v4 engine without a control-flow mismatch. The port builds its own shell.

## 4. Milestones

Every milestone has a command or a log line that decides pass/fail. Effort is in engineer-weeks for
one experienced engine engineer; ranges reflect uncertainty, not padding.

### M1 - ABI skeleton, holder shim and renderer smoke test (4 ew)

M1a - registry and holder shim (1.5 ew):

- New CMake options: `BUILD_FEAR_PORT`, `FEAR_SDK_DIR`, `FEAR_RETAIL_DIR`.
  `BUILD_FEAR_PORT=ON` forces `BUILD_NOLF2=OFF` and `BUILD_FEAR=OFF` and writes to `OUT_FEAR`; the
  existing NOLF2 `CShell`/`CRes`/`SRes` outputs and the old `FEAR_ClientShellDLL` target otherwise
  collide by output name.
- New targets: `FEAR_Compat` (static), `FearCShell` (DLL, output `CShell`), `FearCres` (output
  `cres`), `FearSres` (output `sres`), `FEAR_AbiProbe`.
- Implement the registration shim and prove it: `ILTClient.Default` resolves to v4 for the engine,
  `ILTClient.Fear` resolves to v5 for the game code, the v4 `CInstanceHandleClient.Default` adapter
  stores and returns the module handle, no `VersionError` in Debug.
- Implement the v4 client proxy and the v5 facade skeleton. Every entry in the manifest gets an
  explicit implementation or an `LT_UNSUPPORTED` stub: `ILTClient` 159, `ILTRenderer` 80,
  `ILTTextureMgr` 9, `ILTDrawPrim` 23, `IClientShell` 28, `IServerShell` 17, `ILTServer`,
  `ILTCommon`, `ILTModelClient`, `ILTTextureString`, `ILTClientContentTransfer`,
  `ILTServerContentTransfer`, `ILTCustomRender`, and the file/input/timer/game-util interfaces.
  No null pointers.
- Pass: engine loads `CShell.dll` and `cres.dll`, logs the v4 proxy and the `Fear` registrations,
  a shimmed holder (`g_pLTClient`, `g_pLTServer`, `g_pLTRenderer`) is non-null at first use,
  `OnEngineInitialized` fires with a sentinel GUID and mode, no `VersionError`.

M1b - renderer smoke test (2.5 ew):

- `SetRenderMode` returns `LT_OK`; `GetRenderer()` returns a non-null `ILTRenderer.Fear`.
- Upload one DDS texture and draw one quad through `ILTDrawPrim.Fear` (not the engine v4 draw
  path), with a placeholder material; present a frame.
- Run 60 frames, compare the frame against a stored reference hash, pin the expected draw-call
  count, exit cleanly.
- Pass: frame matches the reference hash, draw-call count matches, 60 frames, clean exit.
- Kill: if the F.E.A.R. renderer surface cannot be driven over the engine's D3D9 device at all,
  stop and re-scope before building the game libraries.

### M2 - LTAR v3 data path (1.5 ew)

- New `libs/ltar` with `CLtarArchive`, index parsing, path normalization, the 16-byte directory
  table, and a streaming `ILTStream`. 64-bit offsets throughout.
- `runtime/kernel/io/src/sys/win/de_file.cpp`: new `LtarFileTree` branch, sniffing `LTAR` before
  `CRezMgr`; `.rez` stays untouched.
- `runtime/kernel/src/sys/win/client.cpp`: re-enable `-rez`, add `-archcfg` and `-arch`, mount
  `Default.archcfg` in listed order.
- Raise `MAX_RESTREES` to at least 48 and `MAX_FILETREES_TO_SEARCH` to at least 64, or add a
  composite tree. `client.cpp` adds four base restrees before the 36 archives, so 40 is exactly
  full; 48 leaves headroom.
- Duplicate-path precedence: the engine's file-tree list is LIFO
  (`CClientFileMgr::AddResourceTrees` inserts at the head), so the last mounted tree wins. The port
  logs the selected archive for every duplicate. M6 verifies the result; if the `FEAR.Arch00` copy
  of `Streets.World00p` (3,817,537,327) is the correct one, reverse the mount order or add an
  explicit precedence option. The `FEAR_1.Arch00` copy is at 15,200,237 (size 27,042,554).
- Add `-fear` mode parsing in `runtime/kernel/src/sys/win/client.cpp` (the flag does not exist yet);
  `-archcfg` selects the archive list, `-world` selects the map.
- `tools/ltar_dump` with `list`, `find`, `stat`, `extract`, `verify`.
- Pass: `ltar_dump list` reports 17,258 records; `find Streets.World00p` reports both records;
  reading record 10524 (`Intro`, 3,467,954,106) succeeds; an existing `.rez` fixture still mounts;
  the engine mounts all 36 archives; no file is written to the retail install.

### M3 - Format probe and go/no-go (2 ew)

- `tools/fear_probe`: dump the v113 world header and section table, the `MODL` 33 block layout, a
  DDS header, a `Mat00` header, an `ANMT` header and a RIFF sound header. Decode one sample of each
  headlessly.
- Compare each structure with the engine's loader and write a short schema note with offsets.
- Decision: extend the engine loader or add an offline converter. Prefer a loader for the first map
  and a converter only where the structure is clearly a different generation.
- Pass: one world, one model, one DDS and one sound decode to named fields; the schema note is
  accepted.
- Kill: no credible decode path within 2 ew. Do not start loader work on guesses.

### M4 - v5 host and retail shell load (3-5 ew)

The public SDK game code is not compiled for the first playable build. The patched retail
`GameClient.dll` in `FEARE_8.Arch00` matches the SDK 1.08 interfaces and is loaded as a vendor
binary, following the project rule to reuse shipped DLLs instead of reconstructing them.

- `FEAR/Compat/v5host`: a DLL compiled only against the FEAR SDK headers, with its own v5
  `CInterfaceDatabase`. It registers the v5 service facades, loads `GameClient.dll`, calls its
  `SetMasterDatabase`, and holds `IClientShell.Default` v5.
- `CShell.dll` (v4): registers the v4 proxy in the engine database and forwards the engine
  callbacks to the v5 host through a C ABI.
- `ClientFx.fxd`: a stub DLL exporting `fxGetNum`, `fxGetRef`, `fxVersion`, `fxInitDLLRuntime`,
  `fxTermDLLRuntime`, `fxDelete` and `fxFreePropList`, with no `SetMasterDatabase` export. The
  retail ClientFX DLL is not required for the menu.
- Pass: the engine loads `CShell.dll`, the v5 host loads `GameClient.dll`, `OnEngineInitialized`
  is called, and both the `Default` v4 and v5 interfaces resolve without a `VersionError`.

### M5 - v5 services, renderer and main menu (12-18 ew)

- Implement the F.E.A.R. service interfaces actually reached by the menu: `ILTFileMgr`,
  `ILTResourceMgr`, `ILTTextureMgr`, `ILTInput` (SDL events), `ILTTimer`, `ILTGameUtil`,
  `ILTLoadingProgress`, `IPerformanceMonitor`, `ILTCommon` messages, `ILTCursor`, and
  `IDatabaseMgr` backed by the retail DLL.
- Implement `ILTRenderer` (76 virtuals), `ILTDrawPrim` (23) and `ILTTextureMgr` (9) over the
  engine's D3D9 device: render targets, 2D quads, font/texture upload, DDS decoding, material
  placeholders. `ILTRenderer` is the long pole; the menu uses a subset, so log every method at
  first use and implement by call frequency.
- Pass: main menu renders, responds to keyboard and mouse, stays up for 60 seconds, screenshot
  captured, no crash and no unimplemented-abort.

### M6 - World, models and the first map (12-16 ew)

- World v113 loader (or converter) and BSP path; `MODL` 33 model loader; DDS upload path;
  placeholder materials for `.Mat00`; lightmap handling.
- Run from a writable staging directory that contains hardlinks (same volume) to the 36 archives;
  pass the asset root separately or use `-workingdir <staging>`. Never point the engine at the
  retail install.
- Pass: `Lithtech.exe -fear -workingdir <staging> -archcfg Default.archcfg -world Streets` loads
  `Streets`, logs the winning archive and offset (expected `FEAR.Arch00` / 3,817,537,327), renders
  BSP geometry, spawns a camera at the start point, runs 300 frames, produces a non-uniform
  screenshot and leaves the retail install unchanged (before/after file listing).

### M7 - Playable vertical slice (12-18 ew)

- Local server through `FEAR_Object` / `IServerShell` 5; physics and collision mapping; player
  movement; one weapon; one enemy; HUD; save/load disabled or stubbed.
- Pass: ten minutes of `Streets` with movement, shooting, one AI encounter and a working HUD;
  30 FPS at 1280x720 on the reference machine.

### M8 - Full single-player hardening (25-35 ew after M7)

- Remaining single-player maps, streaming, audio/FX, mission scripting, stability and performance.
- Pass: first mission completed; all single-player maps load; one-hour soak without a crash;
  known-issues list.
- This milestone is re-estimated after M6. If the first map requires renderer rewrites, it grows.

### Effort summary

| Milestone | Engineer-weeks |
|-----------|----------------|
| M1 ABI + renderer smoke | 4 |
| M2 LTAR | 1.5 |
| M3 format probe | 2 |
| M4 host + retail shell | 3-5 |
| M5 services + renderer + menu | 12-18 |
| M6 first map | 12-16 |
| M7 playable slice | 12-18 |
| M8 full SP | 25-35 |
| **Total** | **71.5-99.5, planning baseline 85.5** |

Critical path: M1 -> M2 -> M3 -> M4 -> M5 -> M6 -> M7 -> M8. M2 can start in parallel with
M1b once the shim exists. With two engineers the calendar is roughly 10-14 months; with three,
roughly 7-9 months. Add 20% contingency for the format work: planning range 86-119 ew.

The estimate is deliberately conditional: M1b and M3 are kill gates. If either fails, the remaining
work is not the table above.

## 5. Validation plan

- Unit tests:
  - LTAR header, record table, directory table, 64-bit offsets, duplicate paths, truncated
    archives, path traversal;
  - CRC vectors; ABI manifest (`sizeof`/`offsetof` and a vtable-order probe);
  - DDS header parsing.
- Synthetic data: an empty LTAR volume, a duplicate-name volume, a truncated volume and a sparse
  volume with records above 4 GB; no retail data required.
- Registration gate: both `ILTClient.Default` (v4) and `ILTClient.Fear` (v5) resolve, all manifest
  entries are non-null, and Debug contains no `VersionError`.
- Integration gates, in order:
  1. `ltar_dump list` reports 17,258 records.
  2. All 36 archives mount through `df_OpenTree` (requires the raised limits).
  3. `CShell.dll` + `cres.dll` load and the v4 proxy receives `OnEngineInitialized`.
  4. The `Fear` facade is complete and the renderer smoke frame is presented (pixel hash).
  5. Main menu appears.
  6. `Streets` loads and renders.
  7. Ten-minute playable slice.
  8. First mission, all maps, one-hour soak.
- Logging: stable markers `FEAR_PORT: archive_open`, `shell_loaded`, `engine_initialized`,
  `renderer_ready`, `menu_ready`, `world_enter`, `first_frame`, `shutdown`; a ring buffer for the
  ABI trace.
- Crash reporting: `MiniDumpWriteDump` plus a watchdog, PDB path recorded.
- Retail install is treated as read-only: stage hardlinks or a junction in a writable directory,
  write logs/config there, and verify with a before/after file listing that the install is
  untouched.

## 6. Risk register

| Risk | Impact | Likelihood | Mitigation |
|------|--------|------------|------------|
| GPL engine plus proprietary F.E.A.R. SDK code | Critical | High | Accepted by project decision (revision 5): licensing review is out of scope, no mitigation and no gate |
| Registry/holder mismatch (v5 vs v4) | Critical | High | `Fear` instance names, holder shim, Debug `VersionError` gate |
| World 113 / MODL 33 structure undecodable | Critical | Medium | M3 probe and kill gate before loader work; converter fallback |
| Renderer surface cannot be mapped over D3D9 | Critical | Medium | M1b smoke test before game libraries |
| ABI/ODR mistake across v4/v5 | Critical | Medium | Separate translation units, generated manifest, v4 proxy only as `Default` |
| 500k LOC of 2005-era C++ under MSVC 2022 | High | High | `/permissive`, C++14 per target, incremental targets |
| GameSpy/PunkBuster/CRC/asserts absent | Medium | High | Single-player stubs, CRC from zlib, explicit dependency matrix |
| 36 archives exceed engine limits | High | High | Raise `MAX_RESTREES` and `MAX_FILETREES_TO_SEARCH`; test all 36 |
| Archive override precedence wrong | Medium | Medium | Configurable rule, log every duplicate, two Streets records as test |
| 32-bit 2 GB address space with a 3.8 GB archive | Medium | Medium | 64-bit offsets, streaming, no whole-file mapping |
| Engine writes into the retail install | High | Medium | Writable staging directory, hardlinks, before/after listing |
| Performance | Medium | Medium | 30 FPS / 720p budget, profile per gate, no visual parity |
| Scope creep into multiplayer/tools | High | Medium | Non-goals in section 1, no MP work before M8 |

## 7. First two weeks

The two weeks deliver M2 (1.5 ew) and start M1a, which completes in week 3; M1b (2.5 ew) and M3
(2 ew) land in weeks 4-5 with two engineers, or weeks 3-4 with three. The full M1-M3 chain is
7.5 ew, not 2, and the schedule below reflects that.

Week 1:

- Day 1: add `BUILD_FEAR_PORT`, `FEAR_SDK_DIR`, `FEAR_RETAIL_DIR` to `CMakeLists.txt`; create
  `FEAR/CMakeLists.txt` and `cmake/FearSdk.cmake`.
- Day 2: LTAR header, name pool, record table and directory table in `libs/ltar`; unit tests with
  synthetic volumes.
- Day 3: `CLtarStream`; `tools/ltar_dump list/find/stat`.
- Day 4: mount all 36 archives in a test with `MAX_RESTREES=48`; verify 17,258 records, both
  `Streets` records and the 2 GB record.
- Day 5: `fear_holder_shim.h` (`define_interface`/`define_holder` overrides plus the v4
  `CInstanceHandleClient` adapter); force-include it or add it to the three module PCHs
  (`FEAR/ClientShellDLL/Stdafx.h`, `FEAR/ObjectDLL/Stdafx.h`, `FEAR/ClientFxDLL/stdafx.h`) and
  `sdk/inc/ltobjref.cpp`; compile-only ABI probe for `IClientShell`, `ILTClient`, `ILTCommon`,
  `IServerShell`, `ILTServer`.

Week 2:

- Day 6-7: `de_file.cpp` LTAR branch; `-archcfg`/`-rez` re-enabled; engine mounts all 36 archives.
- Day 8: `FearCShell` skeleton (v4 proxy + `Fear` v5 facade), `FearCres`/`FearSres` stubs, CMake
  isolation (`BUILD_NOLF2=OFF`, `OUT_FEAR`), and the `object.lto` target skeleton with
  `ObjectDLLSetup`.
- Day 9: compile the canary and check the registration manifest; `link_to_implementation` wired.
- Day 10: `fear_probe` dumps world 113, MODL 33 and DDS headers; write the schema note.

Acceptance for the two weeks: LTAR parser and tests pass; all 36 archives mount through the engine;
the canary `CShell.dll` builds with the `Default` v4 and `Fear` v5 registrations; the schema note is
written. The engine load gate (M1a) completes early in week 3, the renderer gate (M1b) and M3
go/no-go in week 4.

## 8. Open decisions

1. Holder shim mechanism: macro overrides in `fear_holder_shim.h` versus per-module direct pointer
   injection. The macro route is the default; the v4 `CInstanceHandleClient.Default` adapter must be
   proven in M1a.
2. World 113: extend the engine loader or convert offline (M3).
3. Model `MODL` 33 and the 13 LTB-like / 3 `LDOM` variants: one loader or per-variant handling (M3).
4. Whether `IDatabaseMgr` can run against the retail `GameDatabase.dll` through the facade or must
   be reimplemented (M5).
5. Archive override precedence: the engine is LIFO (last mounted wins), so `Default.archcfg` order
   would select the `FEAR_1.Arch00` copy of `Streets.World00p` (15,200,237, size 27,042,554) over
   the `FEAR.Arch00` copy (3,817,537,327). Verify in M6; reverse the mount order or add a
   precedence option if the base archive must win.
6. Whether the first map needs a material shim or a real `.Mat00` implementation (M6).
7. `object.lto` output name, `ObjectDLLSetup` export and `SERVEROBJ_VERSION` 1 layout; confirm
   against `cb_LoadModule` before M7.

## 9. Prototype results (2026-10-21)

A standalone v5 host was built outside the repository to test the vendor-DLL route before touching
the engine build.

- It compiles only against `C:\tools\fear-sdk\engine\sdk\inc` with the SDK's `ltmodule.cpp`,
  creates a v5 `CInterfaceDatabase`, loads `GameClient.dll` from `FEARE_8.Arch00`, calls its
  `SetMasterDatabase`, and holds `IClientShell.Default` v5.
- The base `FEARE.Arch00` DLL registers `IClientShell` 4 / `ILTClient` 104 and is not usable.
  `FEARE_5..8` register 5 / 105, matching the SDK 1.08 headers.
- With a file manager over a staging directory, the shell initialized the GameDatabase,
  StringEdit, NavMarker, SoundFilter, Weapon, Mission, FX and Sound databases. It then loaded
  ClientFX (a stub DLL was used), sound, cursor and message services.
- In the standalone prototype, `OnEngineInitialized` returned 1 after `CreateObject` and the
  process then crashed during module-database shutdown. Renderer, draw prim, texture manager,
  input, timer and object services were stubs at that stage; the integrated engine state is
  described below.
- Staging files, all read-only from the retail install: `GameClient.dll`, `GameDatabase.dll`,
  `StringEditRuntime.dll`, `LTMemory.dll`, `Config.Strdb00p`,
  `StringDatabase\FEAR.Strdb00p`, `Database\FEAR.Gamdb00p`,
  `DatabaseLocalized\FEARL.Gamdb00p`, and the ClientFX stub.
- The main menu is not yet reached. The remaining work is M5: real `ILTRenderer` (76 virtuals),
  `ILTDrawPrim` (23), `ILTTextureMgr` (9) over the engine's D3D9 device, plus `CreateObject`,
  input, timer and message semantics.

The prototype sources and staging data live under `%TEMP%\opencode\v5test` and
`%TEMP%\opencode\fear_stage`; they are not committed because they contain retail binaries.

The prototype is now integrated into the repository:

- `FEAR/Compat/v5host` builds `FearHost.dll` from the v5 facade and service stubs, compiled only
  against the FEAR SDK headers with its own `CInterfaceDatabase`.
- `FEAR/Compat/cshell` builds `CShell.dll` with the v4 proxy; it extracts `GameClient.dll` through
  the engine's `IClientFileMgr` (LTAR-aware), loads `FearHost.dll`, and forwards
  `OnEngineInitialized`.
- `FEAR/Compat/cres` and `sres` build the loader stubs.
- `cmake -S . -B build-fear -DBUILD_FEAR_PORT=ON -DFEAR_SDK_DIR=C:/tools/fear-sdk ...` builds
  `OUT_FEAR\CShell.dll`, `FearHost.dll`, `cres.dll`, `sres.dll` together with the engine.

Running `Lithtech.exe -fear -workingdir <staging> -archcfg Default.archcfg` loads the chain and
calls `OnEngineInitialized` inside the engine. The shell initializes all global managers and then
returns 1, after which the engine shuts down. The remaining blockers to the menu are the v5
`ILTFileMgr` over the engine's LTAR trees, the real `ILTRenderer`/`ILTDrawPrim`/`ILTTextureMgr`,
`CreateObject`, and input/timer semantics. `SendToServer` now returns success; `CPrint` forwards
shell messages to stdout.

### Integration state

- `FEAR/Compat/cshell` builds `CShell.dll` from the v4 proxy plus the file, object and renderer
  bridges (`cshell_filemgr.cpp`, `cshell_object.cpp`, `cshell_render.cpp`) and the DDS header
  parser (`dds_loader.cpp`); `FEAR/Compat/v5host` builds `FearHost.dll` from the v5 facade and
  service stubs. Both targets include the shared `FEAR/Compat/include` headers, and `CShell.dll`
  loads `FearHost.dll` at `OnEngineInitialized`.
- `FEAR/Compat/clientfx` builds the `clientfx.fxd` stub, so the ClientFX module no longer has to be
  supplied by hand. It exports `fxGetNum` (0), `fxGetRef` (zeroed `FX_REF`), `fxGetVersion` (1) and
  the no-op `fxInitDLLRuntime`, `fxTermDLLRuntime`, `fxDelete` and `fxFreePropList`.
- The engine loads `CShell.dll` and `CShell.dll` loads `FearHost.dll` at `OnEngineInitialized`; the
  v4 proxy forwards the engine callbacks and the v5 shell runs inside the engine process.
- The v5 file manager is backed by the engine's file manager and reads the LTAR archives, so the
  shell resolves and enumerates its content instead of failing on a missing file. Two paths are
  reached during startup: `Interface\menu\skins\screen_background_2.dds` and the `ClientFx\*.fx00p`
  scan.
- `tools/run_fear_port.ps1` reproduces the staging directory: 36 hardlinked `*.Arch00` archives in
  `Default.archcfg` order, `Default.archcfg`, the `OUT_FEAR` artifacts, `SDL2.dll`, the retail
  support DLLs, and `FEAR.Strdb00p`, `FEAR.Gamdb00p` and `FEARL.Gamdb00p` extracted from the
  archives with `ltar_dump`. The retail installation is only read, and the engine is not started
  while another `Lithtech` or `quakeray` process is running.
- The v5 renderer path now reaches the engine: `SetRenderMode` is forwarded to the v4
  `ILTClient` (previously it only stored the mode, leaving the D3D device uninitialized), and the
  CShell DDS decoder uploads the menu textures through `ILTTexInterface::CreateTextureFromData`
  (`Interface\menu\frame_n.dds` and the HUD textures decode and return valid handles).
- `ILTFileMgr` is backed by the engine's LTAR-aware `IClientFileMgr`; the shell enumerates
  `ClientFx\*.fx00p`, opens `Database\FEAR.Gamdb00p`, and reads the menu/HUD textures.
- `CreateObject` converts the v5 `ObjectCreateStruct` to the v4 structure and forwards it to the
  engine. `ILTTextureString`, the material instance, the resource allocator and the console
  variable table are implemented well enough for the shell to continue past their first checks.
- The shell now completes `OnEngineInitialized` and, with the prototype workarounds below, reaches
  the construction of the interface manager (`101A4500` becomes non-null). This is the furthest
  point reached so far.
- The current crash is in the interface manager constructor. The retail `GameClient.dll` and the
  SDK 1.08 headers disagree on some v5 method signatures: the shell calls `ILTSoundMgr::SetListener`
  with one pointer argument, while the SDK declares four arguments, so the callee's `ret 16`
  corrupted the stack and the next return went to `0x3f800000`. A one-argument vtable shim fixes
  that specific call, but the interface manager constructor exposes further signature/layout
  mismatches and eventually returns to the shell object address (`0x101A5490`).
- A second mismatch is visible in `ILTFileMgr`: the shell calls the interface slot at offset `0x0c`
  with a single path pointer and uses the returned stream, which matches an `OpenFile`-style
  method. The SDK places `OpenFile` at slot 2 and `ExtractFile` at slot 3; pointing the retail
  global at our `CFearFileMgr` therefore calls `ExtractFile` with one argument and faults inside
  the stub. The interface manager is constructed (`101A4500` becomes non-null) before this fault.
  The exact retail signatures must be reconstructed from the `GameClient.dll` call sites, or the
  matching retail 1.08 SDK headers must be used, before this path can be stable.
- The ClientFX managers (`RealTimeClientFXMgr`, `RenderTargetClientFXMgrs`, `DamageFXMgr`,
  `SimulationClientFXMgr`) require effect types from `clientfx.fxd`. The stub returns zero effect
  types, so their initialization fails; the retail `ClientFX.fxd` extracted from `FEARE.Arch00`
  terminates the process early when loaded by the current host. A stub with the effect types used
  by the menu, or the retail DLL once the v5 ABI is correct, is required.
- The prototype bypasses the `PerformanceDB`/`CharacterDisplayDB`/damage/ClientFX early returns.
  These are diagnostic crutches in `cshell_proxy.cpp`, not a supported configuration; they must be
  removed once the real signatures and services are in place. The former write to the retail
  `GameClient.dll` global at `0x101A44FC` was removed after `ILTClientContentTransfer` was
  registered.
- The main menu is still not displayed. The next milestone is a stable interface manager
  construction with correct v5 signatures, followed by the menu screen creation and the first
  rendered frame.

### Iteration 4 - RenderDoc, content transfer and sound ABI

- RenderDoc 1.46 does not support D3D9. DXVK 3.1.1 (Vulkan 1.4) captures fail to replay with
  `VK_ERROR_OUT_OF_DEVICE_MEMORY` even on a 16 GB RX 9070 XT. DXVK 2.7.1 (Vulkan 1.3) captures
  replay successfully in RenderDoc 1.46; the main menu capture is `fear_cap4_frame344.rdc`.
  The `GabrielMajeri/d3d9-to-11` project is archived (2018) and its README states it cannot run
  real games; it is not a viable alternative to DXVK.
- The unresolved holder at `0x101A44FC` was `ILTClientContentTransfer.Default`. Registering
  `CFearClientContentTransfer` makes `ClientConnectionMgr::SetContentTransferOptions` work; the
  manual global write is gone.
- The retail `ILTSoundMgr` vtable does not match the SDK layout. Retail slot 38 is called with one
  pointer (the SDK declares `SetListener` with four arguments), and slot 30 is called with no
  arguments (the SDK slot 30 is `SetSoundPosition` with three arguments). Both caused stack
  corruption visible as `0x3f800000`, `esi=1`, `eax=1.0f`. Prototype `__stdcall` shims at those
  slots keep the stack balanced; they are workarounds, not a correct implementation.
- `CInterfaceMgr::SwitchToScreen(SCREEN_ID_MAIN)` is reached and the menu assets load
  (`HudMenu_big`, `MenuLine_big`, menu sprites), but `CScreenMgr::SetCurrentScreen` finds no main
  screen (`GetScreenFromID` null), so `ChangeState` fails and no UI is drawn. The captured
  main-menu frame contains only the clear pass and DXVK's present blit; there are no D3D9 menu draw
  calls.
- `skiptitle=1` (with `host` and `runworld` absent) bypasses the black splash and reaches the main
  screen path. Other console variables keep the permissive defaults.
- The texture manager caches textures by filename with reference counts; `screen_background_2.dds`
  is created once instead of every frame. The DDS header confirms it is a 2x1024 32-bit strip,
  uploaded as 8x1024.
- The main menu is still black. The remaining defect is in the game-side screen manager/UI path and
  is consistent with further v5 ABI mismatches (a bool/float value used as a screen pointer). The
  correct fix is to rebuild against the exact retail 1.08 SDK headers or reconstruct the retail v5
  vtables; per-slot shims are temporary.


### Iteration 5 - source-built Public Tools client and the main menu

The shipped Public Tools runtime `GameClient.dll` (and the retail `GameClient.dll`) is compiled
against an internal SDK whose `ILTClient` and `ILTClientSoundMgr` layouts differ from the Public
Tools 1.08 headers the v5 host uses. The symptoms were stack corruption at sound slots, a wrong
`ILTClient` method at `ShutdownWithMessage`, and null `LTObjRef` handles. Rebuilding the client from
the Public Tools source against the same headers as `FearHost` removes the ABI mismatch by
construction.

- Build project: `FEAR/Compat/publictools_client`. It builds `GameClient.dll` from
  `FEAR Public Tools 1.08/Source/Game/ClientShellDLL`, `Game/Shared`,
  `Game/Libs/LTGUIMgr`, `libs/stdlith` and `libs/platform`, links the prebuilt
  `Shared_CRC.lib`/`Shared_Assert.lib`, and uses `_SILENCE_STDEXT_HASH_DEPRECATION_WARNINGS`,
  `/Zc:forScope-` and a forced prelude for the 2005-era code. The `__snprintf` CRT shim is in
  `crt_compat.cpp`.
- Build command:
  `powershell -File tools/build_fear_client.ps1 -PublicToolsDir "<...>\FEAR Public Tools 1.08"`.
  The project is standalone; the engine/`CShell.dll`/`FearHost.dll` still build with the normal
  `build-fear` configuration. `FEAR_PUBLIC_TOOLS_SOURCE` selects the installed Source directory.
- Two source patches are applied in the build directory, never to the installed SDK:
  `HUDChatInput.cpp` uses `L'\0'` instead of the empty wide character constant, and
  `ScreenMain.cpp` loads `Interface\menu\skins\screen_background_2.dds` as a dark menu background.
- Host fixes required for the client:
  - v5 `ObjectCreateStruct` types are translated to the v4 enum (`OT_LIGHT` 3->4, `OT_CAMERA`
    4->5, `OT_CONTAINER` 5->9, `OT_CUSTOMRENDER` 6->10); `CreateObject` retries with the v4
    `OT_NORMAL` object when a model/camera object cannot be created, which is enough for the
    player camera collision model at the menu.
  - `ILTClient::LinkObjRef` returns `LT_OK`; otherwise `LTObjRef` clears the camera handle.
  - `ILTTextureString` is implemented in the host with GDI: each string is rendered into a 32-bit
    DIB, converted to white/alpha RGBA, uploaded through
    `ILTTexInterface::CreateTextureFromData`, and drawn as a textured quad through the existing
    `ILTDrawPrim` bridge. This avoids the unsafe v4 texture-string interface.
  - Sound vtable shims are opt-in only: `FEAR_RETAIL_ABI` selects the full retail shim set and
    `FEAR_TOOLS_CLIENT` selects the one-argument `SetListener` shim for the shipped runtime client.
    The source-built client needs neither.
- Staging: `tools/run_fear_port.ps1 -GameClientDll build-fear-client\GameClient.dll` copies the
  source-built client into the stage. `-GameClientDir` remains for experiments with the shipped
  Public Tools runtime but is not the supported path.
- Verified result: the main menu ("Single player", "Multiplayer", "Options", "Profile", "Quit",
  help text and version) is rendered and the process stays alive for at least two minutes (120 s), responding, with no
  crash or assertion. The menu background is an approximate dark gradient; the original menu
  movie/3D scene and logo are not reproduced yet. Keyboard/mouse navigation and starting a game
  are not yet verified; `GameServer.dll` and the real `ClientFx.fxd` are not required for the menu.

Remaining work for the next iteration:

- Verify menu navigation (keyboard and mouse) and screen transitions.
- Reproduce the intended main-menu background (movie or the retail layout background) and logo.
- Verify the Options/Profile screens, which exercise more UI controls and texture-string paths.
- Supply `GameServer.dll`/the real ClientFX module and start a single-player level.
- Remove the remaining diagnostic logging from `CShell`/`FearHost` once the path is settled.
- Re-test the Release configuration after the Debug path is signed off.

### Iteration 6 - menu input, the loading screen and the world preview

Keyboard and mouse navigation now work in the source-built Public Tools client.

- Keyboard input continues to use the v4 `IClientShell::OnKeyDown`/`OnKeyUp` callbacks. The
  source client's `CInterfaceMgr` receives the engine's virtual-key codes; selection changes and
  help text update.
- The client's own Win32 window hook does not see the D3D proxy window used by the renderer.
  `FearClient_DisableWindowHook` removes it after `OnEngineInitialized`, and the v4 shell forwards
  SDL mouse events directly to the client through `FearClient_MouseMove`/`FearClient_MouseButton`.
  SDL runs in relative mouse mode, so `CFearShellProxy::HandleEvent` accumulates `xrel`/`yrel`
  into a virtual 640x480 cursor and sends clicks at that position. Mouse hover and single-click
  activation work; the Options screen opens from the main menu.
- The mission database was loading, but `GetConsoleString("MissionOrder", "MissionOrder")`
  returned an empty string because the v5 console facade auto-created unknown variables with an
  empty value. `GetConsoleVariable` now only returns variables that already exist, so absent
  console overrides fall back to their defaults. `StartGameNew` resolves `Worlds\Release\Intro`
  and enters the pre-load/loading state.
- Staging now extracts `Database\FEAR.Gamdb00p`, `DatabaseLocalized\FEARL.Gamdb00p` and
  `StringDatabase\FEAR.Strdb00p` from the last archive that contains them, matching the engine's
  archive search order.
- `ILTClient::LoadingProgress` now returns a small `ILTLoadingProgress` implementation instead of
  null, which removes the `CLoadingScreen::Init` null dereference. The loading screen for
  "Interval 01 - Inception" renders with its mission briefing and progress bar.
- `ILTClient::StartGame` returns `LT_OK` and marks the local client connected so the loading
  state can be entered. There is no v5 server behind it yet.
- The v113 world renderer is in `FEAR/Compat/cshell/fear_world.cpp`. It reads the render section
  of `Worlds\Release\Intro.World00p` (2,268 surfaces, 508 materials), decodes the per-surface
  vertex blocks and triangle indices, uploads one static D3D9 vertex buffer and draws the level
  from the `GameStartPoint` position (-1100, -1436, -370) with a 69-degree view. The first
  parse reports 196,229 triangles and world bounds (-29300, -2236, -18803) to
  (36484, 2094, 22780).
- `FearClient_EnterWorldPreview` calls `OnEnterWorld`, hides the loading screen and switches the
  interface to `GS_PLAYING`; `FearClient_RenderWorldPreview` draws the world every frame. After
  the preview starts, `FearHost_Update` stops running the normal client update, so this is a
  deterministic M6 preview, not a playable session. The physics simulation is a stub with the
  complete `ILTPhysicsSim` vtable so `ClientPhysicsCollisionMgr::PostStartWorld` no longer
  dereferences null.
- The preview camera is free-flying: arrow keys move, Space/Ctrl move vertically and the mouse
  looks around. There is no collision, gravity, player object or HUD yet.
- The rendered preview is untextured and unlit: material names are used only to generate flat
  placeholder colours. Model loading, BSP collision, HUD/player state and game logic are still
  absent.

Verified commands:

```
powershell -File tools/build_fear_client.ps1 -PublicToolsDir "<...>\FEAR Public Tools 1.08"
powershell -File tools/run_fear_port.ps1 -RetailDir "<...>" -GameClientDll "build-fear-client\GameClient.dll" -NoRun
run-fear-engine\Lithtech.exe -fear -workingdir "<stage>" -archcfg Default.archcfg
```

Remaining work for the next iteration:

- Implement the v5 local server/world path (`IServerShell.Fear`, `ILTServer.Fear`, object loading)
  and connect `StartGame`/`SendToServer` to it.
- Add a real player/camera, movement, collision, HUD and game-object updates; the current preview
  deliberately freezes the shell after entering the world.
- Add materials/textures and lighting to the v113 world renderer.
- Continue with the first playable level after the M6/M7 gates.


### Iteration 7 - retail v5 server bootstrap and local world load

The v5 server path is now proven with the retail engine binaries. This is a bootstrap for the
next milestone, not the final source-only server.

- `FEAR/Compat/v5host/fear_server_host.cpp` loads `EngineServer.dll` from the stage, creates the
  v5 `ServerInterface` (`SI_VERSION` 2), initializes it, adds the 36 LTAR archives, loads
  `GameServer.dll`, and starts `Worlds\Release\Intro.World00p`.
- Two non-obvious requirements were established experimentally and in IDA:
  - `ServerInterface::AddResources` must receive the archives first and the stage directory last.
    The last resource wins in `LoadServerObjects`; the reverse order makes `LoadBinaries` fail.
  - `NetGameInfo` is an MSVC 7.1 object: six 28-byte `std::string` fields, then three `bool`s and
    the `EServerStartResult` enum (176 bytes total). The engine expects a `NetGameInfo**` with
    length 4. The host constructs the strings with the exported `MSVCP71.dll` constructors and
    assigns `SinglePlayer`, profile `Default`, and mission order `MissionOrder`.
- The shipped `FEARServer.exe`/`EngineServer.dll` reports `IsMultiplayerExe() == true`. For the
  single-player server shell the host patches the three-byte implementation at
  `EngineServer.dll + 0x42F10` (`mov al,1; ret` to `xor al,al; ret`) after validating the bytes.
  This offset is specific to the 1.08.282 build.
- Verified in the standalone probe:
  - `CreateServer` -> `SI_OK`;
  - `LoadBinaries` -> 1;
  - `StartWorld` -> 1;
  - console output: `Loading world: Worlds\Release\Intro.World00p`, `World loaded.`;
  - no "Server is missing class" messages with the retail `GameServer.dll`.
- `StartGame` in the v5 client facade now launches this server. `FearHost_Update` pumps
  `ServerInterface::Update`. `StartGameNew` can be triggered by the `FEAR_AUTOSTART=1`
  environment variable through a debug export, so menu automation is not required.
- The client reaches the loading screen and then the M6 world preview. The server and the client
  are still not connected: no client handle is registered with the server network manager and no
  server-to-client object replication exists.
- An experimental `FEAR_LOCAL_CLIENT=1` path calls `IServerShell::OnAddClient`/
  `OnClientEnterWorld` with a synthetic handle and captures `ILTMessage_Read` messages. It reaches
  player creation, but the engine dereferences the synthetic client in further client methods and
  faults. It is disabled by default; the hooks and call sites remain for the next iteration.
- Staging now copies `EngineServer.dll` from the retail root and extracts `GameServer.dll` from
  the last archive that contains it (`FEARE_8.Arch00`). `tools/run_fear_port.ps1` was updated
  accordingly.
- `tools/run_on_desktop.ps1` creates or opens a Win32 desktop named `fear` and starts a process
  with `STARTUPINFO.lpDesktop = "winsta0\fear"`. The game was run there with
  `-Switch -SwitchBackSeconds 30`; auto-start, server world load and the preview all rendered
  without a crash while the desktop was active. A non-visible desktop is not sufficient for D3D
  initialization; the desktop must be switched to for a visible run.

Verified commands:

```
powershell -File tools/build_fear_client.ps1 -PublicToolsDir "<...>\FEAR Public Tools 1.08"
powershell -File tools/run_fear_port.ps1 -RetailDir "<...>" -GameClientDll "build-fear-client\GameClient.dll" -NoRun
powershell -File tools/run_on_desktop.ps1 -FilePath "<stage>\Lithtech.exe" -ArgumentList "-fear -workingdir \"<stage>\" -archcfg Default.archcfg" -WorkingDirectory "<stage>" -DesktopName fear -Switch -SwitchBackSeconds 30
```

Remaining work for the next iteration:

- Replace the synthetic client with a real local client connection, or implement a source-side
  `ILTServer`/`IServerShell` transport. The first step is to capture the server messages after
  `OnClientEnterWorld` and deliver them to the v5 client shell in order.
- Feed `MID_*` messages through `IClientShell::OnMessage`, create the client-side player/camera,
  and stop using the static M6 preview once the server player exists.
- Implement object replication, movement, collision, HUD and weapon state.
- Revisit the `IsMultiplayerExe` patch: a source-side server should expose the single-player flag
  without modifying a retail DLL.
- Keep the retail `EngineServer.dll`/`GameServer.dll` path clearly marked as a bootstrap; the
  final port must not depend on stub-only `ObjectDLLSetup` classes.

### Iteration 8 - v5 client/server session and first mission state

The retail v5 server is now driven as a real in-process session rather than a
synthetic callback probe.

- `FearServerHost_Start` creates the server and loads the binaries without
  starting a world. `StartGame` only starts the session; the client's
  `MID_START_LEVEL` (185) is what makes `ServerMissionMgr` load
  `Worlds\Release\Intro`.
- The server client is a real-shaped 0x60-byte engine client record with
  `GetClientID` (cdecl), `GetClientName`, `SetClientName`, and
  `FinishClientConnect` left intact. Only `SendToClient` (vtable 94) and
  `GetClientAddr` (vtable 105) are replaced.
- After the world-loaded notification, the host calls `sub_10041750` to create
  the player through the engine's normal `OnClientEnterWorld` path and stores
  the returned object in the client record's `+0x5C` field.
- Client messages are deep-copied into a stable read object and pumped through
  `IServerShell::OnMessage` (slot 10). Server messages are deep-copied before
  delivery so the retail auto-message buffer cannot be reused under the client.
- `ILTClient::GetGameMode` now reports `STARTGAME_NORMAL` while the server is
  running. This was required for `PostScreenState` to allow the transition from
  the postload screen to `GS_PLAYING`.
- The client sends `MID_CLIENTCONNECTION` Loading and InWorld, receives player
  state messages, and reaches `GS_PLAYING` with `recv=12`, `sent=12`, player
  state `ePlayerState_Alive`, and `GetCurrentWorldName()` equal to
  `Worlds\Release\Intro`.
- A save-data request during the level switch is answered by the client bridge
  instead of being handed to the unimplemented client save path.
- The postload screen is continued with a synthetic space key when it is
  visible, which lets the auto-start session reach the playing state.

Verified run (Debug, 20 seconds, no crash):

```
FEARSERVER: Loading world: Worlds\Release\Intro
FEARSERVER: World loaded.
FEARSERVER: player connected=...
FEARPORT: PlayerStateChange value=5
FEARPORT: PlayerStateChange value=1
FEARPORT: ChangeState 6 -> 1 screen=39
FEARPORT: clientstate game=1 recv=12 sent=12 player=1 world='Worlds\Release\Intro'
```

The native D3D9 renderer on this machine currently returns
`D3DERR_DEVICELOST` from `CreateDevice` even for a reference device, so the
verified run above uses DXVK 2.7.1 `d3d9.dll` in the staging directory only.
DXVK is a local test workaround, not a repository dependency and not part of
the final runtime.

Remaining blockers for the stated goal:

- The renderer is still the flat v113 preview. The player camera and a real
  v5 `RenderCamera` path are not yet wired, so the visible frame is not the
  retail mission frame.
- HUD/weapon/object-dependent messages (103, 126, 129, 145, 212, 231, 229,
  233, 230, 138) are filtered until database records and server object handles
  are mapped to the client. Movement, collision, weapons and HUD therefore do
  not yet work.
- `ILTModel`, `ILTCustomRender`, material instances, `.Mat00`/`.fxo`, DDS mip
  chains, lights and shadows are still the next renderer milestone.
- The `IsMultiplayerExe` and internal EngineServer offsets remain a retail
  bootstrap dependency until the source-side server is implemented.

### Iteration 9 - player camera, textured world and HUD bridge

The first mission now renders from the server player camera instead of the
fixed preview camera.

- The server host exposes the engine player object's position, rotation and
  object id through the recovered `sub_1004A780` / `sub_1004A7F0` accessors.
- The host maintains a small client object registry: server object ids are
  resolved to engine object handles, transforms are stored in the registry,
  and `ILTClient` transform getters/setters operate on those records. Locally
  created camera and effect objects are registered as client-only objects.
- The host maps the server player id to a client target. After the client
  reaches `GS_PLAYING`, `FearClient_PreparePortPlayer` attaches
  `CPlayerCamera` to that target and switches it to first person. The camera
  is updated and rendered every frame through the real v5 game code.
- The host renders `CPlayerCamera` and draws `CInterfaceMgr` into the same
  frame. This brings the first-person camera, the crosshair/health/armor HUD
  and the real camera transform into the port for the first time.
- The v113 renderer now parses the world material table, reads each `.Mat00`
  header (LTMI v1), extracts the first DDS texture parameter, decodes the DDS
  data and uploads a managed D3D9 texture per used material. Surfaces are
  batched by material with UVs decoded from the per-stride vertex layouts.
- Verified current parse: 2,268 surfaces, 1,979 visible surfaces, 289 excluded
  shadow-volume surfaces, 508 materials, 457 used materials, 58,355 visible
  triangles and 455 loaded material textures.
- `ILTRenderer::MakeScreenShot` and the CShell render API now capture the
  actual D3D9 back buffer/front buffer to a 32-bit BMP. `FEAR_CAPTURE_PATH`
  captures the first mission frame without relying on a desktop screenshot.
- A captured port frame shows the textured first-person view with HUD at
  640x480. Lighting, lightmaps, `.fxo` shaders, models, weapon view models and
  shadow volumes remain unimplemented, so the frame is still far from the 95%
  visual parity gate.

Remaining blockers for the stated goal:

- The camera target is a registry object driven by the server transform. The
  local `CCharacterFX`/move-manager path is not initialized, so
  `GetClientObject` remains null and the client does not yet receive the full
  local-player movement and weapon state.
- HUD data is only partially populated because the player info and interface
  messages are still filtered. Health, armor, ammo and weapon values need the
  database/object mapping work.
- The world is textured but unlit. Lightmaps, surface lighting, `.fxo`
  materials, normal/tangent data, bump maps, fog, shadows and post-processing
  are the next visual milestone.
- `ILTModel`, `ILTCustomRender`, `ILTPhysicsSim` and most object services are
  still stubs or incomplete; models, animation and object replication are not
  present.
- The `IsMultiplayerExe` patch and version-specific EngineServer offsets remain
  a retail bootstrap dependency until the source-side server is implemented.

## Iteration 10 status

Completed since iteration 9:

- Local-player path: `CPlayerCamera::InitLocalPlayer` runs end to end. The host
  issues the `SFX_CHARACTER_ID` factory request with
  `hModel = ModelsDB::GetModelByRecordName("Player")` (matching
  `PlayerBodyMgr::ResetModel`), the guard scope lets the character FX factory
  resolve the client object, and the port camera is prepared with
  `kCM_FirstPerson` on the server player target.
  Evidence: `local character modelsdb=... model=...`,
  `local character fx created for ...`, `port camera ready ... localplayer=1`.
- Camera: the transform is read from the player object instead of the preview
  defaults, and the world camera follows it every frame.
- Frame capture: `ILTRenderer::MakeScreenShot` renders the world into an
  offscreen render target, copies it to a system surface and writes a 32-bit
  BMP. The back-buffer readback path cannot be used because the v4 host does
  not expose a readable back buffer. `FEAR_CAPTURE_FRAME` selects the frame.
- Log volume: per-message and per-frame logging is rate limited, which removes
  the ~170k line log storms that were being produced by the server message and
  render loops.
- `tools/parity_check.py` implements the acceptance metrics (channel
  tolerance percentage, MAE, luma SSIM, CIEDE2000) and self-validates the
  colour math, so parity numbers are now reproducible.

Known defect carried forward:

- `Critical error, no Null weightset on Character!  Missing: null`. The
  character FX object is created and attached, but `MODL` parsing is still
  stubbed, so the animation sets have no data. Either real `MODL` version 33
  parsing or a guarded `InitAnimations` is required before models can render.

Next order of work:

1. Obtain a retail reference frame for the same mission, camera and settings,
   then run `tools/parity_check.py` to replace estimates with a measured gap.
2. Locate the static lighting data. The `World00p` header exposes render,
   light-grid, property/object and particle-blocker positions; the BSP and
   lightmap payload lives in the first ~10 MB of the file, before the render
   block. The last 92 KB of the render block is an index-ordered record table
   (36-byte stride) whose role is still unidentified.
3. `MODL` version 33 for characters, weapons and world props.
4. `.fxo` material shaders, then fog and post-processing.
5. Message routing: deliver `MID_SFX_MESSAGE` sub messages through
   `IClientShell::SpecialEffectNotify` instead of filtering them, keeping the
   synthetic character creation as a fallback.


## Iteration 11 status

Camera projection now uses the client's own FOV instead of a constant.

- `ILTClient::SetCameraFOV` / `GetCameraFOV` were inert stubs. They now store the
  values and forward them through a new `FearRenderApi::SetCameraFov` entry to
  `FearWorld_SetFov`.
- Verified in `fear_port_run.log` on the first mission:
  `FEARWORLD: camera fov x=1.5936 rad (91.3 deg) y=1.3090 rad (75.0 deg)`.
  These are the values `PlayerCamera` computes for 1024x768, not constants.
- The renderer derives the aspect ratio as `tan(fovX / 2) / tan(fovY / 2)` and
  falls back to 4:3 only when the client has not set a FOV yet. The previous
  code hard-coded `1.2 rad` (68.8 deg) vertical with a 4:3 aspect, so the field
  of view was visibly narrower than retail.
- Capture re-verified after the change: 1024x768, mean luma 82.4, mean 32x32
  block standard deviation 22.1, 98.4% of pixels non-black, no new errors in the
  log. The only remaining diagnostic is the known `no Null weightset` message.
- Measured against the previous capture (`port_materials.bmp`) the framing change
  scores match 11.74%, MAE 75.77, SSIM 0.156, CIEDE2000 mean 28.90 / p95 56.03.
  Those numbers only quantify the FOV delta; the parity gap to retail still needs
  a retail reference frame.

Remaining blocker for the next visible step is unchanged: the lighting payload.
`LightPointFill` (5) and `LightCube` (3) objects exist in the object block, but
a scan of the whole block recovers only 142 objects out of a 2.5 MB section, so
the object stream framing is still not decoded well enough to drive lighting
from it. The next attempt should read the object stream through the retail
server, which already parses it, rather than continuing byte-level guessing.

## Iteration 12 status

The object-stream blocker above is resolved. The framing was decoded from an
independent Jupiter EX implementation (`LithtechLokiResearch`), not by guessing:

- For FEAR (world version 113) the eight world-model counters are XOR-ed with
  399 after reading. That single detail is what defeated every byte-level
  attempt; it also explains why the earlier scan stalled at 142 objects.
- Object record: `uint16`-prefixed class name, `int32 propertyCount`,
  `int32 blobLength` plus blob, then per property `int32 nameOffset`,
  `int32 type` and exactly four payload bytes. `String`/`Text`/`CommandString`
  payloads index a blob string; `Vector3`, `Color` and `Rotation` index blob
  floats (3/3/4); `Float`, `Bool` and `Int` are the four bytes inline.

The C++ parser now agrees with an independent Python reference on
`Worlds\Release\Intro.World00p`: 1988 of 1988 objects, `LightPoint=35`,
`LightSpot=12`, `LightDirectional=2`, `LightCube=39`, `max_radius=3000`,
`AmbientLight=(25,25,25)` and `GameStartPoint00=(-1100,-1436,-370)` — the exact
position the retail server spawns the player at. The lights and world properties
are stored but not yet consumed by `FearWorld_Render`.

Vertex decoding is now table-driven. The file declares 11 vertex types; each is
expanded into `(stride, uvOffset, normalOffset, colorOffset)` by accumulating
property formats in declaration order, and each surface binds through its
`vertexType` field. The derived strides match the recorded per-surface
`vertex_size` for 2267 of 2268 surfaces. 2096 surfaces have normals and 250 have
baked RGBA colour; both are uploaded, but `D3DRS_LIGHTING` stays off, because
F.E.A.R. world shading comes from material lightmaps rather than vertex colour
and enabling fixed-function lighting naively darkens the frame from mean luma
77 to under 10.

Face culling is now measured rather than assumed: `D3DCULL_CW` collapses mean
luma from 76.9 to 37.7 and loses a tenth of the frame, while `D3DCULL_CCW` keeps
the scene and only removes back faces, so `CCW` is the default and
`FEAR_CULL=none|cw` is the override.

Next: feed `g_WorldLights` into the renderer together with the material lightmap
stage, which is the missing term — geometry, projection, culling and the object
layer are now all decoded from the real data.

## Iteration 13 status

The material question is now answered from the data instead of assumed, and the
answer removes a large unknown from the lighting plan.

`FEAR_MATDUMP=1` dumps every parameter of the first twelve world materials. The
`.Mat00` layout in `FEAR/Compat/cshell/fear_world.cpp` was confirmed correct
end-to-end: magic `LTMI`, version, `uint16`-prefixed shader path, `uint32`
parameter count, then typed parameters. The shader path is a real quirk asset,
for example `Shaders\rigid\Prototype\specular_mirror.fx`.

A typical world material carries no lightmap stage at all:

```
Materials\Tech\Research\Tile_Mirror01.Mat00
  shader = Shaders\rigid\Prototype\specular_mirror.fx
  tDiffuseMap          = Materials\Tech\Research\Tile_DarkMirror_D.dds
  tEmissiveMap         = Tex\specular_black.dds
  tSpecularMap         = Tex\Office\Floor01_S.dds
  tNormalMap           = Tex\Office\Floor01_N.dds
  tReflectionMap       = Tex\specular_black.dds
  tReflectionMapMask   = Materials\Tech\Research\Tile_Dark_S.dds
```

F.E.A.R. world shading is therefore a dynamic per-pixel lighting model driven by
the `.fx` shaders and the scene light list, not a baked lightmap stage inside
the material. That is consistent with the earlier measurements: only 250 of 2268
surfaces carry baked vertex colour, and enabling fixed-function lighting just
turns the frame black. Two consequences for the port:

* `g_WorldLights` (35 point, 12 spot, 2 directional, 39 cube) is the real
  lighting input and must eventually be evaluated per pixel by a shader.
* The material texture must be chosen by name. The former heuristic took the
  first `.dds` value in the parameter list, which happens to be `tDiffuseMap`
  for every sampled material but is not guaranteed. `FearReadMaterialTexture`
  now prefers `tDiffuseMap` explicitly and only falls back to the first `.dds`
  parameter when it is absent.

Verification of the change: `surfaces=2268 materials=508 used_materials=457`
unchanged, and the captured frame is unchanged at `mean=77.1 std=64.4
blockstd=59.7 nonblack=97.5% colors=65893`, confirming the heuristic was already
picking the right map for this world.

### Light baking is measured, and it is not the missing term

`FEAR_LIGHTS=1` enables a CPU bake that evaluates every decoded light against the
decoded vertex normal and writes the result into the vertex diffuse colour
(`ambient + sum(colour * max(0, N.L) * (1 - d/r)^2)`, clamped). It runs over all
88 lights and 175 065 vertices at load time.

Measured result: `mean=16.2 std=6.8 blockstd=5.5 nonblack=97.4% colors=10285`.
The frame collapses to near black. The reason is structural, not a bug: the world
ambient is only `(25,25,25)` out of 255, the point/spot lights have radii on the
order of 300 inside level bounds that span roughly `65000 x 4300 x 41500` units,
and the two directional lights are the ones that would carry the broad fill. So
the light list alone cannot stand in for the `.fx` lighting model.

The bake therefore ships **off by default** and behind `FEAR_LIGHTS=1`, so the
measured `mean=77.1 colors=65893` baseline is preserved exactly. The remaining
work is a real pixel shader that reproduces the prototype shading path
(diffuse x normal-mapped specular lighting x emissive x reflection mask); the
data it needs — positions, normals, tangents, UVs, material maps and the light
list — is now all decoded.

## Iteration 14 status

The per-pixel world shader is implemented and now matches the fixed-function
reference within measurement noise, while adding the per-pixel terms
fixed-function cannot express.

### Two defects blocked the first attempts

**Constant register collision.** D3D9 HLSL assigns constant registers in
declaration order, and a shader *file* shares one assignment scheme across the
vertex and pixel entry points even though the two register spaces are
independent. Once the pixel-side light arrays were pinned to `register(c4)`,
`register(c12)`, `register(c20)` and `register(c28)`, the vertex matrix
`g_WorldViewProj` was allocated at **c36-c39** while the CPU uploaded it to c0.
Every vertex was transformed by garbage, the shader drew nothing, and the frame
fell back to the clear colour. The fix is explicit pinning on both sides:

```
row_major float4x4 g_WorldViewProj : register(c0);
```

Verified by compiling with `fxc /T vs_3_0 /E FearVS /Fc out.asm` and reading the
live range, which now reports `g_WorldViewProj c0 4`. Any shared uniform in a
D3D9 HLSL file must be pinned explicitly.

**`FearEnvFloat` discarded zero.** The helper returned the default whenever the
parsed value was not strictly positive, so `FEAR_NORMALMAP=0` silently kept
normal mapping on and the A/B test produced byte-identical frames. It now
returns the parsed value whenever the variable is present and non-empty.

### The actual lighting model

The decisive measurement is that `diffuse_texture * vertex_colour` reproduces the
fixed-function reference almost exactly: mean ratio `1.011` at the fiftieth
percentile of a 16x16 block grid, with no exposure term at all. The vertex colour
already contains the engine's own baked shading — `FearShadeVertex` accumulates
ambient plus every decoded light (`ambient + sum(colour * max(0, N.L) * (1 - d/r)^2)`,
clamped) across all 88 lights.

A shader that recomputes lighting from only the 8 lights selected per batch
therefore cannot reproduce that result, and it is the reason the first working
version was simultaneously too dark in the midtones and too bright near lamps.
The shader now uses the baked vertex colour as the diffuse term and adds only
what fixed-function cannot do per pixel:

```
albedo = tex2D(g_Diffuse, uv).rgb * lerp(1, i.Color.rgb, g_Tint.a)
N      = cotangent-frame perturbed normal (tangent-space normal map)
light  = ambient + sum(select_lights)          // 8 nearest of 88
spec   = sum(lc * pow(saturate(N.H), 24)) * g_Params.w
colour = albedo * lerp(light, 1, g_Tint.a) + spec * specularMap + emissive
colour = ACES(colour * exposure)
```

`g_Tint.a` is the `FEAR_VCOL` blend: `1.0` uses the baked vertex lighting for the
diffuse term, `0.0` uses the recomputed per-pixel lights. The default is `1.0`
because that is the measured-correct path; the per-pixel lights still drive the
specular term and the normal-map perturbation in both modes.

Tone mapping is the ACES approximation `c * (2.51c + 0.03) / (c * (2.43c + 0.59) + 0.14)`.
Linear exposure was the wrong tool: raising it to reach the reference midtones
pushed 12% of pixels above 250/255, because a linear gain cannot lift the
midtones without clipping the highlights. ACES keeps the whole frame inside the
range instead.

### Measured result

Compared against the fixed-function reference (`ref_ff.bmp`, frame 60,
1024x768) with a per-16x16-block brightness-ratio distribution:

| Variant | mean | std | colours | corr | ratio p10 / p50 / p90 | clipped |
|---|---|---|---|---|---|---|
| fixed-function reference | 76.83 | 64.4 | 65893 | - | - | 0.38% |
| linear exposure, no tone curve | 47.7 | 41.0 | 41493 | 0.7267 | 0.392 / 0.412 / 1.050 | 0.07% |
| linear exposure 2.6 | 96.1 | 85.9 | 83579 | 0.8986 | - | 12.38% |
| **ACES, exposure 0.72** | **77.15** | **59.7** | **63180** | **0.9849** | **0.917 / 0.993 / 1.080** | **0.00%** |

The shader frame is 0.5% brighter than the reference, the block-ratio
distribution is centred on `0.993` with a 12% p10-p90 spread, and nothing
clips. The remaining `std` difference (`59.7` against `64.4`) is the ACES
shoulder compressing the brightest region.

### Normal maps and specular are subtle by content, not by bug

`FEAR_NMDUMP=1` now reports each decoded texture's dimensions, DDS FourCC and
per-channel mean and standard deviation. The world's normal maps really are
nearly flat: `Tex\Office\Floor01_N.dds` is `(127.0, 127.0, 253.9)` with a
standard deviation of `5.0` on the two tangent axes, i.e. the surfaces are
architectural planes whose normals tilt by about two degrees. Disabling normal
mapping changes 73 of 63180 unique colours and no measurable statistic, which is
the expected result for that content rather than a dead code path. The path is
verified working: the textures load (397 of 457 used batches have one), decode
to valid tangent-space values near `(0.5, 0.5, 1.0)`, and the cotangent frame is
built from `ddx`/`ddy`.

Specular is measurable but small at this camera: `FEAR_SPECULAR=0` lowers the
mean from `77.15` to `76.61`. Most world materials point at
`Tex\specular_black.dds`, so the term only appears on the subset with a real
specular map.

### Diagnostic flags

The bisection scaffolding (`FEAR_MINIMAL`, `FEAR_VSDBG`, `FEAR_NOPS`,
`FEAR_MTX`, `FEAR_PROBE`) was removed once the root cause was understood.
The remaining knobs are the ones that matter for tuning and asset inspection:
`FEAR_SHADER=0`, `FEAR_SHADERDBG=1`, `FEAR_AMBIENT`, `FEAR_EXPOSURE`,
`FEAR_SPECULAR`, `FEAR_NORMALMAP`, `FEAR_VCOL`, `FEAR_TONE`, `FEAR_CULL`,
`FEAR_MATDUMP`, `FEAR_LIGHTDUMP`, `FEAR_LIGHTS` and `FEAR_NMDUMP`.

## Iteration 15 status

Iteration 15 adds the model pass: `FEAR/Compat/cshell/fear_model.cpp` decodes
MODL containers, uploads them into a D3D9 vertex and index buffer, and draws
them with a dedicated vertex and pixel shader from inside `FearWorld_Render`.

### The MODL container

`modl.py` and the C++ loader now agree on the layout. The geometry block starts
with an eight byte header holding `vertexBytes` and `faceBytes`; the vertex
records follow at `+8` and the faces immediately after them. Every F.E.A.R.
model found so far uses a 32 byte vertex record:

| Offset | Type | Meaning |
|---|---|---|
| 0 | `float3` | position |
| 12 | `float3` | normal (unit length) |
| 24 | `u32` | per-corner flag, `0` or `1` |
| 28 | `u32` | constant per file (`0x00F6F61C` on the submachine gun) |

The two trailing words were measured over all record counts, not sampled. On
`SubMachineGun` the word at `+28` holds `0x00F6F61C` in **all 6396** records,
so it is a file-level id or stamp and never per-corner data. The word at `+24`
takes only two values: `0` in 6288 records and `1` in 108, which is a flag —
most plausibly "this corner belongs to the second UV/material split".

**There is no texture coordinate stream.** A statistical sweep of the whole
file for float pairs inside `[0,1]` finds an unbroken run of `2 * numVerts`
floats immediately after the vertex block, but that run is the 16 bit face
index array read as float32: any index below about 2^30 is a denormal and
therefore lands inside `[0,1]`. Excluding that, the submachine gun, the helmet
and the spectral girl all have zero UV data anywhere in the container. The
`+24` flag being `0` or `1` supports the same reading: LithTech stores the
texture coordinates in the mesh's vertex format, and this decoder only reaches
the position and normal portion of it. Material assignment and UV recovery are
therefore a separate piece of reverse engineering and are **not** required for
the geometry to draw.

Character records are a caveat. The stride is inferred as `vertexBytes /
numVerts`, guided by the largest face index, which yields 64 on `SpectralGirl`
with zero slack — so 64 is almost certainly the real record size. But offset
12 inside that record is *not* a normal: it has a length of about 40 on that
file and is bit-identical to the position of the following record. The
character profile therefore carries extra `vec3` rows between the position and
the next corner, and only the position at offset 0 is currently understood.

This does not corrupt the render, because `FearModelBuildGeometry` measures the
mean squared length of the stored normals and falls back to face-derived
normals whenever it is outside `0.9..1.1`. A profile mismatch degrades into
recomputed normals instead of a broken mesh — do not "fix" the stride before
the extra rows are decoded, or the slack check that keeps the decode honest
will start picking the wrong stride.

The face array is a plain list of 16 bit index triples, but three of every four
triangles are geometrically degenerate: on the submachine gun 6396 of 8528
triangles have two coincident corners. The decoder therefore drops any triangle
whose cross product length is below `1e-5` at load time. On `SubMachineGun` this
reduces 8528 triangles to 2132 and changes the rendered frame by exactly zero
pixels, so the filter is a pure optimisation and not a topology change. The
2132 surviving triangles form a correct submachine gun silhouette when rendered
offline in an orthographic side view.

### The shader path and the `row_major` trap

The model pass runs a vertex and pixel shader pair compiled at runtime with
`d3dcompiler_47.dll`. Lighting is evaluated entirely in the vertex shader
because `ps_3_0` addresses only 224 constant registers (`c0`-`c223`) while
`vs_3_0` has 256, so the world shader's `c0`-`c36` block and the model's
`c240`-`c245` block can only coexist if the model constants stay out of the
pixel shader.

The single defect that cost the most time in this iteration was the matrix
declaration. The world shader declares its matrix as
`row_major float4x4 g_WorldViewProj`; the model shader originally declared
`float4x4 g_WVP` without the qualifier. HLSL then treats the matrix as
**column major**, which silently transposes a matrix uploaded with
`SetVertexShaderConstantF` and destroys the `w` component, so the model
projected to a full height vertical band instead of a gun. The symptom looks
like broken geometry, so it is worth remembering: an uploaded matrix that
produces absurd screen coverage is almost always transposed, not miscomputed.

### State hygiene

`FearModel_RenderQueued` saves and restores render states, world/view/projection
transforms, FVF, vertex declaration, shaders, texture stage states for all four
stages and the stage 0 texture, then rebinds every one of them. The pass was
validated with `FEAR_MODEL_NODRAW=1`, which performs every state change but
skips the draw call: the resulting frame is bit identical to a capture with no
model at all, which proves the save and restore are complete. Texture stages 2
and 3 were the last leak - stages 0 and 1 were handled from the start but the
world shader uses all four.

### Measured result

| Capture | corr vs `ref_ff.bmp` | changed px | bbox |
|---|---|---|---|
| no model (baseline) | 0.9849 | 0 | - |
| `SubMachineGun`, depth off | 0.9640 | 32566 | 332-691 x 281-487 |
| `SubMachineGun`, no draw | 0.9849 | 0 | - |
| `bomb`, depth off | 0.9386 | 45787 | 346-678 x 300-450 |
| `helmet`, depth off | 0.9199 | 73470 | 371-653 x 213-555 |
| `SpectralGirl`, depth off | 0.9808 | - | narrow vertical |

The predicted screen box from the CPU side transform (`ndcbox` in the
`FEARMODEL: draw` log line) matches the observed pixel box to within one
16x16 block, so placement, projection and aspect handling are correct. The
debug model is occluded by the world when depth testing is on, which is
expected: it is parked 170 units in front of the camera and the intro world has
geometry in between. `FEAR_MODEL_ZTEST=0` draws it through the world for
inspection.

The model is flat grey because the material and texture chain is not wired up
yet. `g_Tint` in `c245` supplies a neutral albedo and the vertex shader applies
a directional light with a 0.25 wrap term, which gives a luma spread of 40 to
170 across the silhouette. Real `.dtx` materials are a later iteration.

### Diagnostic flags

`FEAR_DEBUG_MODEL` (base name), `FEAR_MODEL_DIR`, `FEAR_MODEL_DIST`,
`FEAR_MODEL_SIZE`, `FEAR_MODEL_YAW`, `FEAR_MODEL_POINTS`, `FEAR_MODEL_TRIS`,
`FEAR_MODEL_ZTEST`, `FEAR_MODEL_FF`, `FEAR_MODEL_DEBUGN`, `FEAR_MODEL_NODRAW`.

### Regression check

The whole model subsystem is inert unless `FEAR_DEBUG_MODEL` is set, so the
world pass must be bit-identical with it unset. Confirmed against the
pre-change baseline capture:

| Build | Capture | `corr` vs `ref_ff.bmp` | max pixel diff vs old baseline |
|---|---|---|---|
| final | `mdl_base2` (no model) | 0.9849 | 0 |

The `g_Tint` lighting change was re-measured on the two models that were first
captured with the old double-shaded path:

| Model | before `g_Tint` | after `g_Tint` |
|---|---|---|
| `bomb` | 0.9386 | 0.9417 |
| `helmet` | 0.9199 | 0.9188 |

Both are within noise of each other, so the vertex-shader rewrite is a pure
quality change and did not alter placement, culling or coverage.

## Iteration 16 status

Iteration 16 recovered the player control path end to end. The chain
`CGameClientShell::Update -> UpdatePlaying -> UpdatePlayerMovement ->
CMoveMgr::Update` now runs every frame with real input, real velocity and
non-zero movement.

### Root causes found and fixed

1. **Instrumentation was measuring a dead copy of the sources.** The client
   target builds `FEAR Public Tools 1.08\Source\Game\ClientShellDLL\*.cpp`, not
   `lithtech\FEAR\ClientShellDLL\*.cpp`. Every diagnostic counter added to the
   latter tree was a no-op, which is why `movepipe update=0` looked like
   "Update is never called". After instrumenting the real tree the counters
   showed `update=1015 bail=1015`: `CMoveMgr::Update` was called every frame and
   bailed immediately.
2. **`ILTClient::GetClientObject()` returned NULL.** `Stub_GetClientObject_31`
   gates the result behind `FEAR_LOCAL_PLAYER=1` (or local-player scope), and
   the autostart soaks never set it. With the variable set the client object
   resolves (`local server object id=2032 resolved=...`) and `bail` drops to 0.
3. **`CFearPhysics` implemented only `ILTPhysics`.** `CMoveMgr` casts
   `g_pPhysicsLT` to `ILTClientPhysics*` and calls `UpdateMovement`, which sat
   past the end of the smaller vtable and jumped to `00000000`. `CFearPhysics`
   now derives from `ILTClientPhysics` and implements `UpdateMovement`,
   `MovePushObjects` and `RotatePushObjects`.
4. **`UpdateMovement` filled the wrong output.** `MoveInfo` is
   `{ m_hObject, m_dt, m_Offset }`; `CMoveMgr` reads `m_Offset` and applies it
   itself via `MoveObject`. The first implementation moved the object directly
   and left `m_Offset` as uninitialised stack (`0xCCCCCCCC`), so `CMoveMgr` added
   ~-1e8 per frame to the player position. It now integrates velocity into
   `m_Offset` only.
5. **`ILTPhysics::MoveObject` is a teleport, not a translate.** It sets the
   absolute position; adding the argument to the current position doubled the
   player position every frame.
6. **Object getters left outputs untouched on failure.** FEAR code ignores the
   `LTRESULT` and forwards the stack garbage. `GetObjectPos`,
   `GetObjectRotation` and both `GetObjectTransform` overloads now always write
   their outputs (zeros / identity) and only then report `LT_NOTFOUND`. All
   setters reject non-finite or absurd values so garbage can never enter the
   object map.
7. **The player had no ground contact.** `CMoveMgr::UpdateOnGround` sets
   `m_bOnGround` only when `CollisionInfo::m_hObject` is non-null and the plane
   normal is above `0.707`. `GetStandingOn` now reports a synthetic ground
   object (`FearClient_GetGroundObject`) with an up normal, so friction, gravity
   and jump gating behave.

### Verified behaviour

Soak `inp33` / `walk2` (`FEAR_AUTOSTART=1;FEAR_LOCAL_PLAYER=1;FEAR_AUTOWALK=420,820`):

- `core` counters: `movepipe update=1195 bail=0 flags=1195 normal=1195 vel=1195`.
- Input reaches the bind manager: `forward=1` while the key is held.
- Motion: `movemgr vel=0,0,440` and `pos.z` advances 44 units per 0.1 s frame
  while walking, then decays to zero when the key is released (`onground=1`).
- Rendered frames captured during the walk differ by 27 gray levels between
  consecutive captures while moving and by 7 after stopping, i.e. the camera
  really travels with the player.

### Still open

- The retail server never sees the movement: `srvplayer pos=-1100.00 -1436.00
  -370.00` stays at the spawn while the client-side player moves. Client
  messages `id=100` / `id=2` do reach `FearServerHost_SubmitClientMessage` but
  do not move the authoritative player.
- `ILTClient::IntersectSegment` is still a stub, so there is no world collision
  on the predicted path (the player can walk through geometry).
- Nothing yet plays the intro cutscene, and the third-person player body is
  created but not animated.

### Follow-up checks after iteration 16

- `Stub_GetClientObject_31` now enables the local player by default; only
  `FEAR_LOCAL_PLAYER=0` disables it, so a plain interactive launch gets control
  without extra environment variables.
- Client-to-server delivery was instrumented. The retail server shell really
  does receive the client stream through `pShell->OnMessage` (vtable slot 10):
  `183` (start), `185` (start level), `132` (ready/ack) and the 419-byte
  per-frame `id=2` packet all arrive. The authoritative player object still
  stays at the spawn, so the missing link is server-side input handling, not
  the message transport.
- A plain autostart soak (`def1`) without `FEAR_LOCAL_PLAYER` walks the player
  17600 units and stops cleanly, with `playerstate=1 inworld=1 gs=1` and no
  crash.
