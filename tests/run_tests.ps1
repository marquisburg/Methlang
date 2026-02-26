param(
  [string]$CompilerPath = ".\bin\methasm.exe",
  [switch]$BuildCompiler,
  [switch]$SkipRuntime,
  [switch]$SkipDeterminism
)

$ErrorActionPreference = "Stop"

function Write-CaseResult {
  param(
    [string]$Name,
    [bool]$Passed,
    [string]$Reason = ""
  )

  if ($Passed) {
    Write-Host "[PASS] $Name"
  } else {
    if ($Reason) {
      Write-Host "[FAIL] $Name :: $Reason"
    } else {
      Write-Host "[FAIL] $Name"
    }
  }
}

function Test-AssemblyOutput {
  param(
    [string]$AsmPath,
    [string[]]$RequiredPatterns = @(),
    [string[]]$ForbiddenPatterns = @()
  )

  if (-not (Test-Path $AsmPath)) {
    return @{ Passed = $false; Reason = "Output file not produced" }
  }

  $asmText = Get-Content -Path $AsmPath -Raw
  if ([string]::IsNullOrWhiteSpace($asmText)) {
    return @{ Passed = $false; Reason = "Output assembly is empty" }
  }

  if ($asmText -match "\%[a-z]{2,3}" -or $asmText -match "\$[0-9]+") {
    return @{ Passed = $false; Reason = "Found AT&T-style syntax fragments in generated assembly" }
  }

  if ($asmText -notmatch "(?m)^\s*section\s+\.text\b") {
    return @{ Passed = $false; Reason = "Missing text section in generated assembly" }
  }

  if ($asmText -notmatch "(?m)^\s*global\s+") {
    return @{ Passed = $false; Reason = "Missing global symbol in generated assembly" }
  }

  foreach ($pattern in $RequiredPatterns) {
    if ([string]::IsNullOrWhiteSpace($pattern)) {
      continue
    }
    if ($asmText -notmatch $pattern) {
      return @{ Passed = $false; Reason = "Assembly missing required pattern '$pattern'" }
    }
  }

  foreach ($pattern in $ForbiddenPatterns) {
    if ([string]::IsNullOrWhiteSpace($pattern)) {
      continue
    }
    if ($asmText -match $pattern) {
      return @{ Passed = $false; Reason = "Assembly matched forbidden pattern '$pattern'" }
    }
  }

  return @{ Passed = $true; Reason = "" }
}

if ($BuildCompiler) {
  Write-Host "Building compiler..."
  & .\build.bat
  if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed."
    exit 1
  }
}

if (-not (Test-Path $CompilerPath)) {
  Write-Error "Compiler not found at '$CompilerPath'."
  exit 1
}

$tmpDir = Join-Path $env:TEMP "methasm-test-artifacts"
if (-not (Test-Path $tmpDir)) {
  New-Item -Path $tmpDir -ItemType Directory | Out-Null
}

$cases = @(
  @{ Name="ok_global_int"; Path="tests/ok_global_int.masm"; ShouldSucceed=$true },
  @{ Name="only_struct"; Path="tests/only_struct.masm"; ShouldSucceed=$true },
  @{ Name="array_index"; Path="tests/test_array_index.masm"; ShouldSucceed=$true },
  @{ Name="control_flow"; Path="tests/test_control_flow.masm"; ShouldSucceed=$true },
  @{ Name="nested_switch_loop"; Path="tests/test_nested_switch_loop.masm"; ShouldSucceed=$true },
  @{ Name="switch_const_expr"; Path="tests/test_switch_const_expr.masm"; ShouldSucceed=$true },
  @{ Name="switch_continue_loop"; Path="tests/test_switch_continue_loop.masm"; ShouldSucceed=$true },
  @{
    Name="forward_decl"
    Path="tests/test_forward_decl.masm"
    ShouldSucceed=$true
    AsmMustMatch=@("(?m)^\s*add:\s*$")
    AsmMustNotMatch=@("(?s)(?m)^\s*add:\s*.*^\s*add:\s*")
  },
  @{ Name="forward_decl_pointer"; Path="tests/test_forward_decl_pointer.masm"; ShouldSucceed=$true },
  @{
    Name="extern_function_link_name"
    Path="tests/test_extern_function_link_name.masm"
    ShouldSucceed=$true
    AsmMustMatch=@("(?m)^\s*extern\s+puts\b", "(?m)\bcall\s+puts\b")
    AsmMustNotMatch=@("(?m)^\s*global\s+puts\b", "(?m)^\s*puts:\s*$")
  },
  @{
    Name="extern_global_link_name"
    Path="tests/test_extern_global_link_name.masm"
    ShouldSucceed=$true
    AsmMustMatch=@("(?m)^\s*extern\s+errno\b", "\[\s*errno\s*\+\s*rip\s*\]")
    AsmMustNotMatch=@("(?m)^\s*global\s+errno\b", "(?m)^\s*errno:\s*$")
  },
  @{ Name="cstring_alias_type"; Path="tests/test_cstring_alias_type.masm"; ShouldSucceed=$true },
  @{ Name="gc_alloc"; Path="tests/test_gc_alloc.masm"; ShouldSucceed=$true },
  @{ Name="gc_alloc_fixed"; Path="tests/test_gc_alloc_fixed.masm"; ShouldSucceed=$true },
  @{ Name="pointers"; Path="tests/test_pointers.masm"; ShouldSucceed=$true },
  @{ Name="pointer_null"; Path="tests/test_pointer_null.masm"; ShouldSucceed=$true },
  @{ Name="pointer_param_address"; Path="tests/test_pointer_param_address.masm"; ShouldSucceed=$true },
  @{ Name="call_many_args"; Path="tests/test_call_many_args.masm"; ShouldSucceed=$true },
  @{
    Name="string_escape_codegen"
    Path="tests/test_string_escape_codegen.masm"
    ShouldSucceed=$true
    AsmMustMatch=@("(?m)^\s*db .*13,\s*10.*$", "(?m)^\s*db .*9.*34.*92.*$")
  },
  @{ Name="narrowing_conversions"; Path="tests/test_narrowing_conversions.masm"; ShouldSucceed=$true },
  @{ Name="stress_integrated"; Path="tests/test_stress_integrated.masm"; ShouldSucceed=$true },

  @{ Name="err_unknown_char"; Path="tests/err_unknown_char.masm"; ShouldSucceed=$false; Pattern="Lexical error|error" },
  @{ Name="err_invalid_hex"; Path="tests/err_invalid_hex.masm"; ShouldSucceed=$false; Pattern="Invalid hexadecimal literal" },
  @{ Name="err_invalid_bin"; Path="tests/err_invalid_bin.masm"; ShouldSucceed=$false; Pattern="Invalid binary literal" },
  @{ Name="err_missing_brace"; Path="tests/err_missing_brace.masm"; ShouldSucceed=$false },
  @{ Name="err_undefined_var"; Path="tests/err_undefined_var.masm"; ShouldSucceed=$false; Pattern="Undefined variable" },
  @{ Name="err_top_level_return"; Path="tests/err_top_level_return.masm"; ShouldSucceed=$false; Pattern="Return statement outside of a function|Unsupported top-level construct in declaration context" },
  @{ Name="err_break_outside_loop"; Path="tests/err_break_outside_loop.masm"; ShouldSucceed=$false; Pattern="'break' can only be used inside a loop or switch" },
  @{ Name="err_continue_in_switch"; Path="tests/err_continue_in_switch.masm"; ShouldSucceed=$false; Pattern="'continue' can only be used inside a loop" },
  @{ Name="err_switch_duplicate_case"; Path="tests/err_switch_duplicate_case.masm"; ShouldSucceed=$false; Pattern="Duplicate case value|duplicate case" },
  @{ Name="err_switch_nonconst_case"; Path="tests/err_switch_nonconst_case.masm"; ShouldSucceed=$false; Pattern="compile-time integer constant expression" },
  @{ Name="err_forward_decl_mismatch"; Path="tests/err_forward_decl_mismatch.masm"; ShouldSucceed=$false; Pattern="does not match existing declaration" },
  @{ Name="err_forward_decl_pointer_mismatch"; Path="tests/err_forward_decl_pointer_mismatch.masm"; ShouldSucceed=$false; Pattern="does not match existing declaration" },
  @{ Name="err_extern_var_initializer"; Path="tests/err_extern_var_initializer.masm"; ShouldSucceed=$false; Pattern="Extern variable declarations cannot have an initializer|Expected string literal link name after '='" },
  @{ Name="err_extern_var_missing_type"; Path="tests/err_extern_var_missing_type.masm"; ShouldSucceed=$false; Pattern="Extern variable declarations require an explicit type" },
  @{ Name="err_nonextern_link_name"; Path="tests/err_nonextern_link_name.masm"; ShouldSucceed=$false; Pattern="Link-name suffix is only allowed on extern declarations" },
  @{ Name="err_extern_link_name_conflict"; Path="tests/err_extern_link_name_conflict.masm"; ShouldSucceed=$false; Pattern="conflicting link name" },
  @{ Name="err_deref_non_pointer"; Path="tests/err_deref_non_pointer.masm"; ShouldSucceed=$false; Pattern="Dereference operator requires a pointer operand" },
  @{ Name="err_address_of_non_lvalue"; Path="tests/err_address_of_non_lvalue.masm"; ShouldSucceed=$false; Pattern="Address-of operator requires an assignable expression" },
  @{ Name="err_pointer_type_mismatch"; Path="tests/err_pointer_type_mismatch.masm"; ShouldSucceed=$false; Pattern="Type mismatch" },
  @{ Name="err_codegen_member_expr"; Path="tests/err_codegen_member_expr.masm"; ShouldSucceed=$false },
  @{ Name="err_function_arg_count"; Path="tests/err_function_arg_count.masm"; ShouldSucceed=$false; Pattern="expects .* arguments, got" },
  @{ Name="err_function_arg_type"; Path="tests/err_function_arg_type.masm"; ShouldSucceed=$false; Pattern="Type mismatch" },
  @{ Name="err_member_on_non_struct"; Path="tests/err_member_on_non_struct.masm"; ShouldSucceed=$false; Pattern="Cannot access field on non-struct type" },
  @{ Name="err_switch_multiple_default"; Path="tests/err_switch_multiple_default.masm"; ShouldSucceed=$false; Pattern="Only one default case is allowed|only contain one default clause" },
  @{ Name="err_return_type_mismatch"; Path="tests/err_return_type_mismatch.masm"; ShouldSucceed=$false; Pattern="Type mismatch" }
)

$total = 0
$failed = 0

foreach ($case in $cases) {
  $total++
  $outFile = Join-Path $tmpDir ("{0}.s" -f $case.Name)
  if (Test-Path $outFile) {
    Remove-Item -Path $outFile -Force
  }

  $output = & $CompilerPath $case.Path -o $outFile 2>&1 | Out-String
  $exitCode = $LASTEXITCODE

  $passed = $true
  $reason = ""

  if ($case.ShouldSucceed) {
    if ($exitCode -ne 0) {
      $passed = $false
      $reason = "Expected success, got exit code $exitCode"
    } else {
      $requiredAsmPatterns = @()
      $forbiddenAsmPatterns = @()
      if ($case.ContainsKey("AsmMustMatch") -and $case.AsmMustMatch) {
        $requiredAsmPatterns = @($case.AsmMustMatch)
      }
      if ($case.ContainsKey("AsmMustNotMatch") -and $case.AsmMustNotMatch) {
        $forbiddenAsmPatterns = @($case.AsmMustNotMatch)
      }

      $asmCheck = Test-AssemblyOutput -AsmPath $outFile `
        -RequiredPatterns $requiredAsmPatterns `
        -ForbiddenPatterns $forbiddenAsmPatterns
      if (-not $asmCheck.Passed) {
        $passed = $false
        $reason = $asmCheck.Reason
      } elseif (-not $SkipDeterminism) {
        $outFile2 = Join-Path $tmpDir ("{0}.second.s" -f $case.Name)
        if (Test-Path $outFile2) {
          Remove-Item -Path $outFile2 -Force
        }

        $output2 = & $CompilerPath $case.Path -o $outFile2 2>&1 | Out-String
        $exitCode2 = $LASTEXITCODE
        if ($exitCode2 -ne 0) {
          $passed = $false
          $reason = "Determinism compile failed with exit code $exitCode2"
          if ($output2) {
            $output = $output + [Environment]::NewLine + $output2
          }
        } else {
          $hash1 = (Get-FileHash -Algorithm SHA256 -Path $outFile).Hash
          $hash2 = (Get-FileHash -Algorithm SHA256 -Path $outFile2).Hash
          if ($hash1 -ne $hash2) {
            $passed = $false
            $reason = "Determinism check failed: outputs differ between identical runs"
          }
        }
      }
    }
  } else {
    if ($exitCode -eq 0) {
      $passed = $false
      $reason = "Expected failure, got success"
    } elseif ($case.ContainsKey("Pattern") -and $case.Pattern) {
      if ($output -notmatch $case.Pattern) {
        $passed = $false
        $reason = "Failure message did not match expected pattern '$($case.Pattern)'"
      }
    }
  }

  if (-not $passed) {
    $failed++
    Write-CaseResult -Name $case.Name -Passed $false -Reason $reason
    if ($output) {
      Write-Host ($output.TrimEnd())
    }
  } else {
    Write-CaseResult -Name $case.Name -Passed $true
  }
}

if (-not $SkipRuntime) {
  $total++
  try {
    $runtimeExe = "bin\gc_runtime_test.exe"
    & gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE tests\gc_runtime_test.c src\runtime\gc.c -o $runtimeExe
    if ($LASTEXITCODE -ne 0) {
      throw "Failed to compile GC runtime test"
    }

    $runtimeOutput = & $runtimeExe 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
      throw "GC runtime test exited with code $LASTEXITCODE"
    }

    if ($runtimeOutput -notmatch "GC runtime tests passed") {
      throw "GC runtime test output did not contain pass marker"
    }

    Write-CaseResult -Name "gc_runtime" -Passed $true
  } catch {
    $failed++
    Write-CaseResult -Name "gc_runtime" -Passed $false -Reason $_.Exception.Message
  }
}

Write-Host ""
Write-Host "Test summary: $($total - $failed)/$total passed"

if ($failed -ne 0) {
  exit 1
}

exit 0
