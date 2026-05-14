# Dedicated MethASM benchmark harness against C.
#
# - Reads benchmark matrix from docs/benchmarks/harness.json
# - Builds each suite via its build script
# - Executes Meth and C binaries
# - Parses "Time: <N> ms" output
# - Writes canonical JSON to docs/benchmarks/latest.json
# - Mirrors JSON to web/benchmarks.json for the web server
#
# Usage:
#   .\tools\benchmark\run-benchmarks.ps1
#   .\tools\benchmark\run-benchmarks.ps1 -BuildCompiler
#   .\tools\benchmark\run-benchmarks.ps1 -Quiet

param(
    [switch]$BuildCompiler,
    [switch]$Quiet,
    [string]$ConfigPath = "docs/benchmarks/harness.json"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $Root

function Write-Log {
    param([string]$Message)
    if (-not $Quiet) { Write-Host $Message }
}

function Normalize-Path {
    param([string]$PathValue)
    return ($PathValue -replace "/", "\\")
}

function Parse-TimeMs {
    param([string]$Output)

    if ([string]::IsNullOrWhiteSpace($Output)) { return $null }

    $patterns = @(
        "(?im)^\s*Time:\s*([0-9]+(?:\.[0-9]+)?)\s*ms\b",
        "(?im)^\s*Elapsed(?:\s+time)?\s*:\s*([0-9]+(?:\.[0-9]+)?)\s*ms\b",
        "(?im)\b([0-9]+(?:\.[0-9]+)?)\s*ms\b"
    )

    foreach ($pattern in $patterns) {
        if ($Output -match $pattern) {
            return [int][math]::Round([double]$Matches[1], 0)
        }
    }

    return $null
}

function Invoke-ProgramCapture {
    param([string]$ExePath)

    if (-not (Test-Path $ExePath)) {
        return @{
            output = ""
            exit_code = -1
        }
    }

    $output = & $ExePath 2>&1 | Out-String
    return @{
        output = $output
        exit_code = $LASTEXITCODE
    }
}

$configFullPath = Join-Path $Root (Normalize-Path $ConfigPath)
if (-not (Test-Path $configFullPath)) {
    Write-Error "Harness config not found: $configFullPath"
    exit 1
}

$config = Get-Content -Raw -Path $configFullPath | ConvertFrom-Json
if ($null -eq $config -or $null -eq $config.benchmarks -or $config.benchmarks.Count -eq 0) {
    Write-Error "Harness config has no benchmarks: $configFullPath"
    exit 1
}

if ($BuildCompiler) {
    Write-Log "Building compiler..."
    & .\build.bat
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

if (-not (Test-Path ".\bin\methlang.exe")) {
    Write-Error "Compiler not found. Run with -BuildCompiler or build manually."
    exit 1
}

$results = @()
$failed = @()

foreach ($bench in $config.benchmarks) {
    $name = [string]$bench.name
    $description = [string]$bench.description
    $buildScript = Normalize-Path ([string]$bench.build_script)
    $methExe = Normalize-Path ([string]$bench.meth_exe)
    $cExe = Normalize-Path ([string]$bench.c_exe)

    Write-Log "Building $name..."
    $null = & cmd /c $buildScript 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Log "  Build failed: $name"
        $failed += $name
        continue
    }

    $methRun = Invoke-ProgramCapture -ExePath $methExe
    $cRun = Invoke-ProgramCapture -ExePath $cExe

    $methMs = if ($methRun.exit_code -eq 0) { Parse-TimeMs $methRun.output } else { $null }
    $cMs = if ($cRun.exit_code -eq 0) { Parse-TimeMs $cRun.output } else { $null }

    $relative = $null
    if ($null -ne $methMs -and $null -ne $cMs -and $cMs -gt 0) {
        $relative = [math]::Round(($methMs / $cMs), 2)
    }

    $results += [ordered]@{
        name = $name
        description = $description
        meth_ms = $methMs
        c_ms = $cMs
        relative = $relative
    }

    $status = if ($null -ne $methMs) { "Meth: $methMs ms" } else { "Meth: FAIL" }
    $status += " | "
    $status += if ($null -ne $cMs) { "C: $cMs ms" } else { "C: FAIL" }
    if ($null -ne $relative) { $status += " | $relative`x vs C" }
    Write-Log "  $status"
}

$payload = [ordered]@{
    generated = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    harness = [ordered]@{
        config = $ConfigPath
        mode = if ($null -ne $config.mode) { [string]$config.mode } else { "meth-vs-c" }
    }
    benchmarks = $results
    failed = $failed
}

$json = $payload | ConvertTo-Json -Depth 10

$primaryOutput = Normalize-Path ([string]$config.outputs.primary)
$primaryOutputPath = Join-Path $Root $primaryOutput
$primaryDir = Split-Path -Parent $primaryOutputPath
if (-not (Test-Path $primaryDir)) { New-Item -ItemType Directory -Path $primaryDir -Force | Out-Null }
$json | Set-Content -Path $primaryOutputPath -Encoding UTF8

$mirrorOutput = Normalize-Path ([string]$config.outputs.mirror_web)
if (-not [string]::IsNullOrWhiteSpace($mirrorOutput)) {
    $mirrorPath = Join-Path $Root $mirrorOutput
    $mirrorDir = Split-Path -Parent $mirrorPath
    if (-not (Test-Path $mirrorDir)) { New-Item -ItemType Directory -Path $mirrorDir -Force | Out-Null }
    $json | Set-Content -Path $mirrorPath -Encoding UTF8
}

Write-Log ""
Write-Log "Wrote $primaryOutputPath"
if (-not [string]::IsNullOrWhiteSpace($mirrorOutput)) {
    Write-Log "Mirrored to $(Join-Path $Root $mirrorOutput)"
}
