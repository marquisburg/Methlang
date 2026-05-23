#include "ir.h"
#include "../common.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define IR_OPERAND_FMT_BUFSIZE 128

IROperand ir_operand_none(void) {
  IROperand operand = {0};
  operand.kind = IR_OPERAND_NONE;
  return operand;
}

IROperand ir_operand_temp(const char *name) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_TEMP;
  operand.name = mettle_strdup(name);
  return operand;
}

IROperand ir_operand_symbol(const char *name) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_SYMBOL;
  operand.name = mettle_strdup(name);
  return operand;
}

IROperand ir_operand_int(long long value) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_INT;
  operand.int_value = value;
  return operand;
}

IROperand ir_operand_float(double value) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_FLOAT;
  operand.float_value = value;
  operand.float_bits = 64;
  return operand;
}

IROperand ir_operand_float_sized(double value, int float_bits) {
  IROperand operand = ir_operand_float(value);
  operand.float_bits = (float_bits == 32) ? 32 : 64;
  return operand;
}

IROperand ir_operand_string(const char *value) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_STRING;
  operand.name = mettle_strdup(value);
  return operand;
}

IROperand ir_operand_label(const char *name) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_LABEL;
  operand.name = mettle_strdup(name);
  return operand;
}

void ir_operand_destroy(IROperand *operand) {
  if (!operand) {
    return;
  }

  switch (operand->kind) {
  case IR_OPERAND_TEMP:
  case IR_OPERAND_SYMBOL:
  case IR_OPERAND_STRING:
  case IR_OPERAND_LABEL:
    free(operand->name);
    break;
  default:
    break;
  }

  *operand = ir_operand_none();
}

static IROperand ir_operand_clone(const IROperand *operand) {
  if (!operand) {
    return ir_operand_none();
  }

  switch (operand->kind) {
  case IR_OPERAND_TEMP: {
    IROperand copy = ir_operand_temp(operand->name);
    copy.float_bits = operand->float_bits;
    return copy;
  }
  case IR_OPERAND_SYMBOL: {
    IROperand copy = ir_operand_symbol(operand->name);
    copy.float_bits = operand->float_bits;
    return copy;
  }
  case IR_OPERAND_INT:
    return ir_operand_int(operand->int_value);
  case IR_OPERAND_FLOAT:
    return ir_operand_float_sized(operand->float_value, operand->float_bits);
  case IR_OPERAND_STRING:
    return ir_operand_string(operand->name);
  case IR_OPERAND_LABEL:
    return ir_operand_label(operand->name);
  case IR_OPERAND_NONE:
  default:
    return ir_operand_none();
  }
}

static void ir_instruction_destroy(IRInstruction *instruction) {
  if (!instruction) {
    return;
  }

  ir_operand_destroy(&instruction->dest);
  ir_operand_destroy(&instruction->lhs);
  ir_operand_destroy(&instruction->rhs);
  free(instruction->text);

  if (instruction->arguments) {
    for (size_t i = 0; i < instruction->argument_count; i++) {
      ir_operand_destroy(&instruction->arguments[i]);
    }
    free(instruction->arguments);
  }

  instruction->arguments = NULL;
  instruction->argument_count = 0;
}

static void ir_function_clear_parameters(IRFunction *function) {
  if (!function) {
    return;
  }

  if (function->parameter_names) {
    for (size_t i = 0; i < function->parameter_count; i++) {
      free(function->parameter_names[i]);
    }
    free(function->parameter_names);
  }

  if (function->parameter_types) {
    for (size_t i = 0; i < function->parameter_count; i++) {
      free(function->parameter_types[i]);
    }
    free(function->parameter_types);
  }

  function->parameter_names = NULL;
  function->parameter_types = NULL;
  function->parameter_count = 0;
}

IRFunction *ir_function_create(const char *name) {
  IRFunction *function = malloc(sizeof(IRFunction));
  if (!function) {
    return NULL;
  }

  function->name = mettle_strdup(name ? name : "<anonymous>");
  function->profile_id = IR_PROFILE_ID_NONE;
  function->parameter_names = NULL;
  function->parameter_types = NULL;
  function->parameter_count = 0;
  function->instructions = NULL;
  function->instruction_count = 0;
  function->instruction_capacity = 0;
  return function;
}

int ir_function_set_parameters(IRFunction *function, const char **parameter_names,
                               const char **parameter_types,
                               size_t parameter_count) {
  if (!function) {
    return 0;
  }

  ir_function_clear_parameters(function);

  if (parameter_count == 0) {
    return 1;
  }

  char **name_copies = calloc(parameter_count, sizeof(char *));
  char **type_copies = calloc(parameter_count, sizeof(char *));
  if (!name_copies || !type_copies) {
    free(name_copies);
    free(type_copies);
    return 0;
  }

  for (size_t i = 0; i < parameter_count; i++) {
    if (!parameter_names || !parameter_names[i]) {
      goto fail;
    }

    name_copies[i] = mettle_strdup(parameter_names[i]);
    if (!name_copies[i]) {
      goto fail;
    }

    if (parameter_types && parameter_types[i]) {
      type_copies[i] = mettle_strdup(parameter_types[i]);
      if (!type_copies[i]) {
        goto fail;
      }
    }
  }

  function->parameter_names = name_copies;
  function->parameter_types = type_copies;
  function->parameter_count = parameter_count;
  return 1;

fail:
  for (size_t i = 0; i < parameter_count; i++) {
    free(name_copies[i]);
    free(type_copies[i]);
  }
  free(name_copies);
  free(type_copies);
  return 0;
}

void ir_function_destroy(IRFunction *function) {
  if (!function) {
    return;
  }

  free(function->name);
  ir_function_clear_parameters(function);
  for (size_t i = 0; i < function->instruction_count; i++) {
    ir_instruction_destroy(&function->instructions[i]);
  }
  free(function->instructions);
  free(function);
}

int ir_function_append_instruction(IRFunction *function,
                                   const IRInstruction *instruction) {
  if (!function || !instruction) {
    return 0;
  }

  if (function->instruction_count >= function->instruction_capacity) {
    size_t new_capacity = function->instruction_capacity == 0
                              ? 64
                              : function->instruction_capacity * 2;
    IRInstruction *new_instructions =
        realloc(function->instructions, new_capacity * sizeof(IRInstruction));
    if (!new_instructions) {
      return 0;
    }
    function->instructions = new_instructions;
    function->instruction_capacity = new_capacity;
  }

  IRInstruction *slot = &function->instructions[function->instruction_count];
  *slot = *instruction;

  slot->dest = ir_operand_clone(&instruction->dest);
  slot->lhs = ir_operand_clone(&instruction->lhs);
  slot->rhs = ir_operand_clone(&instruction->rhs);
  slot->text = mettle_strdup(instruction->text);
  slot->is_float = instruction->is_float;
  slot->arguments = NULL;
  slot->argument_count = instruction->argument_count;

  if (instruction->argument_count > 0) {
    slot->arguments = malloc(instruction->argument_count * sizeof(IROperand));
    if (!slot->arguments) {
      ir_instruction_destroy(slot);
      return 0;
    }
    for (size_t i = 0; i < instruction->argument_count; i++) {
      slot->arguments[i] = ir_operand_clone(&instruction->arguments[i]);
    }
  }

  function->instruction_count++;
  return 1;
}

int ir_function_insert_instruction(IRFunction *function, size_t index,
                                   const IRInstruction *instruction) {
  if (!function || !instruction || index > function->instruction_count) {
    return 0;
  }

  if (function->instruction_count >= function->instruction_capacity) {
    size_t new_capacity = function->instruction_capacity == 0
                              ? 64
                              : function->instruction_capacity * 2;
    IRInstruction *new_instructions =
        realloc(function->instructions, new_capacity * sizeof(IRInstruction));
    if (!new_instructions) {
      return 0;
    }
    function->instructions = new_instructions;
    function->instruction_capacity = new_capacity;
  }

  if (index < function->instruction_count) {
    memmove(&function->instructions[index + 1], &function->instructions[index],
            (function->instruction_count - index) * sizeof(IRInstruction));
  }

  IRInstruction *slot = &function->instructions[index];
  memset(slot, 0, sizeof(*slot));
  slot->op = instruction->op;
  slot->location = instruction->location;
  slot->is_float = instruction->is_float;
  slot->float_bits = instruction->float_bits;
  slot->ast_ref = instruction->ast_ref;
  slot->dest = ir_operand_clone(&instruction->dest);
  slot->lhs = ir_operand_clone(&instruction->lhs);
  slot->rhs = ir_operand_clone(&instruction->rhs);
  slot->text = mettle_strdup(instruction->text);
  slot->argument_count = instruction->argument_count;
  slot->arguments = NULL;

  if (instruction->argument_count > 0) {
    slot->arguments = malloc(instruction->argument_count * sizeof(IROperand));
    if (!slot->arguments) {
      ir_instruction_destroy(slot);
      return 0;
    }
    for (size_t i = 0; i < instruction->argument_count; i++) {
      slot->arguments[i] = ir_operand_clone(&instruction->arguments[i]);
    }
  }

  function->instruction_count++;
  return 1;
}

IRProgram *ir_program_create(void) {
  IRProgram *program = malloc(sizeof(IRProgram));
  if (!program) {
    return NULL;
  }

  program->functions = NULL;
  program->function_count = 0;
  program->function_capacity = 0;
  program->profile_entries = NULL;
  program->profile_entry_count = 0;
  program->profile_entry_capacity = 0;
  return program;
}

void ir_program_destroy(IRProgram *program) {
  if (!program) {
    return;
  }

  if (program->functions) {
    for (size_t i = 0; i < program->function_count; i++) {
      ir_function_destroy(program->functions[i]);
    }
    free(program->functions);
  }
  if (program->profile_entries) {
    for (size_t i = 0; i < program->profile_entry_count; i++) {
      free(program->profile_entries[i].name);
      free(program->profile_entries[i].filename);
    }
    free(program->profile_entries);
  }
  free(program);
}

int ir_program_add_function(IRProgram *program, IRFunction *function) {
  if (!program || !function) {
    return 0;
  }

  if (program->function_count >= program->function_capacity) {
    size_t new_capacity =
        program->function_capacity == 0 ? 16 : program->function_capacity * 2;
    IRFunction **new_functions =
        realloc(program->functions, new_capacity * sizeof(IRFunction *));
    if (!new_functions) {
      return 0;
    }
    program->functions = new_functions;
    program->function_capacity = new_capacity;
  }

  program->functions[program->function_count++] = function;
  return 1;
}

static const char *ir_opcode_name(IROpcode op) {
  switch (op) {
  case IR_OP_NOP:
    return "nop";
  case IR_OP_LABEL:
    return "label";
  case IR_OP_JUMP:
    return "jump";
  case IR_OP_BRANCH_ZERO:
    return "branch_zero";
  case IR_OP_BRANCH_EQ:
    return "branch_eq";
  case IR_OP_DECLARE_LOCAL:
    return "local";
  case IR_OP_ASSIGN:
    return "assign";
  case IR_OP_ADDRESS_OF:
    return "addr_of";
  case IR_OP_LOAD:
    return "load";
  case IR_OP_STORE:
    return "store";
  case IR_OP_BINARY:
    return "binary";
  case IR_OP_ROTATE_ADD:
    return "rotate_add";
  case IR_OP_UNARY:
    return "unary";
  case IR_OP_CALL:
    return "call";
  case IR_OP_CALL_INDIRECT:
    return "call_indirect";
  case IR_OP_NEW:
    return "new";
  case IR_OP_RETURN:
    return "return";
  case IR_OP_INLINE_ASM:
    return "inline_asm";
  case IR_OP_CAST:
    return "cast";
  case IR_OP_COUNT_WORD_STARTS: return "count_word_starts";
  case IR_OP_MEMCPY_INLINE: return "memcpy_inline";
  case IR_OP_SIMD_SUM_I32: return "simd_sum_i32";
  case IR_OP_SIMD_MATMUL_N32: return "simd_matmul_n32";
  case IR_OP_SIMD_INSERTION_SORT_I32: return "simd_insertion_sort_i32";
  case IR_OP_SIMD_DOT_I32: return "simd_dot_i32";
  case IR_OP_SIMD_SCALE_I32: return "simd_scale_i32";
  case IR_OP_SIMD_CLAMP_I32: return "simd_clamp_i32";
  case IR_OP_SIMD_REVERSE_COPY_I32: return "simd_reverse_copy_i32";
  case IR_OP_LOWER_BOUND_I32: return "lower_bound_i32";
  case IR_OP_PREFIX_SUM_I32: return "prefix_sum_i32";
  case IR_OP_SIMD_MINMAX_I32: return "simd_minmax_i32";
  default:
    return "unknown";
  }
}

static void ir_format_operand(const IROperand *operand, char *buffer,
                              size_t buffer_size) {
  if (!buffer || buffer_size == 0) {
    return;
  }

  if (!operand) {
    snprintf(buffer, buffer_size, "_");
    return;
  }

  switch (operand->kind) {
  case IR_OPERAND_NONE:
    snprintf(buffer, buffer_size, "_");
    break;
  case IR_OPERAND_TEMP:
    snprintf(buffer, buffer_size, "%%%s", operand->name ? operand->name : "?");
    break;
  case IR_OPERAND_SYMBOL:
    snprintf(buffer, buffer_size, "@%s", operand->name ? operand->name : "?");
    break;
  case IR_OPERAND_INT:
    snprintf(buffer, buffer_size, "%lld", operand->int_value);
    break;
  case IR_OPERAND_FLOAT:
    snprintf(buffer, buffer_size, "%f", operand->float_value);
    break;
  case IR_OPERAND_STRING:
    snprintf(buffer, buffer_size, "\"%s\"", operand->name ? operand->name : "");
    break;
  case IR_OPERAND_LABEL:
    snprintf(buffer, buffer_size, "%s", operand->name ? operand->name : "?");
    break;
  default:
    snprintf(buffer, buffer_size, "_");
    break;
  }
}

static int ir_format_instruction_line(const IRInstruction *instruction,
                                      char *buffer, size_t buffer_size) {
  char dest[IR_OPERAND_FMT_BUFSIZE];
  char lhs[IR_OPERAND_FMT_BUFSIZE];
  char rhs[IR_OPERAND_FMT_BUFSIZE];
  int written = 0;

  if (!instruction || !buffer || buffer_size == 0) {
    return 0;
  }

  ir_format_operand(&instruction->dest, dest, sizeof(dest));
  ir_format_operand(&instruction->lhs, lhs, sizeof(lhs));
  ir_format_operand(&instruction->rhs, rhs, sizeof(rhs));

  switch (instruction->op) {
  case IR_OP_LABEL:
    written = snprintf(buffer, buffer_size, "%s %s", ir_opcode_name(instruction->op),
                       instruction->text ? instruction->text : "<label>");
    break;
  case IR_OP_JUMP:
    written = snprintf(buffer, buffer_size, "%s %s", ir_opcode_name(instruction->op),
                       instruction->text ? instruction->text : "<target>");
    break;
  case IR_OP_BRANCH_ZERO:
    written = snprintf(buffer, buffer_size, "%s %s -> %s",
                       ir_opcode_name(instruction->op), lhs,
                       instruction->text ? instruction->text : "<target>");
    break;
  case IR_OP_BRANCH_EQ:
    written = snprintf(buffer, buffer_size, "%s %s, %s -> %s",
                       ir_opcode_name(instruction->op), lhs, rhs,
                       instruction->text ? instruction->text : "<target>");
    break;
  case IR_OP_DECLARE_LOCAL:
    written = snprintf(buffer, buffer_size, "%s %s : %s",
                       ir_opcode_name(instruction->op), dest,
                       instruction->text ? instruction->text : "<unknown>");
    break;
  case IR_OP_ASSIGN:
    written = snprintf(buffer, buffer_size, "%s <- %s", dest, lhs);
    break;
  case IR_OP_ADDRESS_OF:
    written = snprintf(buffer, buffer_size, "%s <- &%s", dest, lhs);
    break;
  case IR_OP_LOAD:
    written = snprintf(buffer, buffer_size, "%s <- *%s [%s]", dest, lhs, rhs);
    break;
  case IR_OP_STORE:
    written = snprintf(buffer, buffer_size, "*%s <- %s [%s]", dest, lhs, rhs);
    break;
  case IR_OP_BINARY:
    written = snprintf(buffer, buffer_size, "%s = %s %s%s %s", dest, lhs,
                       instruction->text ? instruction->text : "?",
                       instruction->is_float ? " (float)" : "", rhs);
    break;
  case IR_OP_ROTATE_ADD:
    written = snprintf(buffer, buffer_size, "%s = rotate_add(%s, %s)", dest, lhs,
                       rhs);
    break;
  case IR_OP_UNARY:
    written = snprintf(buffer, buffer_size, "%s = %s%s%s", dest,
                       instruction->text ? instruction->text : "?", lhs,
                       instruction->is_float ? " (float)" : "");
    break;
  case IR_OP_CALL:
  case IR_OP_CALL_INDIRECT: {
    size_t offset = 0;
    offset += (size_t)snprintf(buffer + offset, buffer_size - offset, "%s = %s(",
                               dest, instruction->text ? instruction->text
                                                       : "<callee>");
    for (size_t arg_i = 0; arg_i < instruction->argument_count; arg_i++) {
      char arg_buffer[128];
      ir_format_operand(&instruction->arguments[arg_i], arg_buffer,
                        sizeof(arg_buffer));
      offset +=
          (size_t)snprintf(buffer + offset, buffer_size - offset, "%s%s",
                           arg_i == 0 ? "" : ", ", arg_buffer);
      if (offset >= buffer_size) {
        break;
      }
    }
    written = (int)snprintf(buffer + offset, buffer_size - offset, ")");
    if (written >= 0) {
      written += (int)offset;
    }
    break;
  }
  case IR_OP_NEW:
    written = snprintf(buffer, buffer_size, "%s = %s [%s]", dest,
                       instruction->text ? instruction->text : "<type>", rhs);
    break;
  case IR_OP_RETURN:
    written = snprintf(buffer, buffer_size, "return %s", lhs);
    break;
  case IR_OP_INLINE_ASM:
    written = snprintf(buffer, buffer_size, "inline_asm \"%s\"",
                       instruction->text ? instruction->text : "");
    break;
  case IR_OP_CAST:
    written = snprintf(buffer, buffer_size, "%s = (%s)%s%s", dest,
                       instruction->text ? instruction->text : "<type>", lhs,
                       instruction->is_float ? " (float)" : "");
    break;
  case IR_OP_COUNT_WORD_STARTS:
    written = snprintf(buffer, buffer_size, "%s = count_word_starts(buf=%s, len=%s)",
                       dest, lhs, rhs);
    break;
  case IR_OP_MEMCPY_INLINE:
    written = snprintf(buffer, buffer_size, "%s = memcpy_inline %s, %s", dest,
                       lhs, rhs);
    break;
  case IR_OP_SIMD_SUM_I32:
    written = snprintf(buffer, buffer_size, "%s += simd_sum_i32(base=%s, len=%s)",
                       dest, lhs, rhs);
    break;
  case IR_OP_SIMD_MATMUL_N32:
    written = snprintf(buffer, buffer_size, "%s = matmul_n32(c=%s, a=%s, b=%s)",
                       dest, dest, lhs, rhs);
    break;
  case IR_OP_SIMD_INSERTION_SORT_I32:
    written = snprintf(buffer, buffer_size, "simd_insertion_sort_i32(base=%s, len=%s)",
                       dest, rhs);
    break;
  case IR_OP_SIMD_DOT_I32: {
    char len[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      len, sizeof(len));
    written = snprintf(buffer, buffer_size, "%s = dot_i32(a=%s, b=%s, len=%s)",
                       dest, lhs, rhs, len);
    break;
  }
  case IR_OP_SIMD_SCALE_I32: {
    char len[128], mul[128], add[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      len, sizeof(len));
    ir_format_operand(instruction->argument_count > 1 ? &instruction->arguments[1]
                                                      : NULL,
                      mul, sizeof(mul));
    ir_format_operand(instruction->argument_count > 2 ? &instruction->arguments[2]
                                                      : NULL,
                      add, sizeof(add));
    written = snprintf(buffer, buffer_size,
                       "%s = scale_i32(src=%s, dst=%s, len=%s, mul=%s, add=%s)",
                       dest, lhs, rhs, len, mul, add);
    break;
  }
  case IR_OP_SIMD_CLAMP_I32: {
    char len[128], lo[128], hi[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      len, sizeof(len));
    ir_format_operand(instruction->argument_count > 1 ? &instruction->arguments[1]
                                                      : NULL,
                      lo, sizeof(lo));
    ir_format_operand(instruction->argument_count > 2 ? &instruction->arguments[2]
                                                      : NULL,
                      hi, sizeof(hi));
    written = snprintf(buffer, buffer_size,
                       "%s = clamp_i32(src=%s, dst=%s, len=%s, lo=%s, hi=%s)",
                       dest, lhs, rhs, len, lo, hi);
    break;
  }
  case IR_OP_SIMD_REVERSE_COPY_I32: {
    char len[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      len, sizeof(len));
    written = snprintf(buffer, buffer_size,
                       "%s = reverse_copy_i32(src=%s, dst=%s, len=%s)", dest,
                       lhs, rhs, len);
    break;
  }
  case IR_OP_LOWER_BOUND_I32: {
    char key[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      key, sizeof(key));
    written = snprintf(buffer, buffer_size,
                       "%s = lower_bound_i32(arr=%s, n=%s, key=%s)", dest, lhs,
                       rhs, key);
    break;
  }
  case IR_OP_PREFIX_SUM_I32: {
    char len[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      len, sizeof(len));
    written = snprintf(buffer, buffer_size,
                       "%s = prefix_sum_i32(src=%s, dst=%s, len=%s)", dest,
                       lhs, rhs, len);
    break;
  }
  case IR_OP_SIMD_MINMAX_I32: {
    char maxv[128];
    ir_format_operand(instruction->argument_count > 0 ? &instruction->arguments[0]
                                                      : NULL,
                      maxv, sizeof(maxv));
    written = snprintf(buffer, buffer_size,
                       "%s = minmax_i32(arr=%s, n=%s, max=%s)", dest, lhs, rhs,
                       maxv);
    break;
  }
  case IR_OP_NOP:
  default:
    written = snprintf(buffer, buffer_size, "%s", ir_opcode_name(instruction->op));
    break;
  }

  return written > 0 && (size_t)written < buffer_size;
}

int ir_instruction_dump(const IRInstruction *instruction,
                        char *buffer, size_t capacity) {
  if (!instruction || !buffer || capacity == 0) {
    return 0;
  }
  return ir_format_instruction_line(instruction, buffer, capacity);
}

int ir_program_dump(IRProgram *program, FILE *output) {
  if (!program || !output) {
    return 0;
  }

  for (size_t i = 0; i < program->function_count; i++) {
    IRFunction *function = program->functions[i];
    if (!function) {
      continue;
    }

    fprintf(output, "function %s {\n",
            function->name ? function->name : "<anonymous>");

    for (size_t j = 0; j < function->instruction_count; j++) {
      IRInstruction *instruction = &function->instructions[j];
      char buffer[1024];
      ir_format_instruction_line(instruction, buffer, sizeof(buffer));
      fprintf(output, "  %4zu: %s\n", j, buffer);
    }

    fprintf(output, "}\n\n");
  }

  return 1;
}
