param(
    [Parameter(Mandatory = $true)]
    [string]$PublicToolsDir,
    [string]$BuildDir = "build-fear-client",
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$sourceDir = $null
if (Test-Path -LiteralPath (Join-Path $PublicToolsDir "Source\Game\ClientShellDLL\Game_ClientShell.vcproj")) {
    $sourceDir = (Resolve-Path -LiteralPath (Join-Path $PublicToolsDir "Source")).ProviderPath
} elseif (Test-Path -LiteralPath (Join-Path $PublicToolsDir "Game\ClientShellDLL\Game_ClientShell.vcproj")) {
    $sourceDir = (Resolve-Path -LiteralPath $PublicToolsDir).ProviderPath
} else {
    throw "Public Tools source not found under $PublicToolsDir"
}

$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDir))

function Find-VcVars32 {
    $candidates = New-Object System.Collections.Generic.List[string]
    if ($env:VCINSTALLDIR) {
        $candidates.Add((Join-Path $env:VCINSTALLDIR "Auxiliary\Build\vcvars32.bat"))
    }
    $vswhere = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($install) {
            $candidates.Add((Join-Path $install "VC\Auxiliary\Build\vcvars32.bat"))
        }
    }
    $candidates.Add("C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat")
    $candidates.Add("C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat")
    foreach ($path in $candidates) {
        if (Test-Path -LiteralPath $path) {
            return $path
        }
    }
    throw "vcvars32.bat not found. Set VCINSTALLDIR or install the MSVC x86 toolchain."
}

$vcvars = Find-VcVars32
$cmdFile = Join-Path $env:TEMP ("fear_client_build_{0}.cmd" -f [System.Guid]::NewGuid().ToString("N"))
$lines = @(
    "@echo off",
    ('call "{0}" >nul' -f $vcvars),
    ('cmake -S "{0}" -B "{1}" -G Ninja -DCMAKE_BUILD_TYPE={2} -DFEAR_PUBLIC_TOOLS_SOURCE="{3}"' -f (Join-Path $repoRoot "FEAR\Compat\publictools_client"), $buildPath, $Configuration, $sourceDir),
    "if errorlevel 1 exit /b %errorlevel%",
    ('ninja -C "{0}"' -f $buildPath)
)
Set-Content -LiteralPath $cmdFile -Value $lines -Encoding ASCII
try {
    & cmd.exe /c $cmdFile
    if ($LASTEXITCODE -ne 0) {
        throw "Public Tools client build failed with exit code $LASTEXITCODE"
    }
} finally {
    Remove-Item -LiteralPath $cmdFile -Force -ErrorAction SilentlyContinue
}

$dll = Join-Path $buildPath "GameClient.dll"
if (-not (Test-Path -LiteralPath $dll)) {
    throw "GameClient.dll was not produced at $dll"
}

Get-Item -LiteralPath $dll | Select-Object FullName, Length, LastWriteTime
