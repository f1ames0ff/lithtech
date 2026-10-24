param(
    [Parameter(Mandatory = $true)]
    [string]$PublicToolsDir,
    [string]$BuildDir = "build-fear-server",
    [string]$Configuration = "Debug"
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
if (Test-Path -LiteralPath (Join-Path $PublicToolsDir "Source\Game\ObjectDLL\Game_ServerShell.vcproj")) {
    $sourceDir = (Resolve-Path -LiteralPath (Join-Path $PublicToolsDir "Source")).ProviderPath
} elseif (Test-Path -LiteralPath (Join-Path $PublicToolsDir "Game\ObjectDLL\Game_ServerShell.vcproj")) {
    $sourceDir = (Resolve-Path -LiteralPath $PublicToolsDir).ProviderPath
} else {
    throw "Public Tools source not found under $PublicToolsDir"
}

$buildPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $BuildDir))

$vcvars = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
if (-not (Test-Path -LiteralPath $vcvars)) {
    throw "vcvars32.bat not found at $vcvars"
}

$cmdFile = Join-Path $env:TEMP ("fear_server_build_{0}.cmd" -f [System.Guid]::NewGuid().ToString("N"))
$lines = @(
    "@echo off",
    ('call "{0}" >nul' -f $vcvars),
    ('cmake -S "{0}" -B "{1}" -G Ninja -DCMAKE_BUILD_TYPE={2} -DFEAR_PUBLIC_TOOLS_SOURCE="{3}"' -f (Join-Path $repoRoot "FEAR\Compat\publictools_server"), $buildPath, $Configuration, $sourceDir),
    "if errorlevel 1 exit /b %errorlevel%",
    ('ninja -C "{0}"' -f $buildPath)
)
Set-Content -LiteralPath $cmdFile -Value $lines -Encoding ASCII
try {
    & cmd.exe /c $cmdFile
    if ($LASTEXITCODE -ne 0) {
        throw "Public Tools server build failed with exit code $LASTEXITCODE"
    }
} finally {
    Remove-Item -LiteralPath $cmdFile -Force -ErrorAction SilentlyContinue
}

$dll = Join-Path $buildPath "GameServer.dll"
if (-not (Test-Path -LiteralPath $dll)) {
    throw "GameServer.dll was not produced at $dll"
}

Get-Item -LiteralPath $dll | Select-Object FullName, Length, LastWriteTime
