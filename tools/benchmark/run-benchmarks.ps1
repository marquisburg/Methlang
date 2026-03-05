# Methlang vs C Benchmark Script
#
# Builds all benchmarks, runs Methlang and C counterparts, parses timing,
# and writes web/benchmarks.json for the benchmarks page.
#
# Usage: .\tools\benchmark\run-benchmarks.ps1
#        .\tools\benchmark\run-benchmarks.ps1 -BuildCompiler
#
# Requires: NASM, gcc, Methlang compiler (or -BuildCompiler)

param(
    [switch]$BuildCompiler
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $Root

if ($BuildCompiler) {
    Write-Host "Building compiler..."
    & .\build.bat
    if ($LASTEXITCODE -ne 0) { exit 1 }
}

if (-not (Test-Path ".\bin\methlang.exe")) {
    Write-Error "Compiler not found. Run with -BuildCompiler or build manually."
    exit 1
}

# Benchmark definitions: name, meth path, c path, build script
$benchmarks = @(
    @{ Name = "fib"; Description = "Fibonacci fib(35) x 10M"; BuildScript = "examples\fib\build.bat"; MethExe = "examples\fib\fib.exe"; CExe = "examples\fib\fib_c.exe" },
    @{ Name = "word_count"; Description = "Word count 256 KB buffer x 500"; BuildScript = "examples\word_count\build.bat"; MethExe = "examples\word_count\word_count.exe"; CExe = "examples\word_count\word_count_c.exe" },
    @{ Name = "grep"; Description = "Grep ERROR 256 KB x 200"; BuildScript = "examples\grep\build.bat"; MethExe = "examples\grep\grep.exe"; CExe = "examples\grep\grep_c.exe" },
    @{ Name = "sum_squares"; Description = "Sum of squares 1..100000 x 500"; BuildScript = "examples\sum_squares\build.bat"; MethExe = "examples\sum_squares\sum_squares.exe"; CExe = "examples\sum_squares\sum_squares_c.exe" },
    @{ Name = "collatz"; Description = "Collatz steps 1..100000 x 10"; BuildScript = "examples\collatz\build.bat"; MethExe = "examples\collatz\collatz.exe"; CExe = "examples\collatz\collatz_c.exe" }
)

function Parse-TimeMs {
    param([string]$Output)
    if ($Output -match "Time:\s*(\d+)\s*ms") {
        return [int]$Matches[1]
    }
    return $null
}

$results = @()
$failed = @()

foreach ($bench in $benchmarks) {
    Write-Host "Building $($bench.Name)..."
    $null = & cmd /c $bench.BuildScript 2>&1
    if ($LASTEXITCODE -ne 0) {
        Write-Host "  Build failed: $($bench.Name)"
        $failed += $bench.Name
        continue
    }

    $methMs = $null
    $cMs = $null

    if (Test-Path $bench.MethExe) {
        $methOut = & $bench.MethExe 2>&1 | Out-String
        $methMs = Parse-TimeMs $methOut
    }
    if (Test-Path $bench.CExe) {
        $cOut = & $bench.CExe 2>&1 | Out-String
        $cMs = Parse-TimeMs $cOut
    }

    $relative = $null
    if ($null -ne $methMs -and $null -ne $cMs -and $cMs -gt 0) {
      $relative = [math]::Round($methMs / $cMs, 2)
    }

    $results += @{
        name = $bench.Name
        description = $bench.Description
        meth_ms = $methMs
        c_ms = $cMs
        relative = $relative
    }

    $status = if ($null -ne $methMs) { "Meth: $methMs ms" } else { "Meth: FAIL" }
    $status += " | "
    $status += if ($null -ne $cMs) { "C: $cMs ms" } else { "C: FAIL" }
    if ($relative) { $status += " | $relative`x vs C" }
    Write-Host "  $status"
}

$json = @{
    generated = (Get-Date -Format "yyyy-MM-ddTHH:mm:ssZ")
    benchmarks = $results
    failed = $failed
} | ConvertTo-Json -Depth 4

$outPath = Join-Path $Root "web\benchmarks.json"
$json | Set-Content -Path $outPath -Encoding UTF8
Write-Host ""
Write-Host "Wrote $outPath"
