#include "ir.h"
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static char *ir_strdup(const char *text) {
  if (!text) {
    return NULL;
  }
  size_t length = strlen(text) + 1;
  char *copy = malloc(length);
  if (!copy) {
    return NULL;
  }
  memcpy(copy, text, length);
  return copy;
}

IROperand ir_operand_none(void) {
  IROperand operand = {0};
  operand.kind = IR_OPERAND_NONE;
  return operand;
}

IROperand ir_operand_temp(const char *name) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_TEMP;
  operand.name = ir_strdup(name);
  return operand;
}

IROperand ir_operand_symbol(const char *name) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_SYMBOL;
  operand.name = ir_strdup(name);
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
  return operand;
}

IROperand ir_operand_string(const char *value) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_STRING;
  operand.name = ir_strdup(value);
  return operand;
}

IROperand ir_operand_label(const char *name) {
  IROperand operand = ir_operand_none();
  operand.kind = IR_OPERAND_LABEL;
  operand.name = ir_strdup(name);
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
  case IR_OPERAND_TEMP:
    return ir_operand_temp(operand->name);
  case IR_OPERAND_SYMBOL:
    return ir_operand_symbol(operand->name);
  case IR_OPERAND_INT:
    return ir_operand_int(operand->int_value);
  case IR_OPERAND_FLOAT:
    return ir_operand_float(operand->float_value);
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

  function->name = ir_strdup(name ? name : "<anonymous>");
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

    name_copies[i] = ir_strdup(parameter_names[i]);
    if (!name_copies[i]) {
      goto fail;
    }

    if (parameter_types && parameter_types[i]) {
      type_copies[i] = ir_strdup(parameter_types[i]);
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
  slot->text = ir_strdup(instruction->text);
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

IRProgram *ir_program_create(void) {
  IRProgram *program = malloc(sizeof(IRProgram));
  if (!program) {
    return NULL;
  }

  program->functions = NULL;
  program->function_count = 0;
  program->function_capacity = 0;
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
  case IR_OP_UNARY:
    return "unary";
  case IR_OP_CALL:
    return "call";
  case IR_OP_NEW:
    return "new";
  case IR_OP_RETURN:
    return "return";
  case IR_OP_INLINE_ASM:
    return "inline_asm";
  case IR_OP_CAST:
    return "cast";
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
      char dest[128];
      char lhs[128];
      char rhs[128];
      ir_format_operand(&instruction->dest, dest, sizeof(dest));
      ir_format_operand(&instruction->lhs, lhs, sizeof(lhs));
      ir_format_operand(&instruction->rhs, rhs, sizeof(rhs));

      fprintf(output, "  %4zu: ", j);
      switch (instruction->op) {
      case IR_OP_LABEL:
        fprintf(output, "%s %s\n", ir_opcode_name(instruction->op),
                instruction->text ? instruction->text : "<label>");
        break;
      case IR_OP_JUMP:
        fprintf(output, "%s %s\n", ir_opcode_name(instruction->op),
                instruction->text ? instruction->text : "<target>");
        break;
      case IR_OP_BRANCH_ZERO:
        fprintf(output, "%s %s -> %s\n", ir_opcode_name(instruction->op), lhs,
                instruction->text ? instruction->text : "<target>");
        break;
      case IR_OP_BRANCH_EQ:
        fprintf(output, "%s %s, %s -> %s\n", ir_opcode_name(instruction->op),
                lhs, rhs, instruction->text ? instruction->text : "<target>");
        break;
      case IR_OP_DECLARE_LOCAL:
        fprintf(output, "%s %s : %s\n", ir_opcode_name(instruction->op), dest,
                instruction->text ? instruction->text : "<unknown>");
        break;
      case IR_OP_ASSIGN:
        fprintf(output, "%s %s <- %s\n", ir_opcode_name(instruction->op), dest,
                lhs);
        break;
      case IR_OP_ADDRESS_OF:
        fprintf(output, "%s %s <- &%s\n", ir_opcode_name(instruction->op), dest,
                lhs);
        break;
      case IR_OP_LOAD:
        fprintf(output, "%s %s <- *%s [%s]\n", ir_opcode_name(instruction->op),
                dest, lhs, rhs);
        break;
      case IR_OP_STORE:
        fprintf(output, "%s *%s <- %s [%s]\n", ir_opcode_name(instruction->op),
                dest, lhs, rhs);
        break;
      case IR_OP_BINARY:
        fprintf(output, "%s %s = %s %s%s %s\n", ir_opcode_name(instruction->op),
                dest, lhs, instruction->text ? instruction->text : "?",
                instruction->is_float ? " (float)" : "", rhs);
        break;
      case IR_OP_UNARY:
        fprintf(output, "%s %s = %s%s%s\n", ir_opcode_name(instruction->op),
                dest, instruction->text ? instruction->text : "?", lhs,
                instruction->is_float ? " (float)" : "");
        break;
      case IR_OP_CALL:
        fprintf(output, "%s %s = %s(", ir_opcode_name(instruction->op), dest,
                instruction->text ? instruction->text : "<callee>");
        for (size_t arg_i = 0; arg_i < instruction->argument_count; arg_i++) {
          char arg_buffer[128];
          ir_format_operand(&instruction->arguments[arg_i], arg_buffer,
                            sizeof(arg_buffer));
          fprintf(output, "%s%s", arg_i == 0 ? "" : ", ", arg_buffer);
        }
        fprintf(output, ")\n");
        break;
      case IR_OP_NEW:
        fprintf(output, "%s %s = %s [%s]\n", ir_opcode_name(instruction->op),
                dest, instruction->text ? instruction->text : "<type>", rhs);
        break;
      case IR_OP_RETURN:
        fprintf(output, "%s %s\n", ir_opcode_name(instruction->op), lhs);
        break;
      case IR_OP_INLINE_ASM:
        fprintf(output, "%s \"%s\"\n", ir_opcode_name(instruction->op),
                instruction->text ? instruction->text : "");
        break;
      case IR_OP_CAST:
        fprintf(output, "%s %s = (%s)%s%s\n", ir_opcode_name(instruction->op),
                dest, instruction->text ? instruction->text : "<type>", lhs,
                instruction->is_float ? " (float)" : "");
        break;
      case IR_OP_NOP:
      default:
        fprintf(output, "%s\n", ir_opcode_name(instruction->op));
        break;
      }
    }

    fprintf(output, "}\n\n");
  }

  return 1;
}
