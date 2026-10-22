param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Debug",
    [string]$BuildDir = "build-win32",
    [string]$DXSDK = $env:DXSDK_DIR,
    [string]$SDL2 = $env:SDL2_ROOT,
    [switch]$EnableDirectShow,
    [switch]$BuildNolf2,
    [switch]$ConfigureOnly
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot

if (-not $DXSDK) {
    if (Test-Path "C:\tools\dxsdk") {
        $DXSDK = "C:\tools\dxsdk"
    } elseif (Test-Path "C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)") {
        $DXSDK = "C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)"
    } elseif (Test-Path "C:\Program Files (x86)\Microsoft DirectX SDK (August 2007)") {
        $DXSDK = "C:\Program Files (x86)\Microsoft DirectX SDK (August 2007)"
    } else {
        throw "DirectX SDK not found. Pass -DXSDK <path> or set DXSDK_DIR."
    }
}

if (-not $SDL2) {
    $candidates = @(
        $env:SDL2DIR,
        "C:\SDL2",
        "C:\tools\SDL2"
    )
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path (Join-Path $candidate "include\SDL.h"))) {
            $SDL2 = $candidate
            break
        }
    }
    if (-not $SDL2) {
        throw "SDL2 not found. Pass -SDL2 <path> or set SDL2_ROOT."
    }
}

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere.exe not found. Install Visual Studio 2022 Build Tools."
}

$vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    throw "No Visual Studio installation with the C++ toolset was found."
}

$vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars32.bat"
if (-not (Test-Path $vcvars)) {
    throw "vcvars32.bat not found at $vcvars"
}

$cmakeDir = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmakeDir) {
    throw "cmake was not found in PATH."
}

$directShow = "OFF"
if ($EnableDirectShow) { $directShow = "ON" }
$nolf2 = "OFF"
if ($BuildNolf2) { $nolf2 = "ON" }

$configureArgs = @(
    "cmake",
    "-S", "`"$root`"",
    "-B", "`"$root\$BuildDir`"",
    "-G", "Ninja",
    "-DCMAKE_BUILD_TYPE=$Configuration",
    "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
    "-DENABLE_DIRECTSHOW=$directShow",
    "-DBUILD_NOLF2=$nolf2",
    "-DDXSDK_DIR=`"$DXSDK`"",
    "-DSDL2_ROOT=`"$SDL2`""
)

$command = "call `"$vcvars`" >nul && $($configureArgs -join ' ')"
if (-not $ConfigureOnly) {
    $command += " && cmake --build `"$root\$BuildDir`" --config $Configuration"
}

Write-Host "Configuring $Configuration build in $BuildDir"
cmd /c $command
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if (-not $ConfigureOnly) {
    Write-Host "Artifacts: $root\$BuildDir\OUT"
}

exit 0
