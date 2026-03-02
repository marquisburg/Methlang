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
  @{ Name = "gc_alloc"; Path = "tests/test_gc_alloc.meth"; ShouldSucceed = $true },
  @{ Name = "gc_alloc_fixed"; Path = "tests/test_gc_alloc_fixed.meth"; ShouldSucceed = $true },
  @{ Name = "pointers"; Path = "tests/test_pointers.meth"; ShouldSucceed = $true },
  @{ Name = "pointer_null"; Path = "tests/test_pointer_null.meth"; ShouldSucceed = $true },
  @{
    Name          = "runtime_null_deref_check"
    Path          = "tests/test_runtime_null_deref_check.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Fatal error: Null pointer dereference", "\bcall exit\b")
  },
  @{
    Name          = "runtime_array_bounds_check"
    Path          = "tests/test_runtime_array_bounds_check.meth"
    ShouldSucceed = $true
    AsmMustMatch  = @("Fatal error: Array index out of bounds", "(\bsetl al\b|\bjge\s+ir_trap_bounds_|\bjl\s+ir_in_bounds_)")
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
