# Building Lithtech on Windows

## Requirements

- Visual Studio 2022 (or Build Tools 2022) with the C++ x86/x64 toolset
- CMake 3.10 or newer
- Ninja (or another CMake generator)
- SDL2 development libraries. A 32-bit build needs the 32-bit SDL2 import library.
- Legacy DirectX SDK headers and libraries (June 2010 or August 2007)

The DirectX SDK is no longer installed by default and its installer cannot be unpacked on
current Windows versions without extra work. Both `Include` and `Lib/x86` (plus `Lib/x64`
for a 64-bit build) are required. Point `DXSDK_DIR` at a directory that contains them.

## Configure and build

```powershell
.\tools\build_windows.ps1 -Configuration Debug
```

The script locates `vcvars32.bat` through `vswhere.exe`, configures a 32-bit Ninja build and
builds it. `Debug` is the default configuration. DirectShow and the NOLF2 game code are disabled
by default; use `-EnableDirectShow` and `-BuildNolf2` to turn them back on.

Manual equivalent:

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
cmake -S . -B build-win32 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
      -DDXSDK_DIR=C:/tools/dxsdk -DSDL2_ROOT=C:/tools/SDL2
cmake --build build-win32
```

Build results are placed in `build-win32/OUT`:

| File | Purpose |
|------|---------|
| `Lithtech.exe` | Client engine |
| `Server.dll` | Server engine |
| `SndDrv.dll` | DirectSound 8 sound driver |
| `LTMsg.dll` | Localized message resources |

## CMake options

| Option | Default | Notes |
|--------|---------|-------|
| `BUILD_ENGINE` | `ON` | Build the engine |
| `BUILD_NOLF2` | `ON` | Build the NOLF2 game code |
| `BUILD_FEAR` | `OFF` | Build the F.E.A.R. game code |
| `BUILD_TOOLS` | `OFF` | Build the editing tools |
| `ENABLE_D3D` | `ON` | Direct3D 9 renderer |
| `ENABLE_DIRECTX` | `ON` | DirectInput/DirectPlay/D3DX |
| `ENABLE_DIRECTSHOW` | `ON` | DirectShow video playback; requires ATL |

DirectShow base classes include `atlbase.h`. The Visual Studio Build Tools installation does
not always ship ATL. Without the ATL component configure with:

```powershell
.\tools\build_windows.ps1 -Configuration Debug -ConfigureOnly
cmake -S . -B build-win32 -DENABLE_DIRECTSHOW=OFF
cmake --build build-win32
```

## Running the engine

`Lithtech.exe` loads `engine.rez`, `game.rez`, `game2.rez` and `sound.rez` from the working
directory and then loads `cshell.dll` from the same place. A game installation can be selected
with:

```bat
Lithtech.exe -workingdir "D:\path\to\game"
```

## Status of the F.E.A.R. target

The engine in this repository reports itself as `Jupiter 69` and exposes the client/server
interfaces of that generation (`IClientShell` 4, `ILTClient` 4).

F.E.A.R. was shipped on a later Jupiter Ex build and differs in several places that prevent the
retail game from running with this engine as-is:

- F.E.A.R. stores its data in `LTAR` version 3 archives (`*.Arch00`). The engine's resource
  manager reads the older text-header `*.rez` format instead.
- The F.E.A.R. SDK 1.08 game code expects `IClientShell` 5 and `ILTClient` 5. The virtual
  layouts and several callbacks differ from the interfaces this engine exports.
- The retail game does not ship a separate `cshell.dll`; its client shell is linked into
  `FEAR.exe`, so a client shell has to be built from the SDK sources to load anything at all.

Because of this, launching `Lithtech.exe` against a retail F.E.A.R. installation initializes the
engine and then exits while looking for the missing `*.rez` resource trees. Building the F.E.A.R.
game code with `-DBUILD_FEAR=ON` is the first step of the port, but it requires the engine
interfaces and data loaders to be brought up to the Jupiter Ex generation.

## Building and running the F.E.A.R. port

The port is an isolated configuration. `BUILD_FEAR_PORT=ON` forces `BUILD_NOLF2` and `BUILD_FEAR`
off, so the older NOLF2/F.E.A.R. targets cannot collide by output name, and writes every runtime
artifact to `OUT_FEAR` instead of `OUT`.

```bat
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
cmake -S . -B build-fear -G Ninja -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DBUILD_FEAR_PORT=ON ^
      -DFEAR_SDK_DIR=C:/tools/fear-sdk ^
      -DFEAR_RETAIL_DIR="D:/SteamLibrary/steamapps/common/FEAR Ultimate Shooter Edition" ^
      -DDXSDK_DIR=C:/tools/dxsdk -DSDL2_ROOT=C:/repos/vkquake-rt/Windows/SDL2
cmake --build build-fear
```

`FEAR_SDK_DIR` is required and must contain `engine/sdk/inc/iclientshell.h`. `FEAR_RETAIL_DIR` is
recorded for reference; the staging script takes the installation path as its own parameter.

Artifacts in `build-fear/OUT_FEAR`:

| File | Target | Purpose |
|------|--------|---------|
| `Lithtech.exe` | `EXE_Lithtech` | Client engine |
| `Server.dll` | `DLL_Server` | Server engine |
| `SndDrv.dll` | `DLL_SndDrvDX8` | DirectSound 8 sound driver |
| `LTMsg.dll` | `DLL_LTMsg` | Localized message resources |
| `CShell.dll` | `FEAR_CShell` | v4 client-shell proxy; loads `FearHost.dll` |
| `FearHost.dll` | `FEAR_V5Host` | v5 facade compiled against the F.E.A.R. SDK headers |
| `cres.dll` | `FEAR_Cres` | Client resource loader stub |
| `sres.dll` | `FEAR_Sres` | Server resource loader stub |
| `clientfx.fxd` | `FEAR_ClientFxStub` | ClientFX stub DLL (no effects) |
| `ltar_dump.exe` | `TOOLS_LTarDump` | LTAR inspection and extraction |

`clientfx.fxd` is a DLL with its suffix renamed to `.fxd`. It exports `fxGetNum`, `fxGetRef`,
`fxGetVersion`, `fxInitDLLRuntime`, `fxTermDLLRuntime`, `fxDelete` and `fxFreePropList`, reports
zero effects, and does not link or include the F.E.A.R. SDK.

### Staging and running

The retail installation is treated as read-only. `tools/run_fear_port.ps1` builds a writable run
directory, links the archives instead of copying them, and extracts the game databases through
`ltar_dump.exe`:

```powershell
.\tools\run_fear_port.ps1 -RetailDir "D:\SteamLibrary\steamapps\common\FEAR Ultimate Shooter Edition"
```

| Parameter | Default | Notes |
|-----------|---------|-------|
| `-RetailDir` | required | Retail install root that contains `Default.archcfg` |
| `-BuildDir` | `build-fear` | Build directory; artifacts are read from `OUT_FEAR` |
| `-Stage` | `<repo>\run-fear-engine` | Run directory; removed and recreated on every call |
| `-NoRun` | off | Stage only, do not start the engine |

The script:

- recreates the stage directory and refuses stage paths that are the repository, a parent of the
  repository, the retail root or a parent of the retail root, or a drive root;
- creates a hard link for every `*.Arch00` listed in `Default.archcfg`, in file order, and falls back
  to a copy with a warning when the link cannot be created (for example across volumes), then copies
  `Default.archcfg`;
- copies `Lithtech.exe`, `Server.dll`, `SndDrv.dll`, `LTMsg.dll`, `CShell.dll`, `FearHost.dll`,
  `cres.dll`, `sres.dll` and `clientfx.fxd` from `OUT_FEAR`, and fails when one of them is missing;
- copies `SDL2.dll` from the first existing candidate: `$env:SDL2_ROOT\lib`,
  `$env:SDL2_ROOT\lib\x86`, `$env:SDL2DIR\lib`, `$env:SDL2DIR\lib\x86`, `C:\tools\SDL2\lib`,
  `C:\SDL2\lib`, `C:\repos\vkquake-rt\Windows\SDL2\lib`;
- copies `EngineServer.dll`, `GameDatabase.dll`, `StringEditRuntime.dll`, `LTMemory.dll`,
  `Config.Strdb00p`, `msvcp71.dll` and `msvcr71.dll` from the retail root, and fails when one of the
  four runtime DLLs is missing;
- extracts `StringDatabase\FEAR.Strdb00p`, `Database\FEAR.Gamdb00p`,
  `DatabaseLocalized\FEARL.Gamdb00p` and `GameServer.dll` with `ltar_dump.exe`, searching the
  archives in `Default.archcfg` order with `find` and writing the result with `extract`; a file that
  is not found in any archive is skipped with a warning;
- prints the exact command line and starts
  `Lithtech.exe -fear -workingdir <stage> -archcfg Default.archcfg` from the stage directory, unless
  `-NoRun` is given or another engine instance is running.

Nothing is written into the retail directory, and the process check refuses to start while any
`quakeray`, `qray_*` or `Lithtech` process is active.

### F.E.A.R. port iteration

Once the port has been configured, an edit to the shell, the v5 host or one of the stubs is
rebuilt and restaged without launching the engine:

```powershell
cmake --build build-fear
.\tools\run_fear_port.ps1 -RetailDir "D:\SteamLibrary\steamapps\common\FEAR Ultimate Shooter Edition" -NoRun
```

The full configure from the previous section is only needed again after a CMake change. `-NoRun`
refreshes `Lithtech.exe`, `Server.dll`, `SndDrv.dll`, `LTMsg.dll`, `CShell.dll`, `FearHost.dll`,
`cres.dll`, `sres.dll` and `clientfx.fxd` from `OUT_FEAR`, rebuilds the hardlinked archive stage and
leaves it ready for a manual start, so the staging step can be repeated as often as needed while the
menu blockers are worked off.

### Running on a dedicated desktop

`tools/run_on_desktop.ps1` opens the Win32 desktop named `fear` (creating it when it does not
exist) and starts a process with `STARTUPINFO.lpDesktop = "winsta0\fear"`. This keeps the game
window off the interactive desktop used for development. Use `-Switch` to make the desktop
visible while the process initializes and `-SwitchBackSeconds` to return to `Default`
afterwards:

```powershell
.\tools\run_on_desktop.ps1 -FilePath "$PWD\run-fear-engine\Lithtech.exe" `
  -ArgumentList "-fear -workingdir `"$PWD\run-fear-engine`" -archcfg Default.archcfg" `
  -WorkingDirectory "$PWD\run-fear-engine" -DesktopName fear -Switch -SwitchBackSeconds 30
```

A non-visible Win32 desktop can fail to initialize the D3D device. Switch to it for the first
frames, then return; the process keeps running. The current v5 server bootstrap is enabled by
`StartGame` automatically. `FEAR_AUTOSTART=1` bypasses menu navigation and starts a new game
through the source client, which is useful for automated runs.

