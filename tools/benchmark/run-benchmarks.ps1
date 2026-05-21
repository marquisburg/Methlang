# Dedicated Mettle benchmark harness against C.
#
# - Reads benchmark matrix from docs/benchmarks/harness.json
# - Compiles Mettle and C binaries (timed)
# - Records executable sizes
# - Executes binaries (warmup + median of N runs)
# - Parses "Time: <N> us" output (falls back to ms)
# - Writes canonical JSON to docs/benchmarks/latest.json
# - Mirrors JSON to web/benchmarks.json for the web server
#
# Usage:
#   .\tools\benchmark\run-benchmarks.ps1
#   .\tools\benchmark\run-benchmarks.ps1 -BuildCompiler
#   .\tools\benchmark\run-benchmarks.ps1 -Runs 7 -Warmup 2
#   .\tools\benchmark\run-benchmarks.ps1 -Quiet

param(
    [switch]$BuildCompiler,
    [switch]$Quiet,
    [string]$ConfigPath = "docs/benchmarks/harness.json",
    [string]$CompilerPath = "",
    [int]$Runs = 5,
    [int]$Warmup = 1
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $Root

$InvariantCulture = [System.Globalization.CultureInfo]::InvariantCulture

function Write-Log {
    param([string]$Message)
    if (-not $Quiet) { Write-Host $Message }
}

function Normalize-Path {
    param([string]$PathValue)
    return ($PathValue -replace "/", "\")
}

function Get-FullPath {
    param([string]$PathValue)

    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return ""
    }

    $normalized = Normalize-Path $PathValue
    if ([System.IO.Path]::IsPathRooted($normalized)) {
        return [System.IO.Path]::GetFullPath($normalized)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $Root $normalized))
}

function Get-BenchmarkMettleExe {
    param([object]$Bench)

    if ($null -ne $Bench.mettle_exe -and -not [string]::IsNullOrWhiteSpace([string]$Bench.mettle_exe)) {
        return Normalize-Path ([string]$Bench.mettle_exe)
    }

    if ($null -ne $Bench.meth_exe -and -not [string]::IsNullOrWhiteSpace([string]$Bench.meth_exe)) {
        return Normalize-Path ([string]$Bench.meth_exe)
    }

    throw "Benchmark '$($Bench.name)' does not define mettle_exe."
}

function Get-BenchmarkMettleSource {
    param([object]$Bench)

    if ($null -ne $Bench.mettle_source -and -not [string]::IsNullOrWhiteSpace([string]$Bench.mettle_source)) {
        return Normalize-Path ([string]$Bench.mettle_source)
    }

    if ($null -ne $Bench.meth_source -and -not [string]::IsNullOrWhiteSpace([string]$Bench.meth_source)) {
        return Normalize-Path ([string]$Bench.meth_source)
    }

    return [System.IO.Path]::ChangeExtension((Get-BenchmarkMettleExe -Bench $Bench), ".mettle")
}

function Get-BenchmarkCSource {
    param([object]$Bench)

    if ($null -ne $Bench.c_source -and -not [string]::IsNullOrWhiteSpace([string]$Bench.c_source)) {
        return Normalize-Path ([string]$Bench.c_source)
    }

    if ($null -eq $Bench.c_exe -or [string]::IsNullOrWhiteSpace([string]$Bench.c_exe)) {
        throw "Benchmark '$($Bench.name)' does not define c_source or c_exe."
    }

    $cExe = Normalize-Path ([string]$Bench.c_exe)
    if ($cExe -match "_c\.exe$") {
        return ($cExe -replace "_c\.exe$", ".c")
    }

    return [System.IO.Path]::ChangeExtension($cExe, ".c")
}

function Parse-TimeUs {
    param([string]$Output)

    if ([string]::IsNullOrWhiteSpace($Output)) { return $null }

    if ($Output -match "(?im)^\s*Time:\s*([0-9]+)\s*us\b") {
        return [double]$Matches[1]
    }

    $msPatterns = @(
        "(?im)^\s*Time:\s*([0-9]+(?:\.[0-9]+)?)\s*ms\b",
        "(?im)^\s*Elapsed(?:\s+time)?\s*:\s*([0-9]+(?:\.[0-9]+)?)\s*ms\b"
    )

    foreach ($pattern in $msPatterns) {
        if ($Output -match $pattern) {
            return [double]$Matches[1] * 1000.0
        }
    }

    return $null
}

function Invoke-CapturedProcess {
    param(
        [string]$FilePath,
        [string[]]$Arguments = @(),
        [string]$WorkingDirectory = ""
    )

    $previousLocation = Get-Location
    $previousErrorActionPreference = $ErrorActionPreference
    if (-not [string]::IsNullOrWhiteSpace($WorkingDirectory)) {
        Set-Location $WorkingDirectory
    }

    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    try {
        $ErrorActionPreference = "Continue"
        $output = & $FilePath @Arguments 2>&1 | Out-String
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousErrorActionPreference
        $stopwatch.Stop()
        Set-Location $previousLocation
    }

    return [pscustomobject][ordered]@{
        ExitCode = $exitCode
        Output = $output
        ElapsedMs = [double]$stopwatch.Elapsed.TotalMilliseconds
    }
}

function Get-Median {
    param([double[]]$Values)

    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }

    $sorted = @($Values | Sort-Object)
    $middle = [int]($sorted.Count / 2)
    if (($sorted.Count % 2) -eq 1) {
        return [double]$sorted[$middle]
    }

    return ([double]$sorted[$middle - 1] + [double]$sorted[$middle]) / 2.0
}

function Format-Number {
    param(
        [object]$Value,
        [int]$Digits = 2
    )

    if ($null -eq $Value) { return "" }
    return ([double]$Value).ToString("F$Digits", $InvariantCulture)
}

function Format-BenchTime {
    param([double]$Microseconds)

    if ($null -eq $Microseconds) { return "FAIL" }

    $us = [uint64][math]::Round([double]$Microseconds, 0)
    $ms = [double]$Microseconds / 1000.0
    $msDigits = if ($ms -ge 100.0) { 2 } elseif ($ms -ge 1.0) { 3 } else { 3 }
    $msText = $ms.ToString("F$msDigits", $InvariantCulture)
    $usText = $us.ToString("N0", $InvariantCulture)
    return "$msText ms ($usText us)"
}

function Format-FileSize {
    param([long]$Bytes)

    if ($null -eq $Bytes -or $Bytes -lt 0) { return "FAIL" }

    if ($Bytes -ge 1048576) {
        return ("{0} MB" -f (([double]$Bytes / 1048576.0).ToString("F2", $InvariantCulture)))
    }

    if ($Bytes -ge 1024) {
        return ("{0} KB" -f (([double]$Bytes / 1024.0).ToString("F1", $InvariantCulture)))
    }

    return ("{0} B" -f $Bytes)
}

function Format-Ratio {
    param(
        [double]$Numerator,
        [double]$Denominator
    )

    if ($null -eq $Numerator -or $null -eq $Denominator -or $Denominator -le 0) {
        return $null
    }

    return [math]::Round($Numerator / $Denominator, 2)
}

function Compile-MettleBenchmark {
    param(
        [string]$CompilerPath,
        [string]$SourcePath,
        [string]$ExePath,
        [string]$BenchmarkName
    )

    $exeDir = Split-Path -Parent $ExePath
    if (-not (Test-Path $exeDir)) {
        New-Item -ItemType Directory -Path $exeDir -Force | Out-Null
    }

    if (Test-Path $ExePath) {
        Remove-Item -LiteralPath $ExePath -Force
    }

    $args = @(
        "--build", "--emit-obj", "--linker", "internal", "--release",
        $SourcePath, "-o", $ExePath
    )
    $compile = Invoke-CapturedProcess -FilePath $CompilerPath -Arguments $args -WorkingDirectory $Root
    if ($compile.ExitCode -ne 0) {
        throw "Mettle compile failed for ${BenchmarkName}:`n$($compile.Output)"
    }

    if (-not (Test-Path $ExePath)) {
        throw "Mettle compile did not produce $ExePath"
    }

    return [double]$compile.ElapsedMs
}

function Compile-CBenchmark {
    param(
        [string]$SourcePath,
        [string]$ExePath,
        [string]$BenchmarkName
    )

    $exeDir = Split-Path -Parent $ExePath
    if (-not (Test-Path $exeDir)) {
        New-Item -ItemType Directory -Path $exeDir -Force | Out-Null
    }

    if (Test-Path $ExePath) {
        Remove-Item -LiteralPath $ExePath -Force
    }

    $args = @("-O2", "-o", $ExePath, $SourcePath, "-lkernel32")
    $compile = Invoke-CapturedProcess -FilePath "gcc" -Arguments $args -WorkingDirectory $Root
    if ($compile.ExitCode -ne 0) {
        throw "C compile failed for ${BenchmarkName}:`n$($compile.Output)"
    }

    if (-not (Test-Path $ExePath)) {
        throw "C compile did not produce $ExePath"
    }

    return [double]$compile.ElapsedMs
}

function Get-ExeSizeBytes {
    param([string]$ExePath)

    if (-not (Test-Path $ExePath)) {
        return $null
    }

    return (Get-Item -LiteralPath $ExePath).Length
}

function Measure-BenchmarkExe {
    param(
        [string]$ExePath,
        [int]$WarmupCount,
        [int]$RunCount
    )

    if (-not (Test-Path $ExePath)) {
        return $null
    }

    for ($i = 0; $i -lt $WarmupCount; $i++) {
        $warmup = Invoke-CapturedProcess -FilePath $ExePath
        if ($warmup.ExitCode -ne 0) {
            return $null
        }
    }

    $values = @()
    for ($i = 0; $i -lt $RunCount; $i++) {
        $run = Invoke-CapturedProcess -FilePath $ExePath
        if ($run.ExitCode -ne 0) {
            return $null
        }

        $reportedUs = Parse-TimeUs $run.Output
        if ($null -ne $reportedUs) {
            $values += [double]$reportedUs
            continue
        }

        if ($null -ne $run.ElapsedMs) {
            $values += [double]$run.ElapsedMs * 1000.0
        }
    }

    if ($values.Count -eq 0) {
        return $null
    }

    return Get-Median -Values $values
}

if ($Runs -lt 1) {
    Write-Error "-Runs must be at least 1."
    exit 1
}

if ($Warmup -lt 0) {
    Write-Error "-Warmup must be at least 0."
    exit 1
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

$compilerFullPath = if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    Get-FullPath "bin\mettle.exe"
} else {
    Get-FullPath $CompilerPath
}

if (-not (Test-Path $compilerFullPath)) {
    Write-Error "Compiler not found: $compilerFullPath"
    exit 1
}

$gccCommand = Get-Command gcc -CommandType Application -ErrorAction SilentlyContinue
if (-not $gccCommand) {
    Write-Error "gcc is required for the C benchmark builds but was not found on PATH."
    exit 1
}

$results = @()
$failed = @()

foreach ($bench in $config.benchmarks) {
    $name = [string]$bench.name
    $description = [string]$bench.description
    $mettleExe = Get-FullPath (Get-BenchmarkMettleExe -Bench $bench)
    $cExe = Get-FullPath ([string]$bench.c_exe)
    $mettleSource = Get-FullPath (Get-BenchmarkMettleSource -Bench $bench)
    $cSource = Get-FullPath (Get-BenchmarkCSource -Bench $bench)

    Write-Log "Building $name..."

    try {
        if (-not (Test-Path $mettleSource)) {
            throw "Mettle source not found: $mettleSource"
        }
        if (-not (Test-Path $cSource)) {
            throw "C source not found: $cSource"
        }

        $mettleCompileMs = Compile-MettleBenchmark -CompilerPath $compilerFullPath -SourcePath $mettleSource -ExePath $mettleExe -BenchmarkName $name
        $cCompileMs = Compile-CBenchmark -SourcePath $cSource -ExePath $cExe -BenchmarkName $name

        $mettleExeBytes = Get-ExeSizeBytes -ExePath $mettleExe
        $cExeBytes = Get-ExeSizeBytes -ExePath $cExe

        $mettleUs = Measure-BenchmarkExe -ExePath $mettleExe -WarmupCount $Warmup -RunCount $Runs
        $cUs = Measure-BenchmarkExe -ExePath $cExe -WarmupCount $Warmup -RunCount $Runs

        $mettleMs = if ($null -ne $mettleUs) { [math]::Round($mettleUs / 1000.0, 3) } else { $null }
        $cMs = if ($null -ne $cUs) { [math]::Round($cUs / 1000.0, 3) } else { $null }

        $runtimeRatio = Format-Ratio -Numerator $mettleUs -Denominator $cUs
        $compileRatio = Format-Ratio -Numerator $mettleCompileMs -Denominator $cCompileMs
        $sizeRatio = Format-Ratio -Numerator $mettleExeBytes -Denominator $cExeBytes

        $results += [ordered]@{
            name = $name
            description = $description
            mettle_us = if ($null -ne $mettleUs) { [uint64][math]::Round($mettleUs, 0) } else { $null }
            c_us = if ($null -ne $cUs) { [uint64][math]::Round($cUs, 0) } else { $null }
            mettle_ms = $mettleMs
            c_ms = $cMs
            relative = $runtimeRatio
            mettle_compile_ms = [math]::Round($mettleCompileMs, 0)
            c_compile_ms = [math]::Round($cCompileMs, 0)
            compile_relative = $compileRatio
            mettle_exe_bytes = $mettleExeBytes
            c_exe_bytes = $cExeBytes
            size_relative = $sizeRatio
        }

        Write-Log ("  runtime  Mettle: {0} | C: {1}{2} | median of {3} run(s), {4} warmup" -f `
            $(if ($null -ne $mettleUs) { Format-BenchTime $mettleUs } else { "FAIL" }), `
            $(if ($null -ne $cUs) { Format-BenchTime $cUs } else { "FAIL" }), `
            $(if ($null -ne $runtimeRatio) { " | $runtimeRatio`x vs C" } else { "" }), `
            $Runs, $Warmup)
        Write-Log ("  compile  Mettle: {0} ms | C: {1} ms{2}" -f `
            (Format-Number $mettleCompileMs 0), `
            (Format-Number $cCompileMs 0), `
            $(if ($null -ne $compileRatio) { " | $compileRatio`x vs C" } else { "" }))
        Write-Log ("  size     Mettle: {0} | C: {1}{2}" -f `
            (Format-FileSize $mettleExeBytes), `
            (Format-FileSize $cExeBytes), `
            $(if ($null -ne $sizeRatio) { " | $sizeRatio`x vs C" } else { "" }))
    } catch {
        Write-Log "  FAILED: $($_.Exception.Message)"
        $failed += $name
    }
}

$payload = [ordered]@{
    generated = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    harness = [ordered]@{
        config = $ConfigPath
        mode = if ($null -ne $config.mode) { [string]$config.mode } else { "mettle-vs-c" }
        runs = $Runs
        warmup = $Warmup
        compiler = $compilerFullPath
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
