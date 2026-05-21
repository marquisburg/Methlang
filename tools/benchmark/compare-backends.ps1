# Compare Mettle COFF (default) vs legacy NASM assembly backends.
#
# For each benchmark in docs/benchmarks/harness.json, compiles the same source
# with both backends, then runs the executables in alternating order.
#
# Usage:
#   .\tools\benchmark\compare-backends.ps1
#   .\tools\benchmark\compare-backends.ps1 -Runs 7 -Benchmark collatz,fib
#   .\tools\benchmark\compare-backends.ps1 -BuildCompiler
#
# Outputs under bin/benchmark-backends/:
#   backend-summary.md
#   backend-summary.csv
#   backend-runs.csv
#   backend-compare.json

param(
    [string]$CompilerPath = "",
    [string]$ConfigPath = "docs/benchmarks/harness.json",
    [string]$OutputRoot = "bin/benchmark-backends",
    [string[]]$Benchmark = @(),
    [int]$Runs = 5,
    [switch]$BuildCompiler,
    [switch]$Quiet
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

function Assert-PathInside {
    param(
        [string]$ParentPath,
        [string]$ChildPath
    )

    $parent = [System.IO.Path]::GetFullPath($ParentPath).TrimEnd("\", "/") + "\"
    $child = [System.IO.Path]::GetFullPath($ChildPath).TrimEnd("\", "/") + "\"

    if (-not $child.StartsWith($parent, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to remove path outside benchmark output root: $ChildPath"
    }
}

function New-CleanDirectory {
    param(
        [string]$PathValue,
        [string]$GuardRoot
    )

    if (Test-Path $PathValue) {
        Assert-PathInside -ParentPath $GuardRoot -ChildPath $PathValue
        Remove-Item -LiteralPath $PathValue -Recurse -Force
    }

    New-Item -ItemType Directory -Path $PathValue -Force | Out-Null
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
        ElapsedMs = [math]::Round($stopwatch.Elapsed.TotalMilliseconds, 0)
    }
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
            return [double]$Matches[1]
        }
    }

    return $null
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

function Get-Mean {
    param([double[]]$Values)

    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }

    $sum = 0.0
    foreach ($value in $Values) {
        $sum += [double]$value
    }

    return $sum / [double]$Values.Count
}

function Get-BenchmarkSource {
    param([object]$Bench)

    if ($null -ne $Bench.source -and -not [string]::IsNullOrWhiteSpace([string]$Bench.source)) {
        return Normalize-Path ([string]$Bench.source)
    }

    if ($null -eq $Bench.mettle_exe -and ($null -eq $Bench.meth_exe -or [string]::IsNullOrWhiteSpace([string]$Bench.meth_exe))) {
        throw "Benchmark '$($Bench.name)' does not define source or mettle_exe."
    }

    $exePath = if ($null -ne $Bench.mettle_exe -and -not [string]::IsNullOrWhiteSpace([string]$Bench.mettle_exe)) {
        [string]$Bench.mettle_exe
    } else {
        [string]$Bench.meth_exe
    }

    return [System.IO.Path]::ChangeExtension((Normalize-Path $exePath), ".mettle")
}

function Get-BackendBuildArgs {
    param([string]$BackendName)

    switch ($BackendName) {
        "coff" {
            return @("--build", "--linker", "internal", "--release")
        }
        "asm" {
            return @("--build", "--emit-asm", "--linker", "internal", "--release")
        }
        default {
            throw "Unknown backend '$BackendName'."
        }
    }
}

function Compile-BenchmarkBackend {
    param(
        [string]$CompilerPath,
        [string]$SourcePath,
        [string]$ExePath,
        [string]$BackendName,
        [string]$BenchmarkName
    )

    $exeDir = Split-Path -Parent $ExePath
    if (-not (Test-Path $exeDir)) {
        New-Item -ItemType Directory -Path $exeDir -Force | Out-Null
    }

    if (Test-Path $ExePath) {
        Remove-Item -LiteralPath $ExePath -Force
    }

    $args = @(Get-BackendBuildArgs -BackendName $BackendName) + @($SourcePath, "-o", $ExePath)
    $compile = Invoke-CapturedProcess -FilePath $CompilerPath -Arguments $args -WorkingDirectory $Root
    if ($compile.ExitCode -ne 0) {
        throw "$BackendName compile failed for ${BenchmarkName}:`n$($compile.Output)"
    }

    if (-not (Test-Path $ExePath)) {
        throw "$BackendName compile did not produce $ExePath"
    }

    return [double]$compile.ElapsedMs
}

function Run-BenchmarkExe {
    param(
        [string]$ExePath,
        [string]$BackendName,
        [string]$BenchmarkName,
        [int]$RunIndex
    )

    $run = Invoke-CapturedProcess -FilePath $ExePath -WorkingDirectory $Root
    if ($run.ExitCode -ne 0) {
        throw "$BackendName run $RunIndex failed for $BenchmarkName with exit code $($run.ExitCode):`n$($run.Output)"
    }

    $reportedMs = Parse-TimeMs $run.Output
    $usedMs = $reportedMs
    if ($null -eq $usedMs) {
        $usedMs = [double]$run.ElapsedMs
    }

    return [pscustomobject][ordered]@{
        benchmark = $BenchmarkName
        backend = $BackendName
        run = $RunIndex
        reported_ms = $reportedMs
        wall_ms = [double]$run.ElapsedMs
        used_ms = [double]$usedMs
    }
}

function Format-Number {
    param(
        [object]$Value,
        [int]$Digits = 2
    )

    if ($null -eq $Value) { return "" }
    return ([double]$Value).ToString("F$Digits", [System.Globalization.CultureInfo]::InvariantCulture)
}

function Write-MarkdownReport {
    param(
        [string]$PathValue,
        [object[]]$SummaryRows,
        [object]$Payload
    )

    $lines = New-Object System.Collections.Generic.List[string]
    $lines.Add("# Mettle Backend Benchmark Compare")
    $lines.Add("")
    $lines.Add("- Generated: $($Payload.generated)")
    $lines.Add("- Compiler: ``$($Payload.compiler.path)``")
    $lines.Add("- COFF backend: ``--build --linker internal --release`` (default)")
    $lines.Add("- ASM backend: ``--build --emit-asm --linker internal --release``")
    $lines.Add("- Runs per benchmark: $($Payload.runs_per_benchmark)")
    if ($null -ne $Payload.geomean_runtime_ratio) {
        $lines.Add("- Geomean runtime ratio (asm/coff): $((Format-Number $Payload.geomean_runtime_ratio 3))x")
    }
    if ($null -ne $Payload.geomean_compile_ratio) {
        $lines.Add("- Geomean compile ratio (asm/coff): $((Format-Number $Payload.geomean_compile_ratio 3))x")
    }
    $lines.Add("")
    $lines.Add("Runtime ratio > 1 means COFF is faster; compile ratio > 1 means COFF compiles faster.")
    $lines.Add("")
    $lines.Add("| Benchmark | COFF runtime ms | ASM runtime ms | Runtime ratio | COFF compile ms | ASM compile ms | Compile ratio |")
    $lines.Add("| --- | ---: | ---: | ---: | ---: | ---: | ---: |")

    foreach ($row in $SummaryRows) {
        $lines.Add("| $($row.benchmark) | $(Format-Number $row.coff_median_ms 2) | $(Format-Number $row.asm_median_ms 2) | $(Format-Number $row.runtime_ratio 3)x | $(Format-Number $row.coff_compile_ms 0) | $(Format-Number $row.asm_compile_ms 0) | $(Format-Number $row.compile_ratio 3)x |")
    }

    $reportDir = Split-Path -Parent $PathValue
    if (-not (Test-Path $reportDir)) {
        New-Item -ItemType Directory -Path $reportDir -Force | Out-Null
    }
    $lines | Set-Content -Path $PathValue -Encoding UTF8
}

if ($Runs -lt 1) {
    throw "-Runs must be at least 1."
}

$outputFullPath = Get-FullPath $OutputRoot
if (-not (Test-Path $outputFullPath)) {
    New-Item -ItemType Directory -Path $outputFullPath -Force | Out-Null
}

$configFullPath = Get-FullPath $ConfigPath
if (-not (Test-Path $configFullPath)) {
    throw "Harness config not found: $configFullPath"
}

$config = Get-Content -Raw -Path $configFullPath | ConvertFrom-Json
if ($null -eq $config -or $null -eq $config.benchmarks -or $config.benchmarks.Count -eq 0) {
    throw "Harness config has no benchmarks: $configFullPath"
}

$benchmarkFilter = @()
foreach ($entry in $Benchmark) {
    foreach ($namePart in ([string]$entry).Split(",")) {
        $trimmed = $namePart.Trim()
        if (-not [string]::IsNullOrWhiteSpace($trimmed)) {
            $benchmarkFilter += $trimmed
        }
    }
}

$selectedBenchmarks = @()
foreach ($bench in $config.benchmarks) {
    $name = [string]$bench.name
    if ($benchmarkFilter.Count -gt 0 -and -not ($benchmarkFilter -contains $name)) {
        continue
    }
    $selectedBenchmarks += $bench
}

if ($selectedBenchmarks.Count -eq 0) {
    throw "No benchmarks selected."
}

if ($BuildCompiler) {
    Write-Log "Building compiler..."
    $build = Invoke-CapturedProcess -FilePath ".\build.bat" -WorkingDirectory $Root
    if ($build.ExitCode -ne 0) {
        throw "Compiler build failed:`n$($build.Output)"
    }
}

$compilerFullPath = if ([string]::IsNullOrWhiteSpace($CompilerPath)) {
    Get-FullPath "bin\mettle.exe"
} else {
    Get-FullPath $CompilerPath
}

if (-not (Test-Path $compilerFullPath)) {
    throw "Compiler not found: $compilerFullPath"
}

$nasmCommand = Get-Command nasm -CommandType Application -ErrorAction SilentlyContinue
if (-not $nasmCommand) {
    throw "NASM is required for the asm backend comparison but was not found on PATH."
}

$summaryRows = @()
$runRows = @()
$failures = @()

$coffExeDir = Join-Path $outputFullPath "exe\coff"
$asmExeDir = Join-Path $outputFullPath "exe\asm"
New-CleanDirectory -PathValue $coffExeDir -GuardRoot $outputFullPath
New-CleanDirectory -PathValue $asmExeDir -GuardRoot $outputFullPath

foreach ($bench in $selectedBenchmarks) {
    $name = [string]$bench.name
    $description = [string]$bench.description
    $sourceRelative = Get-BenchmarkSource -Bench $bench
    $sourcePath = Get-FullPath $sourceRelative
    if (-not (Test-Path $sourcePath)) {
        throw "Benchmark source not found for ${name}: $sourcePath"
    }

    Write-Log ""
    Write-Log "Benchmark $name..."

    $coffExe = Join-Path $coffExeDir "$name.exe"
    $asmExe = Join-Path $asmExeDir "$name.exe"

    try {
        $coffCompileMs = Compile-BenchmarkBackend -CompilerPath $compilerFullPath -SourcePath $sourcePath -ExePath $coffExe -BackendName "coff" -BenchmarkName $name
        $asmCompileMs = Compile-BenchmarkBackend -CompilerPath $compilerFullPath -SourcePath $sourcePath -ExePath $asmExe -BackendName "asm" -BenchmarkName $name

        $coffValues = @()
        $asmValues = @()
        for ($i = 1; $i -le $Runs; $i++) {
            if (($i % 2) -eq 1) {
                $coffRun = Run-BenchmarkExe -ExePath $coffExe -BackendName "coff" -BenchmarkName $name -RunIndex $i
                $asmRun = Run-BenchmarkExe -ExePath $asmExe -BackendName "asm" -BenchmarkName $name -RunIndex $i
            } else {
                $asmRun = Run-BenchmarkExe -ExePath $asmExe -BackendName "asm" -BenchmarkName $name -RunIndex $i
                $coffRun = Run-BenchmarkExe -ExePath $coffExe -BackendName "coff" -BenchmarkName $name -RunIndex $i
            }

            $runRows += $coffRun
            $runRows += $asmRun
            $coffValues += [double]$coffRun.used_ms
            $asmValues += [double]$asmRun.used_ms
        }

        $coffMedian = Get-Median -Values $coffValues
        $asmMedian = Get-Median -Values $asmValues
        $coffMean = Get-Mean -Values $coffValues
        $asmMean = Get-Mean -Values $asmValues

        $runtimeRatio = $null
        $runtimeDeltaPercent = $null
        if ($null -ne $coffMedian -and $null -ne $asmMedian -and $coffMedian -gt 0) {
            $runtimeRatio = [double]$asmMedian / [double]$coffMedian
            $runtimeDeltaPercent = (([double]$asmMedian - [double]$coffMedian) / [double]$coffMedian) * 100.0
        }

        $compileRatio = $null
        if ($null -ne $coffCompileMs -and $null -ne $asmCompileMs -and $coffCompileMs -gt 0) {
            $compileRatio = [double]$asmCompileMs / [double]$coffCompileMs
        }

        $summaryRow = [pscustomobject][ordered]@{
            benchmark = $name
            description = $description
            source = $sourceRelative
            coff_median_ms = $coffMedian
            asm_median_ms = $asmMedian
            coff_mean_ms = $coffMean
            asm_mean_ms = $asmMean
            runtime_ratio = $runtimeRatio
            runtime_delta_percent = $runtimeDeltaPercent
            coff_compile_ms = $coffCompileMs
            asm_compile_ms = $asmCompileMs
            compile_ratio = $compileRatio
        }
        $summaryRows += $summaryRow

        Write-Log ("  runtime  coff {0} ms | asm {1} ms | ratio {2}x" -f (Format-Number $coffMedian 2), (Format-Number $asmMedian 2), (Format-Number $runtimeRatio 3))
        Write-Log ("  compile  coff {0} ms | asm {1} ms | ratio {2}x" -f (Format-Number $coffCompileMs 0), (Format-Number $asmCompileMs 0), (Format-Number $compileRatio 3))
    } catch {
        $message = $_.Exception.Message
        $failures += [pscustomobject][ordered]@{
            benchmark = $name
            error = $message
        }
        Write-Log "  FAILED: $message"
    }
}

function Get-Geomean {
    param([double[]]$Values)

    if ($null -eq $Values -or $Values.Count -eq 0) { return $null }

    $logSum = 0.0
    foreach ($value in $Values) {
        if ([double]$value -le 0) { return $null }
        $logSum += [math]::Log([double]$value)
    }

    return [math]::Exp($logSum / [double]$Values.Count)
}

$runtimeRatios = @()
$compileRatios = @()
foreach ($row in $summaryRows) {
    if ($null -ne $row.runtime_ratio -and [double]$row.runtime_ratio -gt 0) {
        $runtimeRatios += [double]$row.runtime_ratio
    }
    if ($null -ne $row.compile_ratio -and [double]$row.compile_ratio -gt 0) {
        $compileRatios += [double]$row.compile_ratio
    }
}

$geomeanRuntimeRatio = Get-Geomean -Values $runtimeRatios
$geomeanCompileRatio = Get-Geomean -Values $compileRatios

$summaryCsvPath = Join-Path $outputFullPath "backend-summary.csv"
$runsCsvPath = Join-Path $outputFullPath "backend-runs.csv"
$jsonPath = Join-Path $outputFullPath "backend-compare.json"
$markdownPath = Join-Path $outputFullPath "backend-summary.md"

$summaryRows | Export-Csv -Path $summaryCsvPath -NoTypeInformation -Encoding UTF8
$runRows | Export-Csv -Path $runsCsvPath -NoTypeInformation -Encoding UTF8

$payload = [ordered]@{
    generated = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    config = $ConfigPath
    output_root = $OutputRoot
    runs_per_benchmark = $Runs
    compiler = [ordered]@{
        path = $compilerFullPath
    }
    backends = [ordered]@{
        coff = "--build --linker internal --release"
        asm = "--build --emit-asm --linker internal --release"
    }
    geomean_runtime_ratio = $geomeanRuntimeRatio
    geomean_compile_ratio = $geomeanCompileRatio
    summary = $summaryRows
    runs = $runRows
    failures = $failures
}

$payload | ConvertTo-Json -Depth 10 | Set-Content -Path $jsonPath -Encoding UTF8
Write-MarkdownReport -PathValue $markdownPath -SummaryRows $summaryRows -Payload $payload

Write-Log ""
Write-Log "Wrote $markdownPath"
Write-Log "Wrote $jsonPath"
if ($null -ne $geomeanRuntimeRatio) {
    Write-Log ("Geomean runtime ratio (asm/coff): {0}x" -f (Format-Number $geomeanRuntimeRatio 3))
}
if ($null -ne $geomeanCompileRatio) {
    Write-Log ("Geomean compile ratio (asm/coff): {0}x" -f (Format-Number $geomeanCompileRatio 3))
}
if ($failures.Count -gt 0) {
    Write-Log ("Failures: {0}" -f ($failures.Count))
    exit 1
}

exit 0
