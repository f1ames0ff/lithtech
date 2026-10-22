param(
    [Parameter(Mandatory = $true)]
    [string]$RetailDir,
    [string]$BuildDir = "build-fear",
    [string]$Stage = "",
    [string]$GameClientDir = "",
    [string]$GameClientDll = "",
    [switch]$NoRun
)

$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot

function Get-FullPath([string]$Path, [string]$BaseDir) {
    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }
    return [System.IO.Path]::GetFullPath((Join-Path $BaseDir $Path))
}

function Test-PathInside([string]$Candidate, [string]$Parent) {
    $c = $Candidate.TrimEnd('\', '/')
    $p = $Parent.TrimEnd('\', '/')
    if ($c.Equals($p, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $true
    }
    return $c.StartsWith($p + '\', [System.StringComparison]::OrdinalIgnoreCase)
}

function Find-Sdl2Dll() {
    $candidates = New-Object System.Collections.Generic.List[string]
    if ($env:SDL2_ROOT) {
        $candidates.Add((Join-Path $env:SDL2_ROOT "lib"))
        $candidates.Add((Join-Path $env:SDL2_ROOT "lib\x86"))
    }
    if ($env:SDL2DIR) {
        $candidates.Add((Join-Path $env:SDL2DIR "lib"))
        $candidates.Add((Join-Path $env:SDL2DIR "lib\x86"))
    }
    $candidates.Add("C:\tools\SDL2\lib")
    $candidates.Add("C:\SDL2\lib")
    $candidates.Add("C:\repos\vkquake-rt\Windows\SDL2\lib")
    foreach ($dir in $candidates) {
        $dll = Join-Path $dir "SDL2.dll"
        if (Test-Path -LiteralPath $dll) {
            return $dll
        }
    }
    return $null
}

if (-not (Test-Path -LiteralPath $RetailDir)) {
    throw "Retail directory not found: $RetailDir"
}

$retail = (Resolve-Path -LiteralPath $RetailDir).ProviderPath
if (-not $Stage) {
    $Stage = Join-Path $repoRoot "run-fear-engine"
}
$stage = Get-FullPath $Stage $repoRoot
$build = Get-FullPath $BuildDir $repoRoot

if (-not (Test-Path -LiteralPath $retail -PathType Container)) {
    throw "Retail path is not a directory: $retail"
}
if (Test-PathInside $stage $retail) {
    if (-not (Test-PathInside $stage $repoRoot)) {
        throw "Refusing a stage inside the retail install outside the repository: $stage"
    }
}
if (Test-PathInside $retail $stage) {
    throw "Refusing a stage that contains the retail install: $stage"
}
if (Test-PathInside $repoRoot $stage) {
    throw "Refusing a stage that contains the repository: $stage"
}
if ($stage.TrimEnd('\') -match '^[A-Za-z]:$') {
    throw "Refusing a stage at a drive root: $stage"
}

$outDir = Join-Path $build "OUT_FEAR"
if (-not (Test-Path -LiteralPath $outDir -PathType Container)) {
    throw "Build output directory not found: $outDir. Configure and build with -DBUILD_FEAR_PORT=ON first."
}

$archCfg = Join-Path $retail "Default.archcfg"
if (-not (Test-Path -LiteralPath $archCfg)) {
    throw "Default.archcfg not found in the retail directory: $archCfg"
}
$archives = @(Get-Content -LiteralPath $archCfg |
    ForEach-Object { $_.Trim() } |
    Where-Object { $_ -and -not $_.StartsWith("#") -and -not $_.StartsWith(";") })
if ($archives.Count -eq 0) {
    throw "Default.archcfg lists no archives: $archCfg"
}
if ($archives.Count -ne 36) {
    Write-Warning "Default.archcfg lists $($archives.Count) archives, expected 36."
}

Write-Host "Staging F.E.A.R. port run directory"
Write-Host "  retail: $retail"
Write-Host "  build:  $outDir"
Write-Host "  stage:  $stage"

if (Test-Path -LiteralPath $stage) {
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null

$linked = 0
$copied = 0
foreach ($name in $archives) {
    $source = Join-Path $retail $name
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Archive listed in Default.archcfg is missing: $source"
    }
    $target = Join-Path $stage $name
    try {
        New-Item -ItemType HardLink -Path $target -Target $source -Force | Out-Null
        $linked++
    } catch {
        Copy-Item -LiteralPath $source -Destination $target -Force
        $copied++
        Write-Warning "Hardlink failed for $name, copied instead: $($_.Exception.Message)"
    }
}
Write-Host "Archives: $linked hardlinked, $copied copied"

Copy-Item -LiteralPath $archCfg -Destination (Join-Path $stage "Default.archcfg") -Force

$buildFiles = @(
    "Lithtech.exe",
    "Server.dll",
    "SndDrv.dll",
    "LTMsg.dll",
    "CShell.dll",
    "FearHost.dll",
    "cres.dll",
    "sres.dll",
    "clientfx.fxd"
)
$missingBuild = @()
foreach ($name in $buildFiles) {
    $source = Join-Path $outDir $name
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $stage $name) -Force
    } else {
        $missingBuild += $name
    }
}
if ($missingBuild.Count -gt 0) {
    throw "Missing build artifacts in ${outDir}: $($missingBuild -join ', ')"
}
Write-Host "Build artifacts copied: $($buildFiles.Count)"

$sdl2 = Find-Sdl2Dll
if (-not $sdl2) {
    throw "SDL2.dll not found. Set SDL2_ROOT to the SDL2 root that contains lib\SDL2.dll."
}
Copy-Item -LiteralPath $sdl2 -Destination (Join-Path $stage "SDL2.dll") -Force
Write-Host "SDL2.dll from $sdl2"

$retailFiles = @(
    "EngineServer.dll",
    "GameDatabase.dll",
    "StringEditRuntime.dll",
    "LTMemory.dll",
    "Config.Strdb00p",
    "msvcp71.dll",
    "msvcr71.dll"
)
$missingRetail = @()
foreach ($name in $retailFiles) {
    $source = Join-Path $retail $name
    if (Test-Path -LiteralPath $source) {
        Copy-Item -LiteralPath $source -Destination (Join-Path $stage $name) -Force
    } else {
        $missingRetail += $name
    }
}
if ($missingRetail.Count -gt 0) {
    Write-Warning "Missing retail support files: $($missingRetail -join ', ')"
    $requiredRuntime = @($missingRetail | Where-Object { $_ -in @("EngineServer.dll", "GameDatabase.dll", "StringEditRuntime.dll", "LTMemory.dll") })
    if ($requiredRuntime.Count -gt 0) {
        throw "Required runtime DLLs are missing from the retail directory: $($requiredRuntime -join ', ')"
    }
}

if ($GameClientDir) {
    $publicTools = Get-FullPath $GameClientDir $repoRoot
    if (-not (Test-Path -LiteralPath $publicTools -PathType Container)) {
        throw "GameClientDir not found: $publicTools"
    }
    $publicToolsFiles = @(
        @{ Source = "GameClient.dll"; Target = "GameClient.dll" },
        @{ Source = "GameServer.dll"; Target = "GameServer.dll" },
        @{ Source = "ClientFx.fxd"; Target = "clientfx.fxd" }
    )
    foreach ($entry in $publicToolsFiles) {
        $source = Join-Path $publicTools $entry.Source
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Missing Public Tools runtime file: $source"
        }
        Copy-Item -LiteralPath $source -Destination (Join-Path $stage $entry.Target) -Force
        Write-Host "Public Tools runtime: $($entry.Source) -> $($entry.Target)"
    }
}

if ($GameClientDll) {
    $source = Get-FullPath $GameClientDll $repoRoot
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "GameClientDll not found: $source"
    }
    Copy-Item -LiteralPath $source -Destination (Join-Path $stage "GameClient.dll") -Force
    Write-Host "Public Tools client: $source"
} elseif (-not $GameClientDir) {
    Write-Warning "No GameClient.dll selected. Build it with tools/build_fear_client.ps1 and pass -GameClientDll; the retail client extracted by CShell is not ABI-compatible with FearHost."
}

$ltar = Join-Path $outDir "ltar_dump.exe"
if (-not (Test-Path -LiteralPath $ltar)) {
    throw "ltar_dump.exe not found in $outDir"
}

function Invoke-Ltar([string]$LtarPath, [string[]]$LtarArgs) {
    $savedPreference = $ErrorActionPreference
    $ErrorActionPreference = "SilentlyContinue"
    try {
        & $LtarPath @LtarArgs 2>$null | Out-Null
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedPreference
    }
    return $code
}

$databaseFiles = @(
    "StringDatabase\FEAR.Strdb00p",
    "Database\FEAR.Gamdb00p",
    "DatabaseLocalized\FEARL.Gamdb00p"
)
foreach ($relative in $databaseFiles) {
    $owner = $null
    foreach ($name in $archives) {
        $archive = Join-Path $retail $name
        if ((Invoke-Ltar -LtarPath $ltar -LtarArgs @("find", $archive, $relative)) -eq 0) {
            $owner = $archive
        }
    }
    if (-not $owner) {
        Write-Warning "Not found in any archive, skipped: $relative"
        continue
    }
    if ((Invoke-Ltar -LtarPath $ltar -LtarArgs @("extract", $owner, $relative, $stage)) -ne 0) {
        throw "Failed to extract $relative from $owner"
    }
    Write-Host "Extracted $relative from $(Split-Path -Leaf $owner)"
}

$archiveRuntimeFiles = @(
    "GameServer.dll"
)
foreach ($relative in $archiveRuntimeFiles) {
    $owner = $null
    foreach ($name in $archives) {
        $archive = Join-Path $retail $name
        if ((Invoke-Ltar -LtarPath $ltar -LtarArgs @("find", $archive, $relative)) -eq 0) {
            $owner = $archive
        }
    }
    if (-not $owner) {
        Write-Warning "Not found in any archive, skipped: $relative"
        continue
    }
    if ((Invoke-Ltar -LtarPath $ltar -LtarArgs @("extract", $owner, $relative, $stage)) -ne 0) {
        throw "Failed to extract $relative from $owner"
    }
    Write-Host "Extracted $relative from $(Split-Path -Leaf $owner)"
}

Write-Host "Stage ready: $stage"

$exe = Join-Path $stage "Lithtech.exe"
$arguments = @("-fear", "-workingdir", ('"' + $stage + '"'), "-archcfg", "Default.archcfg")
Write-Host ('Command line: "' + $exe + '" ' + ($arguments -join ' '))

if ($NoRun) {
    Write-Host "-NoRun specified, engine not launched."
    exit 0
}

$active = @(Get-Process -Name quakeray, "qray_*", Lithtech -ErrorAction SilentlyContinue)
if ($active.Count -gt 0) {
    $names = ($active | ForEach-Object { "$($_.ProcessName)($($_.Id))" }) -join ", "
    throw "Refusing to launch: another engine instance is running: $names"
}

$process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $stage -PassThru -Wait
Write-Host "Lithtech.exe exit code: $($process.ExitCode)"
if ($process.ExitCode -ne 0) {
    exit $process.ExitCode
}
exit 0
