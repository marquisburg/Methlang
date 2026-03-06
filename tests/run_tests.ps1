param(
  [string]$CompilerPath = ".\bin\methlang.exe",
  [switch]$BuildCompiler,
  [switch]$SkipRuntime,
  [switch]$SkipDeterminism
)

$ErrorActionPreference = "Continue"

function Write-CaseResult {
  param(
    [string]$Name,
    [bool]$Passed,
    [string]$Reason = ""
  )

  if ($Passed) {
    Write-Host "[PASS] $Name"
  }
  else {
    if ($Reason) {
      Write-Host "[FAIL] $Name :: $Reason"
    }
    else {
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

$tmpDir = Join-Path $env:TEMP "Methlang-test-artifacts"
if (-not (Test-Path $tmpDir)) {
  New-Item -Path $tmpDir -ItemType Directory | Out-Null
}

$callManyArgsAsmMustMatch = @()
$callManyArgsAsmMustNotMatch = @()
if ($env:OS -eq "Windows_NT") {
  $callManyArgsAsmMustMatch = @(
    "(?m)^\s*mov rax, \[rbp \+ 48\]\s+; Load stack param 'e'",
    "(?m)^\s*mov rax, \[rbp \+ 56\]\s+; Load stack param 'f'",
    "(?m)^\s*mov rax, \[rbp \+ 64\]\s+; Load stack param 'g'",
    "(?m)^\s*mov rax, \[rbp \+ 72\]\s+; Load stack param 'h'"
  )
  $callManyArgsAsmMustNotMatch = @(
    "(?m)^\s*mov rax, \[rbp \+ 16\]\s+; Load stack param 'e'",
    "(?m)^\s*mov rax, \[rbp \+ 24\]\s+; Load stack param 'f'",
    "(?m)^\s*mov rax, \[rbp \+ 32\]\s+; Load stack param 'g'",
    "(?m)^\s*mov rax, \[rbp \+ 40\]\s+; Load stack param 'h'"
  )
}

$cases = @(
  @{ Name = "ok_global_int"; Path = "tests/ok_global_int.meth"; ShouldSucceed = $true },
  @{ Name = "only_struct"; Path = "tests/only_struct.meth"; ShouldSucceed = $true },
  @{ Name = "array_index"; Path = "tests/test_array_index.meth"; ShouldSucceed = $true },
  @{ Name = "control_flow"; Path = "tests/test_control_flow.meth"; ShouldSucceed = $true },
  @{ Name = "nested_switch_loop"; Path = "tests/test_nested_switch_loop.meth"; ShouldSucceed = $true },
  @{ Name = "elseif_chaining"; Path = "tests/test_elseif.meth"; ShouldSucceed = $true },
  @{ Name = "switch_const_expr"; Path = "tests/test_switch_const_expr.meth"; ShouldSucceed = $true },
  @{ Name = "switch_continue_loop"; Path = "tests/test_switch_continue_loop.meth"; ShouldSucceed = $true },
  @{
    Name            = "forward_decl"
    Path            = "tests/test_forward_decl.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @("(?m)^\s*add:\s*$")
    AsmMustNotMatch = @("(?s)(?m)^\s*add:\s*.*^\s*add:\s*")
  },
  @{ Name = "forward_decl_pointer"; Path = "tests/test_forward_decl_pointer.meth"; ShouldSucceed = $true },
  @{
    Name            = "extern_function_link_name"
    Path            = "tests/test_extern_function_link_name.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @("(?m)^\s*extern\s+puts\b", "(?m)\bcall\s+puts\b")
    AsmMustNotMatch = @("(?m)^\s*global\s+puts\b", "(?m)^\s*puts:\s*$")
  },
  @{
    Name            = "extern_global_link_name"
    Path            = "tests/test_extern_global_link_name.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @("(?m)^\s*extern\s+errno\b", "(\[\s*errno\s*\+\s*rip\s*\]|\[\s*rel\s+errno\s*\])")
    AsmMustNotMatch = @("(?m)^\s*global\s+errno\b", "(?m)^\s*errno:\s*$")
  },
  @{ Name = "cstring_alias_type"; Path = "tests/test_cstring_alias_type.meth"; ShouldSucceed = $true },
  @{ Name = "nested_function_pointer_type_annotation"; Path = "tests/test_nested_function_pointer_type_annotation.meth"; ShouldSucceed = $true },
  @{ Name = "gc_alloc"; Path = "tests/test_gc_alloc.meth"; ShouldSucceed = $true },
  @{ Name = "gc_alloc_fixed"; Path = "tests/test_gc_alloc_fixed.meth"; ShouldSucceed = $true },
  @{ Name = "pointers"; Path = "tests/test_pointers.meth"; ShouldSucceed = $true },
  @{ Name = "pointer_null"; Path = "tests/test_pointer_null.meth"; ShouldSucceed = $true },
  @{
    Name          = "runtime_null_deref_check"
    Path          = "tests/test_runtime_null_deref_check.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Fatal error: Null pointer dereference", "\bcall meth_runtime_debug_trap\b")
  },
  @{
    Name          = "runtime_array_bounds_check"
    Path          = "tests/test_runtime_array_bounds_check.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Fatal error: Array index out of bounds", "(\bsetl al\b|\bjge\s+ir_trap_bounds_|\bjl\s+ir_in_bounds_)")
  },
  @{
    Name          = "stack_trace_support"
    Path          = "tests/test_runtime_null_deref_check.meth"
    ShouldSucceed = $true
    Args          = @("-s")
    AsmMustMatch  = @(
      "extern meth_runtime_debug_install_crash_handler",
      "call meth_runtime_debug_install_crash_handler",
      "extern meth_runtime_debug_register_image",
      "extern meth_runtime_debug_trap",
      "meth_debug_functions:",
      "meth_debug_locations:"
    )
  },
  @{ Name = "pointer_param_address"; Path = "tests/test_pointer_param_address.meth"; ShouldSucceed = $true },
  @{
    Name            = "call_many_args"
    Path            = "tests/test_call_many_args.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = $callManyArgsAsmMustMatch
    AsmMustNotMatch = $callManyArgsAsmMustNotMatch
  },
  @{ Name = "import_relative_no_ext"; Path = "tests/test_import_relative_no_ext.meth"; ShouldSucceed = $true },
  @{ Name = "import_circular"; Path = "tests/test_import_circular.meth"; ShouldSucceed = $true },
  @{
    Name          = "import_include_path"
    Path          = "tests/test_import_include_path.meth"
    ShouldSucceed = $true
    Args          = @("-I", "tests/lib")
  },
  @{ Name = "import_std_core"; Path = "tests/test_import_std_core.meth"; ShouldSucceed = $true },
  @{ Name = "std_io"; Path = "tests/test_std_io.meth"; ShouldSucceed = $true },
  @{ Name = "enum"; Path = "tests/test_enum.meth"; ShouldSucceed = $true },
  @{
    Name          = "prelude"
    Path          = "tests/test_prelude.meth"
    ShouldSucceed = $true
    Args          = @("--prelude")
  },
  @{
    Name          = "string_escape_codegen"
    Path          = "tests/test_string_escape_codegen.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("(?m)^\s*db .*13,\s*10.*$", "(?m)^\s*db .*9.*34.*92.*$")
  },
  @{ Name = "char_literals"; Path = "tests/test_char_literals.meth"; ShouldSucceed = $true },
  @{ Name = "logical_ops"; Path = "tests/test_logical_ops.meth"; ShouldSucceed = $true },
  @{ Name = "strncmp_slice"; Path = "tests/test_strncmp_slice.meth"; ShouldSucceed = $true },
  @{ Name = "narrowing_conversions"; Path = "tests/test_narrowing_conversions.meth"; ShouldSucceed = $true },
  @{ Name = "signed_negation"; Path = "tests/test_signed_negation.meth"; ShouldSucceed = $true },
  @{
    Name          = "signed_division"
    Path          = "tests/test_signed_division.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("\bidiv\b")
  },
  @{ Name = "signed_comparison"; Path = "tests/test_signed_comparison.meth"; ShouldSucceed = $true },
  @{ Name = "signed_wraparound"; Path = "tests/test_signed_wraparound.meth"; ShouldSucceed = $true },
  @{ Name = "signed_arithmetic"; Path = "tests/test_signed_arithmetic.meth"; ShouldSucceed = $true },
  @{
    Name          = "sign_extension"
    Path          = "tests/test_sign_extension.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("\bmovsx\b")
  },
  @{
    Name            = "unsigned_zero_ext"
    Path            = "tests/test_unsigned_zero_ext.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @("\bmovzx\b")
    AsmMustNotMatch = @("\bmovsx\b")
  },
  @{ Name = "unsigned_division"; Path = "tests/test_unsigned_division.meth"; ShouldSucceed = $true },
  @{ Name = "mixed_signed_unsigned"; Path = "tests/test_mixed_signed_unsigned.meth"; ShouldSucceed = $true },
  @{
    Name          = "narrowing_reverify"
    Path          = "tests/test_narrowing_reverify.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("\bmovsx\b", "\bmovzx\b")
  },
  @{ Name = "stack_mixed_locals"; Path = "tests/test_stack_mixed_locals.meth"; ShouldSucceed = $true },
  @{ Name = "stack_large_struct"; Path = "tests/test_stack_large_struct.meth"; ShouldSucceed = $true },
  @{ Name = "stack_array_scalar"; Path = "tests/test_stack_array_scalar.meth"; ShouldSucceed = $true },
  @{ Name = "int64_truncate"; Path = "tests/test_int64_truncate.meth"; ShouldSucceed = $true },
  @{ Name = "string_length"; Path = "tests/test_string_length.meth"; ShouldSucceed = $true },
  @{ Name = "struct_new_zeroed"; Path = "tests/test_struct_new_zeroed.meth"; ShouldSucceed = $true },
  @{ Name = "struct_field_offset"; Path = "tests/test_struct_field_offset.meth"; ShouldSucceed = $true },
  @{
    Name          = "import_exported"
    Path          = "tests/test_import_exported.meth"
    ShouldSucceed = $true
    Args          = @("-I", "tests/lib")
  },
  @{
    Name          = "import_enum_switch"
    Path          = "tests/test_import_enum_switch.meth"
    ShouldSucceed = $true
    Args          = @("-I", "tests/lib")
  },
  @{ Name = "extern_signed_param"; Path = "tests/test_extern_signed_param.meth"; ShouldSucceed = $true },
  @{ Name = "extern_signed_return"; Path = "tests/test_extern_signed_return.meth"; ShouldSucceed = $true },
  @{ Name = "extern_cstring"; Path = "tests/test_extern_cstring.meth"; ShouldSucceed = $true },

  # ABI tests (MS x64 on Windows; patterns may need adjustment for SysV/Linux)
  @{
    Name          = "abi_int4_regs"
    Path          = "tests/test_abi_int4_regs.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter 'a' arrived in register rcx", "Parameter 'b' arrived in register rdx", "Parameter 'c' arrived in register r8", "Parameter 'd' arrived in register r9")
  },
  @{
    Name          = "abi_int_stack"
    Path          = "tests/test_abi_int_stack.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter 'e' arrived on stack", "Parameter 'f' arrived on stack", "\[rsp \+ \d+\]|\[rbp \+ \d+\]")
  },
  @{
    Name          = "abi_return_int"
    Path          = "tests/test_abi_return_int.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("\bmov (eax|rax),")
  },
  @{
    Name          = "abi_return_int64"
    Path          = "tests/test_abi_return_int64.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("\bmov rax,")
  },
  @{
    Name          = "abi_float_args"
    Path          = "tests/test_abi_float_args.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter 'a' arrived in register xmm0", "Parameter 'b' arrived in register xmm1")
  },
  @{
    Name          = "abi_float_return"
    Path          = "tests/test_abi_float_return.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @(
      "Float return value in xmm0|xmm0.*return",
      "(?s); IR call: get_pi \(0 args\).*call get_pi.*movq rax, xmm0"
    )
  },
  @{
    Name            = "abi_float_symbol_args"
    Path            = "tests/test_abi_float_symbol_args.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @(
      "(?s); IR call: sum5f \(5 args\).*movq xmm0, rax",
      "(?s); IR call: sum5f \(5 args\).*movq xmm1, rax",
      "(?s); IR call: sum5f \(5 args\).*movq xmm2, rax",
      "(?s); IR call: sum5f \(5 args\).*movq xmm3, rax",
      "(?s); IR call: sum5f \(5 args\).*mov \[rsp \+ 32\], rax"
    )
    AsmMustNotMatch = @(
      "(?s); IR call: sum5f \(5 args\).*mov rcx, rax",
      "(?s); IR call: sum5f \(5 args\).*mov rdx, rax",
      "(?s); IR call: sum5f \(5 args\).*mov r8, rax",
      "(?s); IR call: sum5f \(5 args\).*mov r9, rax"
    )
  },
  @{
    Name          = "abi_mixed_args"
    Path          = "tests/test_abi_mixed_args.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter.*arrived in register (rcx|rdx|r8|r9|xmm0)")
  },
  @{
    Name          = "abi_shadow_space"
    Path          = "tests/test_abi_shadow_space.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("sub rsp, 32|Shadow space")
  },
  @{
    Name          = "abi_prologue"
    Path          = "tests/test_abi_prologue.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("push rbp", "mov rbp, rsp")
  },
  @{
    Name          = "abi_pointer_arg"
    Path          = "tests/test_abi_pointer_arg.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter 'p' arrived in register rcx|mov \[rbp.*\], rcx")
  },
  @{
    Name          = "abi_extern_calling_convention"
    Path          = "tests/test_abi_extern_calling_convention.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("extern ext_check", "\bcall ext_check\b")
  },
  @{ Name = "abi_callee_saved"; Path = "tests/test_abi_callee_saved.meth"; ShouldSucceed = $true },
  @{ Name = "abi_stack_alignment"; Path = "tests/test_abi_stack_alignment.meth"; ShouldSucceed = $true },
  @{
    Name          = "abi_float4_args"
    Path          = "tests/test_abi_float4_args.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("xmm0", "xmm1", "xmm2", "xmm3")
  },
  @{
    Name          = "abi_float_stack"
    Path          = "tests/test_abi_float_stack.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter 'e' arrived on stack|movsd \[rsp")
  },
  @{ Name = "abi_void_return"; Path = "tests/test_abi_void_return.meth"; ShouldSucceed = $true },
  @{
    Name          = "abi_small_int_args"
    Path          = "tests/test_abi_small_int_args.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Parameter.*arrived in register (rcx|rdx)")
  },
  @{ Name = "abi_nested_calls"; Path = "tests/test_abi_nested_calls.meth"; ShouldSucceed = $true },
  @{ Name = "abi_indirect_call"; Path = "tests/test_abi_indirect_call.meth"; ShouldSucceed = $true },

  @{ Name = "stress_integrated"; Path = "tests/test_stress_integrated.meth"; ShouldSucceed = $true },
  @{ Name = "bitwise"; Path = "tests/test_bitwise.meth"; ShouldSucceed = $true },
  @{ Name = "modulo"; Path = "tests/test_modulo.meth"; ShouldSucceed = $true },
  @{ Name = "logical_not"; Path = "tests/test_logical_not.meth"; ShouldSucceed = $true },
  @{
    Name           = "optimize_ir_passes"
    Path           = "tests/test_optimize_ir_passes.meth"
    ShouldSucceed  = $true
    Args           = @("-O")
    AsmMustNotMatch = @("\bcall cold_path\b")
    IrMustMatch    = @("ASSIGN .* <- 42")
    IrMustNotMatch = @("BRANCH_ZERO 0 ->", "CALL .*cold_path\(")
  },
  @{
    Name          = "opt_dead_temp"
    Path          = "tests/test_opt_dead_temp.meth"
    ShouldSucceed = $true
    Args          = @("-O")
    IrMustNotMatch = @("ASSIGN %t[0-9]+ <- 123456")
  },
  @{
    Name          = "opt_symbol_temp_forwarding"
    Path          = "tests/test_opt_symbol_temp_forwarding.meth"
    ShouldSucceed = $true
    Args          = @("-O")
    IrMustNotMatch = @("ASSIGN %t[0-9]+ <- @x")
    IrMustMatch   = @("BRANCH_ZERO @x ->")
  },
  @{
    Name            = "release_size_mode"
    Path            = "tests/test_optimize_ir_passes.meth"
    ShouldSucceed   = $true
    Args            = @("--release")
    AsmMustNotMatch = @("(?m)^\s*;", "\bcall cold_path\b", "(?m)^\s*global\s+cold_path\b")
  },
  @{ Name = "string_concat"; Path = "tests/test_string_concat.meth"; ShouldSucceed = $true },
  @{ Name = "defer_single"; Path = "tests/test_defer_single.meth"; ShouldSucceed = $true },
  @{ Name = "defer_lifo"; Path = "tests/test_defer_lifo.meth"; ShouldSucceed = $true },
  @{ Name = "defer_nested"; Path = "tests/test_defer_nested_control_flow.meth"; ShouldSucceed = $true },
  @{ Name = "defer_early_return"; Path = "tests/test_defer_early_return.meth"; ShouldSucceed = $true },
  @{
    Name          = "defer_block_exit"
    Path          = "tests/test_defer_block_exit.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("(?s)global main.*?main:.*?; IR call: inner_defer.*?; IR call: after_block.*?; IR call: outer_defer")
  },
  @{
    Name          = "defer_if_else_branch_exit"
    Path          = "tests/test_defer_if_else_branch_exit.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @(
      "(?s); IR call: then_body.*?; IR call: then_defer",
      "(?s); IR call: else_body.*?; IR call: else_defer",
      "(?s); IR call: after_if"
    )
  },
  @{
    Name          = "defer_loop_iteration"
    Path          = "tests/test_defer_loop_iteration.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @(
      "(?s); IR call: iter_body.*?; IR call: iter_defer.*?\bjmp\b"
    )
  },
  @{
    Name          = "errdefer_runs_on_error"
    Path          = "tests/test_errdefer_runs_on_error.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @(
      "(?s)global main\s*(\r\n|\n)\s*(\r\n|\n)main:.*?ir_errdefer_ok_\d+:",
      "(?s)global main\s*(\r\n|\n)\s*(\r\n|\n)main:.*?; IR call: err \(0 args\).*?ir_errdefer_ok_\d+:",
      "(?s)global main\s*(\r\n|\n)\s*(\r\n|\n)main:.*?; IR call: ok \(0 args\).*?ir_errdefer_ok_\d+:"
    )
  },
  @{
    Name            = "errdefer_skipped_on_success"
    Path            = "tests/test_errdefer_skipped_on_success.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @(
      "(?s)global main\s*(\r\n|\n)\s*(\r\n|\n)main:.*?ir_errdefer_ok_\d+:.*?; IR call: ok \(0 args\)"
    )
    AsmMustNotMatch = @(
      "(?s)global main\s*(\r\n|\n)\s*(\r\n|\n)main:.*?ir_errdefer_ok_\d+:.*?; IR call: err \(0 args\)"
    )
  },
  @{
    Name          = "errdefer_multiple_returns"
    Path          = "tests/test_errdefer_multiple_returns.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @(
      "(?s); IR call: err.*?; IR call: ok",
      "(?s)errdefer_ok.*?; IR call: ok"
    )
  },
  @{ Name = "web_server_import"; Path = "web/server.meth"; ShouldSucceed = $true },

  # New errdefer tests
  @{ Name = "test_cast_expression"; Path = "tests/test_cast_expression.meth"; ShouldSucceed = $true },
  @{ Name = "errdefer_interleaved_with_defer"; Path = "tests/test_errdefer_interleaved_with_defer.meth"; ShouldSucceed = $true },
  @{ Name = "errdefer_block_exit"; Path = "tests/test_errdefer_block_exit.meth"; ShouldSucceed = $true },
  @{ Name = "errdefer_nested_if_else"; Path = "tests/test_errdefer_nested_if_else.meth"; ShouldSucceed = $true },
  @{ Name = "errdefer_loop_with_break_continue"; Path = "tests/test_errdefer_loop_with_break_continue.meth"; ShouldSucceed = $true },
  @{ Name = "errdefer_top_level"; Path = "tests/test_errdefer_top_level.meth"; ShouldSucceed = $false; Pattern = "Defer statement outside of a function|Errdefer statement outside of a function" },
  @{ Name = "defer_block_statement"; Path = "tests/test_defer_block_statement.meth"; ShouldSucceed = $true },
  @{ Name = "errdefer_assignment_statement"; Path = "tests/test_errdefer_assignment_statement.meth"; ShouldSucceed = $true },
  @{
    Name            = "errdefer_implicit_fallthrough"
    Path            = "tests/test_errdefer_implicit_fallthrough.meth"
    ShouldSucceed   = $true
    AsmMustMatch    = @(
      "\bcall ok\b"
    )
    AsmMustNotMatch = @(
      "\bcall err\b"
    )
  },
  @{ Name = "defer_complex_interleaving"; Path = "tests/test_defer_complex_interleaving.meth"; ShouldSucceed = $true },
  @{
    Name            = "warn_gc_escape_extern"
    Path            = "tests/test_warn_gc_escape_extern.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("Managed pointer passed to extern function 'sink' may escape GC visibility")
  },
  @{
    Name            = "warn_recv_buffer_extent"
    Path            = "tests/test_warn_recv_buffer_extent.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("recv length 8192 exceeds tracked allocation 4096 bytes for 'buf'")
  },
  @{
    Name             = "no_warn_recv_within_extent"
    Path             = "tests/test_no_warn_recv_within_extent.meth"
    ShouldSucceed    = $true
    OutputMustNotMatch = @("recv length .* exceeds tracked allocation")
  },
  @{
    Name            = "warn_memcpy_src_extent"
    Path            = "tests/test_warn_memcpy_src_extent.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("memcpy length 200 exceeds known source extent 128 bytes")
  },
  @{
    Name            = "warn_memcpy_dst_extent"
    Path            = "tests/test_warn_memcpy_dst_extent.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("memcpy length 200 exceeds known destination extent 128 bytes")
  },
  @{
    Name              = "no_warn_memcpy_within_extent"
    Path              = "tests/test_no_warn_memcpy_within_extent.meth"
    ShouldSucceed     = $true
    OutputMustNotMatch = @("memcpy length .* exceeds known (destination|source) extent")
  },
  @{
    Name            = "warn_memmove_src_extent"
    Path            = "tests/test_warn_memmove_src_extent.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("memmove length 200 exceeds known source extent 128 bytes")
  },
  @{
    Name            = "warn_memmove_dst_extent_offset"
    Path            = "tests/test_warn_memmove_dst_extent_offset.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("memmove length 220 exceeds known destination extent 192 bytes")
  },
  @{
    Name              = "no_warn_memmove_within_extent_offset"
    Path              = "tests/test_no_warn_memmove_within_extent_offset.meth"
    ShouldSucceed     = $true
    OutputMustNotMatch = @("memmove length .* exceeds known (destination|source) extent")
  },
  @{
    Name            = "warn_cast_alignment_violation"
    Path            = "tests/test_warn_cast_alignment_violation.meth"
    ShouldSucceed   = $true
    OutputMustMatch = @("Cast to int64\* may violate required 8-byte alignment")
  },
  @{
    Name              = "no_warn_cast_alignment_ok"
    Path              = "tests/test_no_warn_cast_alignment_ok.meth"
    ShouldSucceed     = $true
    OutputMustNotMatch = @("Cast to int64\* may violate required 8-byte alignment")
  },

  @{ Name = "err_unknown_char"; Path = "tests/err_unknown_char.meth"; ShouldSucceed = $false; Pattern = "Lexical error|error" },
  @{ Name = "err_unknown_fnptr_return_type"; Path = "tests/err_unknown_fnptr_return_type.meth"; ShouldSucceed = $false; Pattern = "Unknown type|no_such_type" },
  @{ Name = "err_invalid_hex"; Path = "tests/err_invalid_hex.meth"; ShouldSucceed = $false; Pattern = "Invalid hexadecimal literal" },
  @{ Name = "err_invalid_bin"; Path = "tests/err_invalid_bin.meth"; ShouldSucceed = $false; Pattern = "Invalid binary literal" },
  @{ Name = "err_missing_brace"; Path = "tests/err_missing_brace.meth"; ShouldSucceed = $false },
  @{ Name = "err_undefined_var"; Path = "tests/err_undefined_var.meth"; ShouldSucceed = $false; Pattern = "Undefined variable" },
  @{ Name = "err_top_level_return"; Path = "tests/err_top_level_return.meth"; ShouldSucceed = $false; Pattern = "Return statement outside of a function|Unsupported top-level construct in declaration context" },
  @{ Name = "err_break_outside_loop"; Path = "tests/err_break_outside_loop.meth"; ShouldSucceed = $false; Pattern = "'break' can only be used inside a loop or switch" },
  @{ Name = "err_continue_in_switch"; Path = "tests/err_continue_in_switch.meth"; ShouldSucceed = $false; Pattern = "'continue' can only be used inside a loop" },
  @{ Name = "err_switch_duplicate_case"; Path = "tests/err_switch_duplicate_case.meth"; ShouldSucceed = $false; Pattern = "Duplicate case value|duplicate case" },
  @{ Name = "err_switch_nonconst_case"; Path = "tests/err_switch_nonconst_case.meth"; ShouldSucceed = $false; Pattern = "compile-time integer constant expression" },
  @{ Name = "err_forward_decl_mismatch"; Path = "tests/err_forward_decl_mismatch.meth"; ShouldSucceed = $false; Pattern = "does not match existing declaration" },
  @{ Name = "err_forward_decl_pointer_mismatch"; Path = "tests/err_forward_decl_pointer_mismatch.meth"; ShouldSucceed = $false; Pattern = "does not match existing declaration" },
  @{ Name = "err_extern_var_initializer"; Path = "tests/err_extern_var_initializer.meth"; ShouldSucceed = $false; Pattern = "Extern variable declarations cannot have an initializer|Expected string literal link name after '='" },
  @{ Name = "err_extern_var_missing_type"; Path = "tests/err_extern_var_missing_type.meth"; ShouldSucceed = $false; Pattern = "Extern variable declarations require an explicit type" },
  @{ Name = "err_nonextern_link_name"; Path = "tests/err_nonextern_link_name.meth"; ShouldSucceed = $false; Pattern = "Link-name suffix is only allowed on extern declarations" },
  @{ Name = "err_extern_link_name_conflict"; Path = "tests/err_extern_link_name_conflict.meth"; ShouldSucceed = $false; Pattern = "conflicting link name" },
  @{ Name = "err_deref_non_pointer"; Path = "tests/err_deref_non_pointer.meth"; ShouldSucceed = $false; Pattern = "Dereference operator requires a pointer operand" },
  @{ Name = "err_address_of_non_lvalue"; Path = "tests/err_address_of_non_lvalue.meth"; ShouldSucceed = $false; Pattern = "Address-of operator requires an assignable expression" },
  @{ Name = "err_pointer_type_mismatch"; Path = "tests/err_pointer_type_mismatch.meth"; ShouldSucceed = $false; Pattern = "Type mismatch" },
  @{ Name = "err_use_before_init"; Path = "tests/err_use_before_init.meth"; ShouldSucceed = $false; Pattern = "before initialization" },
  @{ Name = "err_array_index_oob_const"; Path = "tests/err_array_index_oob_const.meth"; ShouldSucceed = $false; Pattern = "out of bounds" },
  @{ Name = "err_array_index_oob_const_negative"; Path = "tests/err_array_index_oob_const_negative.meth"; ShouldSucceed = $false; Pattern = "out of bounds" },
  @{ Name = "err_null_deref_const"; Path = "tests/err_null_deref_const.meth"; ShouldSucceed = $false; Pattern = "Null pointer dereference" },
  @{ Name = "err_codegen_member_expr"; Path = "tests/err_codegen_member_expr.meth"; ShouldSucceed = $false },
  @{ Name = "err_function_arg_count"; Path = "tests/err_function_arg_count.meth"; ShouldSucceed = $false; Pattern = "expects .* arguments, got" },
  @{ Name = "err_function_arg_type"; Path = "tests/err_function_arg_type.meth"; ShouldSucceed = $false; Pattern = "Type mismatch" },
  @{ Name = "err_member_on_non_struct"; Path = "tests/err_member_on_non_struct.meth"; ShouldSucceed = $false; Pattern = "Cannot access field on non-struct type" },
  @{ Name = "err_switch_multiple_default"; Path = "tests/err_switch_multiple_default.meth"; ShouldSucceed = $false; Pattern = "Only one default case is allowed|only contain one default clause" },
  @{ Name = "err_return_type_mismatch"; Path = "tests/err_return_type_mismatch.meth"; ShouldSucceed = $false; Pattern = "Type mismatch" },
  @{ Name = "err_defer_top_level"; Path = "tests/err_defer_top_level.meth"; ShouldSucceed = $false; Pattern = "Defer statement outside of a function" },
  @{
    Name          = "err_import_private"
    Path          = "tests/err_import_private.meth"
    ShouldSucceed = $false
    Pattern       = "Undefined variable|not visible|private_func"
    Args          = @("-I", "tests/lib")
  },
  @{
    Name          = "err_import_chain"
    Path          = "tests/test_import_chain_error.meth"
    ShouldSucceed = $false
    Pattern       = "Could not resolve|import chain"
  }
)

$total = 0
$failed = 0

foreach ($case in $cases) {
  $caseName = $case.Name
  try {
    $total++
    $outFile = Join-Path $tmpDir ("{0}.s" -f $case.Name)
    if (Test-Path $outFile) {
      Remove-Item -Path $outFile -Force -ErrorAction SilentlyContinue
    }

    $caseArgs = @()
    if ($case.ContainsKey("Args") -and $case.Args) {
      $caseArgs = @($case.Args)
    }

    $output = & $CompilerPath @caseArgs $case.Path -o $outFile 2>&1 | Out-String
    $exitCode = $LASTEXITCODE

    $passed = $true
    $reason = ""

    if ($case.ShouldSucceed) {
      if ($exitCode -ne 0) {
        $passed = $false
        $reason = "Expected success, got exit code $exitCode"
      }
      else {
        $requiredAsmPatterns = @()
        $forbiddenAsmPatterns = @()
        $requiredOutputPatterns = @()
        $forbiddenOutputPatterns = @()
        $requiredIrPatterns = @()
        $forbiddenIrPatterns = @()
        if ($case.ContainsKey("AsmMustMatch") -and $case.AsmMustMatch) {
          $requiredAsmPatterns = @($case.AsmMustMatch)
        }
        if ($case.ContainsKey("AsmMustNotMatch") -and $case.AsmMustNotMatch) {
          $forbiddenAsmPatterns = @($case.AsmMustNotMatch)
        }
        if ($case.ContainsKey("OutputMustMatch") -and $case.OutputMustMatch) {
          $requiredOutputPatterns = @($case.OutputMustMatch)
        }
        if ($case.ContainsKey("OutputMustNotMatch") -and $case.OutputMustNotMatch) {
          $forbiddenOutputPatterns = @($case.OutputMustNotMatch)
        }
        if ($case.ContainsKey("IrMustMatch") -and $case.IrMustMatch) {
          $requiredIrPatterns = @($case.IrMustMatch)
        }
        if ($case.ContainsKey("IrMustNotMatch") -and $case.IrMustNotMatch) {
          $forbiddenIrPatterns = @($case.IrMustNotMatch)
        }

        $asmCheck = Test-AssemblyOutput -AsmPath $outFile `
          -RequiredPatterns $requiredAsmPatterns `
          -ForbiddenPatterns $forbiddenAsmPatterns
        if (-not $asmCheck.Passed) {
          $passed = $false
          $reason = $asmCheck.Reason
        }
        else {
          foreach ($pattern in $requiredOutputPatterns) {
            if ([string]::IsNullOrWhiteSpace($pattern)) {
              continue
            }
            if ($output -notmatch $pattern) {
              $passed = $false
              $reason = "Compiler output missing required pattern '$pattern'"
              break
            }
          }
        }
        if ($passed) {
          foreach ($pattern in $forbiddenOutputPatterns) {
            if ([string]::IsNullOrWhiteSpace($pattern)) {
              continue
            }
            if ($output -match $pattern) {
              $passed = $false
              $reason = "Compiler output matched forbidden pattern '$pattern'"
              break
            }
          }
        }
        if ($passed -and (($requiredIrPatterns.Count -gt 0) -or ($forbiddenIrPatterns.Count -gt 0))) {
          $irFile = "$outFile.ir"
          if (-not (Test-Path $irFile)) {
            $passed = $false
            $reason = "IR output file not produced"
          }
          else {
            $irText = Get-Content -Path $irFile -Raw

            foreach ($pattern in $requiredIrPatterns) {
              if ([string]::IsNullOrWhiteSpace($pattern)) {
                continue
              }
              if ($irText -notmatch $pattern) {
                $passed = $false
                $reason = "IR output missing required pattern '$pattern'"
                break
              }
            }

            if ($passed) {
              foreach ($pattern in $forbiddenIrPatterns) {
                if ([string]::IsNullOrWhiteSpace($pattern)) {
                  continue
                }
                if ($irText -match $pattern) {
                  $passed = $false
                  $reason = "IR output matched forbidden pattern '$pattern'"
                  break
                }
              }
            }
          }
        }
        if ($passed -and -not $SkipDeterminism) {
          $outFile2 = Join-Path $tmpDir ("{0}.second.s" -f $case.Name)
          if (Test-Path $outFile2) {
            Remove-Item -Path $outFile2 -Force -ErrorAction SilentlyContinue
          }

          $output2 = & $CompilerPath @caseArgs $case.Path -o $outFile2 2>&1 | Out-String
          $exitCode2 = $LASTEXITCODE
          if ($exitCode2 -ne 0) {
            $passed = $false
            $reason = "Determinism compile failed with exit code $exitCode2"
            if ($output2) {
              $output = $output + [Environment]::NewLine + $output2
            }
          }
          else {
            $hash1 = (Get-FileHash -Algorithm SHA256 -Path $outFile).Hash
            $hash2 = (Get-FileHash -Algorithm SHA256 -Path $outFile2).Hash
            if ($hash1 -ne $hash2) {
              $passed = $false
              $reason = "Determinism check failed: outputs differ between identical runs"
            }
          }
        }
      }
    }
    else {
      if ($exitCode -eq 0) {
        $passed = $false
        $reason = "Expected failure, got success"
      }
      elseif ($case.ContainsKey("Pattern") -and $case.Pattern) {
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
    }
    else {
      Write-CaseResult -Name $case.Name -Passed $true
    }
  }
  catch {
    $failed++
    Write-CaseResult -Name $caseName -Passed $false -Reason $_.Exception.Message
  }
}

# Function pointer test: compile, assemble, link, and run
$total++
try {
  $fpAsm = Join-Path $tmpDir "test_function_pointer.s"
  $fpObj = Join-Path $tmpDir "test_function_pointer.o"
  $fpGc = Join-Path $tmpDir "test_function_pointer_gc.o"
  $fpExe = Join-Path $tmpDir "test_function_pointer.exe"

  $fpOut = & $CompilerPath tests\test_function_pointer.meth -o $fpAsm 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Function pointer compile failed: $fpOut"
  }

  & nasm -f win64 $fpAsm -o $fpObj 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Function pointer NASM assembly failed"
  }

  & gcc -c src\runtime\gc.c -o $fpGc -Isrc 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Function pointer gc.c compile failed"
  }

  & gcc -nostartfiles $fpObj $fpGc -o $fpExe -lkernel32 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Function pointer link failed (use -nostartfiles like web server)"
  }

  $fpResult = & $fpExe 2>&1
  if ($LASTEXITCODE -ne 1) {
    throw "Function pointer test exited with $LASTEXITCODE (expected 1)"
  }

  Write-CaseResult -Name "function_pointer" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "function_pointer" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend test: emit COFF object directly, then build and run
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_return_const.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_return_const.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_return_const.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object compile did not produce an object file"
  }

  $objSymbols = & objdump -t $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object symbol dump failed"
  }
  if ($objSymbols -notmatch "(?m)\bmain\b") {
    throw "Direct object symbol table did not contain main"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_return_const.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 7) {
    throw "Direct object executable exited with $LASTEXITCODE (expected 7)"
  }

  Write-CaseResult -Name "direct_object_return_const" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_return_const" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend relocation test: internal call lowered to REL32 relocation
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_call_return.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_call_return.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_call_return.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object call compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object call compile did not produce an object file"
  }

  $relocs = & objdump -r $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object relocation dump failed"
  }
  if ($relocs -notmatch "IMAGE_REL_AMD64_REL32\s+callee") {
    throw "Direct object relocation table did not contain a REL32 call to callee"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_call_return.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object call build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object call build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 5) {
    throw "Direct object call executable exited with $LASTEXITCODE (expected 5)"
  }

  Write-CaseResult -Name "direct_object_call_return" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_call_return" -Passed $false -Reason $_.Exception.Message
}

# COFF reader test: parse Methlang and GCC-produced COFF objects
$total++
try {
  $coffReaderExe = Join-Path $tmpDir "coff_reader_test.exe"
  $basicObjPath = Join-Path $tmpDir "coff_reader_basic.obj"
  $relocObjPath = Join-Path $tmpDir "coff_reader_reloc.obj"
  $longObjPath = Join-Path $tmpDir "coff_reader_long.obj"
  $gccSourcePath = Join-Path $tmpDir "coff_reader_gcc_input.c"
  $gccObjPath = Join-Path $tmpDir "coff_reader_gcc_input.o"

  $compileHarness = & gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE tests\coff_reader_test.c src\linker\coff_reader.c -Isrc -o $coffReaderExe 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "COFF reader harness compile failed: $compileHarness"
  }

  $basicOut = & $CompilerPath --emit-obj tests\test_direct_object_return_const.meth -o $basicObjPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "COFF reader basic object compile failed: $basicOut"
  }

  $relocOut = & $CompilerPath --emit-obj tests\test_direct_object_call_return.meth -o $relocObjPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "COFF reader relocation object compile failed: $relocOut"
  }

  $longOut = & $CompilerPath --emit-obj tests\test_direct_object_long_symbol_name.meth -o $longObjPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "COFF reader long-symbol object compile failed: $longOut"
  }

  @'
int gcc_reader_helper_symbol_name(void) {
  return 11;
}

int gcc_reader_entry_symbol_name(void) {
  return gcc_reader_helper_symbol_name();
}
'@ | Set-Content -Path $gccSourcePath

  $gccOut = & gcc -c $gccSourcePath -o $gccObjPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "COFF reader GCC object compile failed: $gccOut"
  }

  $coffOut = & $coffReaderExe $basicObjPath $relocObjPath $longObjPath $gccObjPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "COFF reader verification failed: $coffOut"
  }

  Write-CaseResult -Name "coff_reader" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "coff_reader" -Passed $false -Reason $_.Exception.Message
}

# Linker symbol resolution test: merge sections, resolve externals, and reject invalid symbol graphs
$total++
try {
  $symbolResolveExe = Join-Path $tmpDir "symbol_resolve_test.exe"
  $fnEntryObj = Join-Path $tmpDir "linker_merge_entry.obj"
  $fnProviderObj = Join-Path $tmpDir "linker_merge_provider.obj"
  $dataEntryObj = Join-Path $tmpDir "linker_merge_data_entry.obj"
  $dataProviderObj = Join-Path $tmpDir "linker_merge_data_provider.obj"
  $bssEntryObj = Join-Path $tmpDir "linker_merge_bss_entry.obj"
  $bssProviderObj = Join-Path $tmpDir "linker_merge_bss_provider.obj"
  $dupAObj = Join-Path $tmpDir "linker_duplicate_a.obj"
  $dupBObj = Join-Path $tmpDir "linker_duplicate_b.obj"
  $unresolvedObj = Join-Path $tmpDir "linker_unresolved_entry.obj"

  $compileHarness = & gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE tests\symbol_resolve_test.c src\linker\coff_reader.c src\linker\symbol_resolve.c src\codegen\binary_emitter.c -Isrc -Isrc\codegen -o $symbolResolveExe 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Symbol-resolve harness compile failed: $compileHarness"
  }

  $cases = @(
    @{ Path = "tests\test_linker_merge_entry.meth"; Out = $fnEntryObj; Label = "function-entry" },
    @{ Path = "tests\test_linker_merge_provider.meth"; Out = $fnProviderObj; Label = "function-provider" },
    @{ Path = "tests\test_linker_merge_data_entry.meth"; Out = $dataEntryObj; Label = "data-entry" },
    @{ Path = "tests\test_linker_merge_data_provider.meth"; Out = $dataProviderObj; Label = "data-provider" },
    @{ Path = "tests\test_linker_merge_bss_entry.meth"; Out = $bssEntryObj; Label = "bss-entry" },
    @{ Path = "tests\test_linker_merge_bss_provider.meth"; Out = $bssProviderObj; Label = "bss-provider" },
    @{ Path = "tests\test_linker_duplicate_a.meth"; Out = $dupAObj; Label = "duplicate-a" },
    @{ Path = "tests\test_linker_duplicate_b.meth"; Out = $dupBObj; Label = "duplicate-b" },
    @{ Path = "tests\test_linker_unresolved_entry.meth"; Out = $unresolvedObj; Label = "unresolved-entry" }
  )

  foreach ($case in $cases) {
    $objOut = & $CompilerPath --emit-obj $case.Path -o $case.Out 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
      throw "Symbol-resolve $($case.Label) object compile failed: $objOut"
    }
    if (-not (Test-Path $case.Out)) {
      throw "Symbol-resolve $($case.Label) object compile did not produce an object file"
    }
  }

  $resolveOut = & $symbolResolveExe $fnEntryObj $fnProviderObj $dataEntryObj $dataProviderObj $bssEntryObj $bssProviderObj $dupAObj $dupBObj $unresolvedObj $tmpDir 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Symbol-resolve verification failed: $resolveOut"
  }

  Write-CaseResult -Name "symbol_resolve" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "symbol_resolve" -Passed $false -Reason $_.Exception.Message
}

# Linker relocation test: apply merged-image relocations for REL32, ADDR64, ADDR32NB, and SECREL
$total++
try {
  $relocationExe = Join-Path $tmpDir "relocation_test.exe"

  $compileHarness = & gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE tests\relocation_test.c src\linker\coff_reader.c src\linker\symbol_resolve.c src\linker\relocation.c src\codegen\binary_emitter.c -Isrc -Isrc\codegen -o $relocationExe 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Relocation harness compile failed: $compileHarness"
  }

  $relocationOut = & $relocationExe $tmpDir 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Relocation verification failed: $relocationOut"
  }

  Write-CaseResult -Name "relocation" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "relocation" -Passed $false -Reason $_.Exception.Message
}

# PE emitter test: write a minimal PE32+ image, verify headers/sections, and run it
$total++
try {
  $peEmitterExe = Join-Path $tmpDir "pe_emitter_test.exe"

  $compileHarness = & gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE tests\pe_emitter_test.c src\linker\coff_reader.c src\linker\symbol_resolve.c src\linker\relocation.c src\linker\pe_emitter.c src\linker\import_lib.c src\codegen\binary_emitter.c -Isrc -Isrc\codegen -o $peEmitterExe 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "PE-emitter harness compile failed: $compileHarness"
  }

  $peEmitterOut = & $peEmitterExe $tmpDir 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "PE-emitter verification failed: $peEmitterOut"
  }

  Write-CaseResult -Name "pe_emitter" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "pe_emitter" -Passed $false -Reason $_.Exception.Message
}

# Internal linker basic test: direct object build uses native PE emission for default imports
$total++
try {
  $exePath = Join-Path $tmpDir "internal_link_return_const.exe"

  $buildOut = & $CompilerPath --build --emit-obj --linker internal tests\test_direct_object_return_const.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Internal linker basic build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Internal linker basic build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 7) {
    throw "Internal linker basic executable exited with $LASTEXITCODE (expected 7)"
  }

  Write-CaseResult -Name "internal_link_basic" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "internal_link_basic" -Passed $false -Reason $_.Exception.Message
}

# Internal linker extra-library test: --link-arg -lws2_32 resolves imports via the native linker
$total++
try {
  $exePath = Join-Path $tmpDir "internal_link_ws2_32.exe"

  $buildOut = & $CompilerPath --build --emit-obj --linker internal tests\test_internal_link_ws2_32.meth -o $exePath --link-arg -lws2_32 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Internal linker ws2_32 build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Internal linker ws2_32 build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Internal linker ws2_32 executable exited with $LASTEXITCODE (expected 0)"
  }

  Write-CaseResult -Name "internal_link_ws2_32" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "internal_link_ws2_32" -Passed $false -Reason $_.Exception.Message
}

# Auto linker PATH isolation test: auto mode should succeed with only NASM on PATH
$total++
try {
  $exePath = Join-Path $tmpDir "auto_link_internal_only.exe"
  $wrapperDir = Join-Path $tmpDir "phase6_auto_path_bin"
  $wrapperPath = Join-Path $wrapperDir "nasm.cmd"
  $compilerFullPath = (Resolve-Path $CompilerPath).Path
  $system32Dir = Join-Path $env:SystemRoot "System32"
  $gccBinDir = Split-Path -Parent ((Get-Command gcc -CommandType Application -ErrorAction Stop).Source)
  $nasmCommand = Get-Command nasm -CommandType Application -ErrorAction Stop

  if (-not (Test-Path $wrapperDir)) {
    New-Item -Path $wrapperDir -ItemType Directory | Out-Null
  }

  Get-ChildItem -Path $gccBinDir -Filter *.dll | Copy-Item -Destination $wrapperDir -Force

  @(
    "@echo off"
    "`"$($nasmCommand.Source)`" %*"
  ) | Set-Content -Path $wrapperPath -Encoding ASCII

  $originalPath = $env:PATH
  try {
    $env:PATH = "$wrapperDir;$system32Dir"
    $buildOut = & $compilerFullPath --build --emit-obj tests\test_direct_object_return_const.meth -o $exePath 2>&1 | Out-String
  }
  finally {
    $env:PATH = $originalPath
  }

  if ($LASTEXITCODE -ne 0) {
    throw "Auto linker internal-only build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Auto linker internal-only build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 7) {
    throw "Auto linker internal-only executable exited with $LASTEXITCODE (expected 7)"
  }

  Write-CaseResult -Name "auto_link_internal_only_path" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "auto_link_internal_only_path" -Passed $false -Reason $_.Exception.Message
}

# Auto linker fallback test: a static archive should fail internally, then link via GCC
$total++
try {
  $cSourcePath = Join-Path $tmpDir "phase6_fallback_static_lib.c"
  $cObjectPath = Join-Path $tmpDir "phase6_fallback_static_lib.o"
  $libPath = Join-Path $tmpDir "phase6_fallback_static_lib.a"
  $exePath = Join-Path $tmpDir "auto_link_fallback_static_lib.exe"
  $arCommand = Get-Command ar -CommandType Application -ErrorAction SilentlyContinue
  if (-not $arCommand) {
    $arCommand = Get-Command gcc-ar -CommandType Application -ErrorAction SilentlyContinue
  }
  if (-not $arCommand) {
    throw "Static-library archiver not found (expected ar or gcc-ar)"
  }

  @'
int fallback_value(void) {
  return 42;
}
'@ | Set-Content -Path $cSourcePath -Encoding ASCII

  $gccOut = & gcc -c $cSourcePath -o $cObjectPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Static-library compile failed: $gccOut"
  }

  $arOut = & $arCommand.Source rcs $libPath $cObjectPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Static-library archive build failed: $arOut"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_auto_link_fallback_static_lib.meth -o $exePath --link-arg $libPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Auto linker fallback build failed: $buildOut"
  }
  if ($buildOut -notmatch "Internal linker failed in auto mode, falling back to external linkers") {
    throw "Auto linker fallback build did not report an internal-link failure before fallback: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Auto linker fallback build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 42) {
    throw "Auto linker fallback executable exited with $LASTEXITCODE (expected 42)"
  }

  Write-CaseResult -Name "auto_link_fallback_static_lib" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "auto_link_fallback_static_lib" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend parameter test: integer arg passed into callee home slot
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_params.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_params.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_params.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object params compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object params compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_params.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object params build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object params build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 9) {
    throw "Direct object params executable exited with $LASTEXITCODE (expected 9)"
  }

  Write-CaseResult -Name "direct_object_params" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_params" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend control-flow test: labels and conditional branches lower directly
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_control_flow.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_control_flow.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_control_flow.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object control-flow compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object control-flow compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_control_flow.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object control-flow build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object control-flow build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 11) {
    throw "Direct object control-flow executable exited with $LASTEXITCODE (expected 11)"
  }

  Write-CaseResult -Name "direct_object_control_flow" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_control_flow" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend local-slot test: locals plus call result materialization
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_abi_return_int.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_abi_return_int.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_abi_return_int.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object ABI-return-int compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object ABI-return-int compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_abi_return_int.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object ABI-return-int build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object ABI-return-int build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 1) {
    throw "Direct object ABI-return-int executable exited with $LASTEXITCODE (expected 1)"
  }

  Write-CaseResult -Name "direct_object_abi_return_int" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_abi_return_int" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend arithmetic test: locals plus unary/binary integer lowering
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_signed_arithmetic.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_signed_arithmetic.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_signed_arithmetic.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object signed-arithmetic compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object signed-arithmetic compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_signed_arithmetic.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object signed-arithmetic build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object signed-arithmetic build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 1) {
    throw "Direct object signed-arithmetic executable exited with $LASTEXITCODE (expected 1)"
  }

  Write-CaseResult -Name "direct_object_signed_arithmetic" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_signed_arithmetic" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend structured control-flow test: locals, comparisons, loops, and switch lowering
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_structured_control_flow.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_structured_control_flow.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_control_flow.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object structured control-flow compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object structured control-flow compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_control_flow.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object structured control-flow build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object structured control-flow build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 64) {
    throw "Direct object structured control-flow executable exited with $LASTEXITCODE (expected 64)"
  }

  Write-CaseResult -Name "direct_object_structured_control_flow" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_structured_control_flow" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend scalar matrix test: integer ops plus stack args
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_integer_matrix.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_integer_matrix.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_integer_matrix.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object integer-matrix compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object integer-matrix compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_integer_matrix.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object integer-matrix build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object integer-matrix build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 37) {
    throw "Direct object integer-matrix executable exited with $LASTEXITCODE (expected 37)"
  }

  Write-CaseResult -Name "direct_object_integer_matrix" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_integer_matrix" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend scalar cast test: integer truncation/extension and pointer reinterpretation
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_scalar_casts.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_scalar_casts.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_scalar_casts.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object scalar-casts compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object scalar-casts compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_scalar_casts.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object scalar-casts build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object scalar-casts build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 21) {
    throw "Direct object scalar-casts executable exited with $LASTEXITCODE (expected 21)"
  }

  Write-CaseResult -Name "direct_object_scalar_casts" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_scalar_casts" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend float/scalar coverage: Win64 float ABI plus float/int casts
$directObjectFloatCases = @(
  @{ Name = "direct_object_abi_float_return"; Path = "tests/test_abi_float_return.meth"; ExitCode = 1; Label = "float-return" },
  @{ Name = "direct_object_abi_float_args"; Path = "tests/test_abi_float_args.meth"; ExitCode = 1; Label = "float-args" },
  @{ Name = "direct_object_abi_mixed_args"; Path = "tests/test_abi_mixed_args.meth"; ExitCode = 1; Label = "mixed-args" },
  @{ Name = "direct_object_abi_float_symbol_args"; Path = "tests/test_abi_float_symbol_args.meth"; ExitCode = 1; Label = "float-symbol-args" },
  @{ Name = "direct_object_abi_float4_args"; Path = "tests/test_abi_float4_args.meth"; ExitCode = 1; Label = "float4-args" },
  @{ Name = "direct_object_abi_float_stack"; Path = "tests/test_abi_float_stack.meth"; ExitCode = 1; Label = "float-stack" },
  @{ Name = "direct_object_cast_expression"; Path = "tests/test_cast_expression.meth"; ExitCode = 0; Label = "cast-expression" }
)

foreach ($case in $directObjectFloatCases) {
  $total++
  try {
    $objPath = Join-Path $tmpDir ($case.Name + ".obj")
    $exePath = Join-Path $tmpDir ($case.Name + ".exe")

    $objOut = & $CompilerPath --emit-obj $case.Path -o $objPath 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
      throw "Direct object $($case.Label) compile failed: $objOut"
    }
    if (-not (Test-Path $objPath)) {
      throw "Direct object $($case.Label) compile did not produce an object file"
    }

    $buildOut = & $CompilerPath --build --emit-obj $case.Path -o $exePath 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) {
      throw "Direct object $($case.Label) build failed: $buildOut"
    }
    if (-not (Test-Path $exePath)) {
      throw "Direct object $($case.Label) build did not produce an executable"
    }

    & $exePath 2>&1 | Out-Null
    if ($LASTEXITCODE -ne $case.ExitCode) {
      throw "Direct object $($case.Label) executable exited with $LASTEXITCODE (expected $($case.ExitCode))"
    }

    Write-CaseResult -Name $case.Name -Passed $true
  }
  catch {
    $failed++
    Write-CaseResult -Name $case.Name -Passed $false -Reason $_.Exception.Message
  }
}

# Direct object backend globals: scalar definitions plus extern-global symbol emission
$total++
try {
  $objPath = Join-Path $tmpDir "direct_object_ok_global_int.obj"
  $exePath = Join-Path $tmpDir "direct_object_ok_global_int.exe"

  $objOut = & $CompilerPath --emit-obj tests\ok_global_int.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object ok-global-int compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object ok-global-int compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\ok_global_int.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object ok-global-int build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object ok-global-int build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 1) {
    throw "Direct object ok-global-int executable exited with $LASTEXITCODE (expected 1)"
  }

  Write-CaseResult -Name "direct_object_ok_global_int" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_ok_global_int" -Passed $false -Reason $_.Exception.Message
}

$total++
try {
  $objPath = Join-Path $tmpDir "direct_object_extern_global_link_name.obj"

  $objOut = & $CompilerPath --emit-obj tests\test_extern_global_link_name.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object extern-global-link-name compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object extern-global-link-name compile did not produce an object file"
  }

  $symbols = & objdump -t $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object extern-global-link-name symbol dump failed"
  }
  if ($symbols -notmatch "errno") {
    throw "Direct object extern-global-link-name object is missing extern symbol 'errno'"
  }

  $relocs = & objdump -r $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object extern-global-link-name relocation dump failed"
  }
  if ($relocs -notmatch "IMAGE_REL_AMD64_REL32" -or $relocs -notmatch "errno") {
    throw "Direct object extern-global-link-name object is missing REL32 relocations to 'errno'"
  }

  Write-CaseResult -Name "direct_object_extern_global_link_name" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_extern_global_link_name" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend pointer-param-address test: address of parameter slot survives load/store
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_pointer_param_address.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_pointer_param_address.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_pointer_param_address.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object pointer-param-address compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object pointer-param-address compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_pointer_param_address.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object pointer-param-address build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object pointer-param-address build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 7) {
    throw "Direct object pointer-param-address executable exited with $LASTEXITCODE (expected 7)"
  }

  Write-CaseResult -Name "direct_object_pointer_param_address" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_pointer_param_address" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend pointer-memory test: new, addr_of, load, store, and pointer args
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_pointer_memory.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_pointer_memory.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_direct_object_pointer_memory.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object pointer-memory compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object pointer-memory compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_direct_object_pointer_memory.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object pointer-memory build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object pointer-memory build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 29) {
    throw "Direct object pointer-memory executable exited with $LASTEXITCODE (expected 29)"
  }

  Write-CaseResult -Name "direct_object_pointer_memory" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_pointer_memory" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend aggregate-local test: stack-allocated struct addressed and passed by pointer
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_struct_field_offset.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_struct_field_offset.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_struct_field_offset.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object struct-field-offset compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object struct-field-offset compile did not produce an object file"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_struct_field_offset.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object struct-field-offset build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object struct-field-offset build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 1) {
    throw "Direct object struct-field-offset executable exited with $LASTEXITCODE (expected 1)"
  }

  Write-CaseResult -Name "direct_object_struct_field_offset" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_struct_field_offset" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend function-pointer test: addr_of function plus indirect call
$total++
try {
  $objPath = Join-Path $tmpDir "test_direct_object_function_pointer.obj"
  $exePath = Join-Path $tmpDir "test_direct_object_function_pointer.exe"

  $objOut = & $CompilerPath --emit-obj tests\test_function_pointer.meth -o $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object function-pointer compile failed: $objOut"
  }
  if (-not (Test-Path $objPath)) {
    throw "Direct object function-pointer compile did not produce an object file"
  }

  $relocs = & objdump -r $objPath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object function-pointer relocation dump failed"
  }
  if ($relocs -notmatch "IMAGE_REL_AMD64_REL32\s+add") {
    throw "Direct object function-pointer relocations did not contain add"
  }
  if ($relocs -notmatch "IMAGE_REL_AMD64_REL32\s+multiply") {
    throw "Direct object function-pointer relocations did not contain multiply"
  }

  $buildOut = & $CompilerPath --build --emit-obj tests\test_function_pointer.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object function-pointer build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object function-pointer build did not produce an executable"
  }

  & $exePath 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 1) {
    throw "Direct object function-pointer executable exited with $LASTEXITCODE (expected 1)"
  }

  Write-CaseResult -Name "direct_object_function_pointer" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_function_pointer" -Passed $false -Reason $_.Exception.Message
}

# Direct object backend runtime trap test: null deref lowers and links through the trap helper
$total++
try {
  $exePath = Join-Path $tmpDir "test_direct_object_runtime_null_deref.exe"

  $buildOut = & $CompilerPath --build --emit-obj tests\test_runtime_null_deref_check.meth -o $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Direct object runtime-null build failed: $buildOut"
  }
  if (-not (Test-Path $exePath)) {
    throw "Direct object runtime-null build did not produce an executable"
  }

  $runtimeOut = & $exePath 2>&1 | Out-String
  if ($LASTEXITCODE -ne 1) {
    throw "Direct object runtime-null executable exited with $LASTEXITCODE (expected 1)"
  }
  if ($runtimeOut -notmatch "Fatal error: Null pointer dereference") {
    throw "Direct object runtime-null output missing null-deref message"
  }

  Write-CaseResult -Name "direct_object_runtime_null_deref" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "direct_object_runtime_null_deref" -Passed $false -Reason $_.Exception.Message
}

# main(argc, argv) test: requires methlang_entry.o and shell32 on Windows
$total++
try {
  $avAsm = Join-Path $tmpDir "test_main_argc_argv.s"
  $avObj = Join-Path $tmpDir "test_main_argc_argv.o"
  $avGc = Join-Path $tmpDir "test_main_argc_argv_gc.o"
  $avEntry = Join-Path $tmpDir "test_main_argc_argv_entry.o"
  $avExe = Join-Path $tmpDir "test_main_argc_argv.exe"

  $avOut = & $CompilerPath tests\test_main_argc_argv.meth -o $avAsm 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "main(argc,argv) compile failed: $avOut"
  }

  & nasm -f win64 $avAsm -o $avObj 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "main(argc,argv) NASM assembly failed"
  }

  & gcc -c src\runtime\gc.c -o $avGc -Isrc 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "main(argc,argv) gc.c compile failed"
  }

  & gcc -c src\runtime\methlang_entry.c -o $avEntry -Isrc 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "main(argc,argv) methlang_entry.c compile failed"
  }

  & gcc -nostartfiles $avObj $avGc $avEntry -o $avExe -lkernel32 -lshell32 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "main(argc,argv) link failed (need -lshell32 for CommandLineToArgvW)"
  }

  $avResult = & $avExe 2>&1
  if ($LASTEXITCODE -ne 0) {
    throw "main(argc,argv) test exited with $LASTEXITCODE (expected 0)"
  }

  Write-CaseResult -Name "main_argc_argv" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "main_argc_argv" -Passed $false -Reason $_.Exception.Message
}

$total++
try {
  $nullAsm = Join-Path $tmpDir "test_runtime_null_trace.s"
  $nullObj = Join-Path $tmpDir "test_runtime_null_trace.o"
  $nullGc = Join-Path $tmpDir "test_runtime_null_trace_gc.o"
  $nullExe = Join-Path $tmpDir "test_runtime_null_trace.exe"

  $nullOut = & $CompilerPath -s tests\test_runtime_null_deref_check.meth -o $nullAsm 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime null trace compile failed: $nullOut"
  }

  & nasm -f win64 $nullAsm -o $nullObj 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime null trace NASM assembly failed"
  }

  & gcc -c src\runtime\gc.c -o $nullGc -Isrc 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime null trace gc.c compile failed"
  }

  & gcc -nostartfiles $nullObj $nullGc -o $nullExe -lkernel32 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime null trace link failed"
  }

  $nullRuntime = & $nullExe 2>&1 | Out-String
  if ($LASTEXITCODE -ne 1) {
    throw "Runtime null trace exited with $LASTEXITCODE (expected 1)"
  }
  if ($nullRuntime -notmatch "Fatal error: Null pointer dereference") {
    throw "Runtime null trace output missing null-deref message"
  }
  if ($nullRuntime -notmatch "Stack trace:") {
    throw "Runtime null trace output missing stack trace header"
  }
  if ($nullRuntime -notmatch "main") {
    throw "Runtime null trace output missing Meth frame names"
  }

  Write-CaseResult -Name "runtime_null_trace" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "runtime_null_trace" -Passed $false -Reason $_.Exception.Message
}

$total++
try {
  $avAsm = Join-Path $tmpDir "test_runtime_av_trace.s"
  $avObj2 = Join-Path $tmpDir "test_runtime_av_trace.o"
  $avGc2 = Join-Path $tmpDir "test_runtime_av_trace_gc.o"
  $avExe2 = Join-Path $tmpDir "test_runtime_av_trace.exe"

  $avTraceOut = & $CompilerPath -s tests\test_runtime_access_violation_trace.meth -o $avAsm 2>&1 | Out-String
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime access-violation trace compile failed: $avTraceOut"
  }

  & nasm -f win64 $avAsm -o $avObj2 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime access-violation trace NASM assembly failed"
  }

  & gcc -c src\runtime\gc.c -o $avGc2 -Isrc 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime access-violation trace gc.c compile failed"
  }

  & gcc -nostartfiles $avObj2 $avGc2 -o $avExe2 -lkernel32 2>&1 | Out-Null
  if ($LASTEXITCODE -ne 0) {
    throw "Runtime access-violation trace link failed"
  }

  $avRuntime = & $avExe2 2>&1 | Out-String
  if ($LASTEXITCODE -ne 1) {
    throw "Runtime access-violation trace exited with $LASTEXITCODE (expected 1)"
  }
  if ($avRuntime -notmatch "0xC0000005") {
    throw "Runtime access-violation trace output missing exception code"
  }
  if ($avRuntime -notmatch "Stack trace:") {
    throw "Runtime access-violation trace output missing stack trace header"
  }
  if ($avRuntime -notmatch "leaf_crash" -or $avRuntime -notmatch "intermediate") {
    throw "Runtime access-violation trace output missing generated frame names"
  }

  Write-CaseResult -Name "runtime_access_violation_trace" -Passed $true
}
catch {
  $failed++
  Write-CaseResult -Name "runtime_access_violation_trace" -Passed $false -Reason $_.Exception.Message
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
  }
  catch {
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
