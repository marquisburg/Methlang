#include "startup.h"

#include "internal.h"

#define STARTUP_ARGV_FRAME_SIZE 80

static int emit_getmainargs_prologue(BinaryCodeBuffer *code,
                                     BinaryCallRelocationTable *relocations) {
  size_t getmainargs_offset = 0;

  if (!binary_emit_sub_rsp_imm32(code, STARTUP_ARGV_FRAME_SIZE) ||
      !binary_emit_lea_reg_mem(code, BINARY_GP_RCX, BINARY_GP_RSP, 40) ||
      !binary_emit_lea_reg_mem(code, BINARY_GP_RDX, BINARY_GP_RSP, 48) ||
      !binary_emit_lea_reg_mem(code, BINARY_GP_R8, BINARY_GP_RSP, 56) ||
      !binary_emit_xor_reg_reg32(code, BINARY_GP_R9) ||
      !binary_emit_lea_reg_mem(code, BINARY_GP_RAX, BINARY_GP_RSP, 64) ||
      !binary_emit_mov_mem_reg(code, BINARY_GP_RSP, 32, BINARY_GP_RAX) ||
      !binary_emit_mov_reg_imm64(code, BINARY_GP_RAX, 0) ||
      !binary_emit_mov_mem_reg(code, BINARY_GP_RSP, 56, BINARY_GP_RAX) ||
      !binary_emit_mov_mem_reg(code, BINARY_GP_RSP, 64, BINARY_GP_RAX) ||
      !binary_emit_mov_mem_reg(code, BINARY_GP_RSP, 72, BINARY_GP_RAX) ||
      !binary_emit_call_placeholder(code, &getmainargs_offset) ||
      !binary_call_relocation_table_add(relocations, "__getmainargs",
                                        getmainargs_offset) ||
      !binary_emit_mov_reg_mem32(code, BINARY_GP_RCX, BINARY_GP_RSP, 40) ||
      !binary_emit_mov_reg_mem(code, BINARY_GP_RDX, BINARY_GP_RSP, 48)) {
    return 0;
  }

  return 1;
}

/* Linux/SysV self-contained program entry. The kernel transfers control to
 * `_start` with the stack laid out as:
 *     [rsp]      = argc
 *     [rsp + 8]  = argv[0]
 *     ...        = argv[argc-1], NULL, envp..., NULL
 * rsp is 16-byte aligned at entry and there is no return address. We pass argc
 * (RDI) and &argv (RSI) to main per SysV, call it, then invoke the exit(2)
 * syscall with main's return value. No libc, no CRT object. */
static int binary_write_elf_start_object(const char *path, int profile_runtime,
                                         int stack_trace_init,
                                         int main_wants_argc_argv) {
  BinaryEmitter *emitter = NULL;
  BinaryCodeBuffer code = {0};
  BinaryCallRelocationTable relocations = {0};
  size_t text_section = 0;
  size_t function_offset = 0;
  size_t crash_startup_offset = 0;
  size_t main_call_offset = 0;
  size_t report_call_offset = 0;
  int result = 1;

  if (!path) {
    return 1;
  }

  emitter = binary_emitter_create(BINARY_TARGET_FORMAT_ELF_X64);
  if (!emitter) {
    return 1;
  }

  /* Optional crash-handler init before main. It takes no args. */
  if (stack_trace_init) {
    if (!binary_emit_call_placeholder(&code, &crash_startup_offset) ||
        !binary_call_relocation_table_add(&relocations, "mettle_crash_startup",
                                          crash_startup_offset)) {
      goto cleanup;
    }
  }

  /* main(argc, argv): argc = [rsp], &argv = rsp + 8.
   * At _start, rsp % 16 == 0. The crash-startup call above (if emitted) pushed
   * and popped a return address symmetrically, so rsp is still 16-aligned here.
   * A direct `call main` with nothing else pushed satisfies SysV's "rsp % 16
   * == 0 immediately before call" rule. */
  if (main_wants_argc_argv) {
    if (!binary_emit_mov_reg_mem(&code, BINARY_GP_RDI, BINARY_GP_RSP, 0) ||
        !binary_emit_lea_reg_mem(&code, BINARY_GP_RSI, BINARY_GP_RSP, 8)) {
      goto cleanup;
    }
  }

  if (!binary_emit_call_placeholder(&code, &main_call_offset) ||
      !binary_call_relocation_table_add(&relocations, "main",
                                        main_call_offset)) {
    goto cleanup;
  }

  if (profile_runtime) {
    /* Preserve main's result (RAX) across the report call. Push keeps the
     * stack 16-aligned for the call that follows. */
    if (!binary_emit_push_reg(&code, BINARY_GP_RAX) ||
        !binary_emit_call_placeholder(&code, &report_call_offset) ||
        !binary_call_relocation_table_add(&relocations, "mettle_profile_report",
                                          report_call_offset) ||
        !binary_emit_pop_reg(&code, BINARY_GP_RAX)) {
      goto cleanup;
    }
  }

  /* exit(main_result): syscall 60, status in RDI. main returns int32 in EAX;
   * move it to EDI (zero-extends into RDI), set RAX = 60, syscall. */
  if (!binary_emit_mov_reg_reg(&code, BINARY_GP_RDI, BINARY_GP_RAX) ||
      !binary_emit_mov_reg_imm32_zero_extend(&code, BINARY_GP_RAX, 60) ||
      !binary_emit_syscall(&code)) {
    goto cleanup;
  }

  text_section = binary_emitter_get_or_create_section(
      emitter, ".text", BINARY_SECTION_TEXT, 0, BINARY_TEXT_SECTION_ALIGNMENT);
  if (text_section == (size_t)-1 ||
      !binary_emitter_align_section(emitter, text_section,
                                    BINARY_TEXT_SECTION_ALIGNMENT, 0x90)) {
    goto cleanup;
  }

  {
    BinarySection *section = binary_emitter_get_section(emitter, text_section);
    if (!section) {
      goto cleanup;
    }
    function_offset = section->size;
  }

  if (!binary_emitter_define_symbol(emitter, "_start", BINARY_SYMBOL_GLOBAL,
                                    text_section, function_offset, code.size) ||
      !binary_emitter_append_bytes(emitter, text_section, code.data, code.size,
                                   NULL) ||
      !binary_emitter_declare_external(emitter, "main")) {
    goto cleanup;
  }

  if (stack_trace_init &&
      !binary_emitter_declare_external(emitter, "mettle_crash_startup")) {
    goto cleanup;
  }
  if (profile_runtime &&
      !binary_emitter_declare_external(emitter, "mettle_profile_report")) {
    goto cleanup;
  }

  for (size_t i = 0; i < relocations.count; i++) {
    BinaryCallRelocation *relocation = &relocations.items[i];
    if (!binary_emitter_add_relocation(
            emitter, text_section,
            function_offset + relocation->displacement_offset,
            BINARY_RELOCATION_REL32, relocation->symbol_name, 0)) {
      goto cleanup;
    }
  }

  if (!binary_emitter_write_object_file(emitter, path)) {
    goto cleanup;
  }

  result = 0;

cleanup:
  binary_call_relocation_table_destroy(&relocations);
  binary_code_buffer_destroy(&code);
  binary_emitter_destroy(emitter);
  return result;
}

int binary_write_program_startup_object(const char *path, int profile_runtime,
                                        int stack_trace_init,
                                        int main_wants_argc_argv) {
  if (binary_target_format_host_default() == BINARY_TARGET_FORMAT_ELF_X64) {
    return binary_write_elf_start_object(path, profile_runtime, stack_trace_init,
                                         main_wants_argc_argv);
  }
  BinaryEmitter *emitter = NULL;
  BinaryCodeBuffer code = {0};
  BinaryCallRelocationTable relocations = {0};
  size_t text_section = 0;
  size_t function_offset = 0;
  size_t crash_startup_offset = 0;
  size_t main_call_offset = 0;
  size_t report_call_offset = 0;
  size_t exit_call_offset = 0;
  int result = 1;

  if (!path) {
    return 1;
  }

  emitter = binary_emitter_create(BINARY_TARGET_FORMAT_COFF_WIN64);
  if (!emitter) {
    return 1;
  }

  if (!binary_emit_sub_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE + 8)) {
    goto cleanup;
  }

  if (stack_trace_init) {
    if (!binary_emit_call_placeholder(&code, &crash_startup_offset) ||
        !binary_call_relocation_table_add(&relocations, "mettle_crash_startup",
                                          crash_startup_offset)) {
      goto cleanup;
    }
  }

  if (main_wants_argc_argv &&
      !emit_getmainargs_prologue(&code, &relocations)) {
    goto cleanup;
  }

  if (!binary_emit_call_placeholder(&code, &main_call_offset) ||
      !binary_call_relocation_table_add(&relocations, "main",
                                        main_call_offset)) {
    goto cleanup;
  }

  if (main_wants_argc_argv &&
      !binary_emit_add_rsp_imm32(&code, STARTUP_ARGV_FRAME_SIZE)) {
    goto cleanup;
  }

  if (profile_runtime) {
    if (!binary_emit_push_reg(&code, BINARY_GP_RAX) ||
        !binary_emit_sub_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE + 8) ||
        !binary_emit_call_placeholder(&code, &report_call_offset) ||
        !binary_call_relocation_table_add(&relocations, "mettle_profile_report",
                                          report_call_offset) ||
        !binary_emit_add_rsp_imm32(&code, BINARY_WIN64_SHADOW_SPACE_SIZE + 8) ||
        !binary_emit_pop_reg(&code, BINARY_GP_RAX)) {
      goto cleanup;
    }
  }

  if (!binary_emit_mov_reg_reg(&code, BINARY_GP_RCX, BINARY_GP_RAX) ||
      !binary_emit_call_placeholder(&code, &exit_call_offset) ||
      !binary_call_relocation_table_add(&relocations, "ExitProcess",
                                        exit_call_offset)) {
    goto cleanup;
  }

  text_section = binary_emitter_get_or_create_section(
      emitter, ".text", BINARY_SECTION_TEXT, 0, BINARY_TEXT_SECTION_ALIGNMENT);
  if (text_section == (size_t)-1 ||
      !binary_emitter_align_section(emitter, text_section,
                                    BINARY_TEXT_SECTION_ALIGNMENT, 0x90)) {
    goto cleanup;
  }

  {
    BinarySection *section = binary_emitter_get_section(emitter, text_section);
    if (!section) {
      goto cleanup;
    }
    function_offset = section->size;
  }

  if (!binary_emitter_define_symbol(emitter, "mainCRTStartup",
                                    BINARY_SYMBOL_GLOBAL, text_section,
                                    function_offset, code.size) ||
      !binary_emitter_append_bytes(emitter, text_section, code.data, code.size,
                                   NULL) ||
      !binary_emitter_declare_external(emitter, "main") ||
      !binary_emitter_declare_external(emitter, "ExitProcess")) {
    goto cleanup;
  }

  if (main_wants_argc_argv &&
      !binary_emitter_declare_external(emitter, "__getmainargs")) {
    goto cleanup;
  }

  if (stack_trace_init &&
      !binary_emitter_declare_external(emitter, "mettle_crash_startup")) {
    goto cleanup;
  }

  if (profile_runtime &&
      !binary_emitter_declare_external(emitter, "mettle_profile_report")) {
    goto cleanup;
  }

  for (size_t i = 0; i < relocations.count; i++) {
    BinaryCallRelocation *relocation = &relocations.items[i];
    if (!binary_emitter_add_relocation(
            emitter, text_section,
            function_offset + relocation->displacement_offset,
            BINARY_RELOCATION_REL32, relocation->symbol_name, 0)) {
      goto cleanup;
    }
  }

  if (!binary_emitter_write_object_file(emitter, path)) {
    goto cleanup;
  }

  result = 0;

cleanup:
  binary_call_relocation_table_destroy(&relocations);
  binary_code_buffer_destroy(&code);
  binary_emitter_destroy(emitter);
  return result;
}
