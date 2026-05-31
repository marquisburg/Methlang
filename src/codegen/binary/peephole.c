#include "codegen/binary/internal.h"

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

int code_generator_binary_is_compare_operator(const char *op) {
  return op &&
         (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 ||
          strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
          strcmp(op, ">") == 0 || strcmp(op, ">=") == 0);
}

int code_generator_binary_compare_false_jcc(const char *op,
                                                   unsigned char *opcode_out) {
  if (!op || !opcode_out) {
    return 0;
  }

  if (strcmp(op, "==") == 0) {
    *opcode_out = 0x85; /* jne */
    return 1;
  }
  if (strcmp(op, "!=") == 0) {
    *opcode_out = 0x84; /* je */
    return 1;
  }
  if (strcmp(op, "<") == 0) {
    *opcode_out = 0x8D; /* jge */
    return 1;
  }
  if (strcmp(op, "<=") == 0) {
    *opcode_out = 0x8F; /* jg */
    return 1;
  }
  if (strcmp(op, ">") == 0) {
    *opcode_out = 0x8E; /* jle */
    return 1;
  }
  if (strcmp(op, ">=") == 0) {
    *opcode_out = 0x8C; /* jl */
    return 1;
  }

  return 0;
}

int code_generator_binary_compare_true_cmov(const char *op,
                                                   unsigned char *opcode_out) {
  if (!op || !opcode_out) {
    return 0;
  }

  if (strcmp(op, "==") == 0) {
    *opcode_out = 0x44; /* cmove */
    return 1;
  }
  if (strcmp(op, "!=") == 0) {
    *opcode_out = 0x45; /* cmovne */
    return 1;
  }
  if (strcmp(op, "<") == 0) {
    *opcode_out = 0x4C; /* cmovl */
    return 1;
  }
  if (strcmp(op, "<=") == 0) {
    *opcode_out = 0x4E; /* cmovle */
    return 1;
  }
  if (strcmp(op, ">") == 0) {
    *opcode_out = 0x4F; /* cmovg */
    return 1;
  }
  if (strcmp(op, ">=") == 0) {
    *opcode_out = 0x4D; /* cmovge */
    return 1;
  }

  return 0;
}

int code_generator_binary_operand_uses_temp(const IROperand *operand,
                                                   const char *name) {
  return operand && operand->kind == IR_OPERAND_TEMP && operand->name && name &&
         strcmp(operand->name, name) == 0;
}

int code_generator_binary_instruction_temp_use_count(
    const IRInstruction *instruction, const char *name) {
  int count = 0;

  if (!instruction || !name) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&instruction->lhs, name)) {
    count++;
  }
  if (code_generator_binary_operand_uses_temp(&instruction->rhs, name)) {
    count++;
  }
  if (instruction->op == IR_OP_STORE &&
      code_generator_binary_operand_uses_temp(&instruction->dest, name)) {
    count++;
  }
  for (size_t i = 0; i < instruction->argument_count; i++) {
    if (code_generator_binary_operand_uses_temp(&instruction->arguments[i],
                                                name)) {
      count++;
    }
  }

  return count;
}

int code_generator_binary_function_temp_use_count(
    const IRFunction *function, const char *name) {
  int count = 0;

  if (!function || !name) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    count += code_generator_binary_instruction_temp_use_count(
        &function->instructions[i], name);
  }

  return count;
}

int code_generator_binary_label_reference_count(
    const IRFunction *function, const char *label) {
  int count = 0;
  if (!function || !label) {
    return 0;
  }

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    if (!instruction || !instruction->text) {
      continue;
    }
    if ((instruction->op == IR_OP_JUMP ||
         instruction->op == IR_OP_BRANCH_ZERO ||
         instruction->op == IR_OP_BRANCH_EQ) &&
        strcmp(instruction->text, label) == 0) {
      count++;
    }
  }

  return count;
}

const IRInstruction *code_generator_binary_find_temp_producer_before(
    const IRFunction *function, size_t before, const char *name) {
  if (!function || !name) {
    return NULL;
  }

  if (before > function->instruction_count) {
    before = function->instruction_count;
  }
  while (before > 0) {
    const IRInstruction *instruction = &function->instructions[--before];
    if (instruction->dest.kind == IR_OPERAND_TEMP && instruction->dest.name &&
        strcmp(instruction->dest.name, name) == 0) {
      return instruction;
    }
  }

  return NULL;
}

int code_generator_binary_shift_scale(const IRInstruction *instruction,
                                             int *scale_out) {
  if (!instruction || instruction->op != IR_OP_BINARY || instruction->is_float ||
      !instruction->text || strcmp(instruction->text, "<<") != 0 ||
      instruction->rhs.kind != IR_OPERAND_INT ||
      instruction->rhs.int_value < 0 || instruction->rhs.int_value > 3) {
    return 0;
  }

  if (scale_out) {
    *scale_out = 1 << (int)instruction->rhs.int_value;
  }
  return 1;
}

int code_generator_binary_emit_compare_flags(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *compare) {
  BinaryGpRegister lhs_register = BINARY_GP_RAX;
  BinaryGpRegister rhs_register = BINARY_GP_R10;
  int lhs_has_register = 0;
  int rhs_has_register = 0;

  if (!generator || !context || !compare ||
      !code_generator_binary_is_compare_operator(compare->text)) {
    return 0;
  }

  lhs_has_register =
      compare->lhs.kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, compare->lhs.name, &lhs_register);
  rhs_has_register =
      compare->rhs.kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, compare->rhs.name, &rhs_register);

  if (compare->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          compare->rhs.int_value)) {
    if (!lhs_has_register &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &compare->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    return binary_emit_cmp_reg_imm32(
        &context->code, lhs_has_register ? lhs_register : BINARY_GP_RAX,
        (uint32_t)(int32_t)compare->rhs.int_value);
  }

  if ((strcmp(compare->text, "==") == 0 ||
       strcmp(compare->text, "!=") == 0) &&
      compare->lhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(compare->lhs.int_value)) {
    if (!rhs_has_register &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &compare->rhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    return binary_emit_cmp_reg_imm32(
        &context->code, rhs_has_register ? rhs_register : BINARY_GP_RAX,
        (uint32_t)(int32_t)compare->lhs.int_value);
  }

  if ((!rhs_has_register &&
       !code_generator_binary_emit_operand_load(generator, context,
                                                &compare->rhs,
                                                BINARY_GP_R10)) ||
      (!lhs_has_register &&
       !code_generator_binary_emit_operand_load(generator, context,
                                                &compare->lhs,
                                                BINARY_GP_RAX))) {
    return 0;
  }

  return code_generator_binary_emit_reg_reg_compare(
      &context->code, lhs_has_register ? lhs_register : BINARY_GP_RAX,
      rhs_has_register ? rhs_register : BINARY_GP_R10,
      code_generator_binary_instruction_compare_width(generator, context,
                                                    compare));
}

int code_generator_binary_emit_compare_false_branch(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *compare, const char *target_label) {
  unsigned char branch_opcode = 0;
  size_t displacement_offset = 0;
  BinaryGpRegister lhs_register = BINARY_GP_RAX;
  BinaryGpRegister rhs_register = BINARY_GP_R10;
  int lhs_has_register = 0;
  int rhs_has_register = 0;

  if (!generator || !context || !compare || !target_label ||
      target_label[0] == '\0' ||
      !code_generator_binary_compare_false_jcc(compare->text,
                                               &branch_opcode)) {
    return 0;
  }

  lhs_has_register =
      compare->lhs.kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, compare->lhs.name, &lhs_register);
  rhs_has_register =
      compare->rhs.kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, compare->rhs.name, &rhs_register);

  if (compare->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          compare->rhs.int_value)) {
    if ((!lhs_has_register &&
         !code_generator_binary_emit_operand_load(generator, context,
                                                  &compare->lhs,
                                                  BINARY_GP_RAX)) ||
        !binary_emit_cmp_reg_imm32(
            &context->code, lhs_has_register ? lhs_register : BINARY_GP_RAX,
            (uint32_t)(int32_t)compare->rhs.int_value)) {
      return 0;
    }
  } else if ((strcmp(compare->text, "==") == 0 ||
              strcmp(compare->text, "!=") == 0) &&
             compare->lhs.kind == IR_OPERAND_INT &&
             code_generator_binary_immediate_fits_signed_32(
                 compare->lhs.int_value)) {
    if ((!rhs_has_register &&
         !code_generator_binary_emit_operand_load(generator, context,
                                                  &compare->rhs,
                                                  BINARY_GP_RAX)) ||
        !binary_emit_cmp_reg_imm32(
            &context->code, rhs_has_register ? rhs_register : BINARY_GP_RAX,
            (uint32_t)(int32_t)compare->lhs.int_value)) {
      return 0;
    }
  } else {
    if (!rhs_has_register &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &compare->rhs,
                                                 BINARY_GP_R10)) {
      return 0;
    }
    if (!lhs_has_register &&
        !code_generator_binary_emit_operand_load(generator, context,
                                                 &compare->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    if (!code_generator_binary_emit_reg_reg_compare(
            &context->code, lhs_has_register ? lhs_register : BINARY_GP_RAX,
            rhs_has_register ? rhs_register : BINARY_GP_R10,
            code_generator_binary_instruction_compare_width(generator, context,
                                                            compare))) {
      return 0;
    }
  }

  if (!binary_emit_jcc_placeholder(&context->code, branch_opcode,
                                   &displacement_offset) ||
      !binary_label_fixup_table_add(&context->label_fixups, target_label,
                                    displacement_offset)) {
    code_generator_set_error(generator,
                             "Out of memory while emitting compare branch");
    return 0;
  }

  return 1;
}

int code_generator_binary_emit_integer_binary_to_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  const char *op = NULL;

  if (!generator || !context || !instruction || instruction->is_float ||
      !instruction->text) {
    return 0;
  }

  op = instruction->text;
  if ((strcmp(op, "/") == 0 || strcmp(op, "%") == 0) &&
      instruction->rhs.kind == IR_OPERAND_INT) {
    unsigned int shift = 0;
    unsigned long long mask = 0;
    if (code_generator_binary_extract_positive_power_of_two(
            instruction->rhs.int_value, &shift, &mask)) {
      if (!code_generator_binary_emit_operand_load(generator, context,
                                                   &instruction->lhs,
                                                   BINARY_GP_RAX)) {
        return 0;
      }

      if (strcmp(op, "/") == 0) {
        if (shift != 0 &&
            (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                      BINARY_GP_RAX) ||
             !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RCX,
                                         63) ||
             !code_generator_binary_emit_and_mask(context, BINARY_GP_RCX,
                                                  mask) ||
             !binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                      BINARY_GP_RCX) ||
             !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RAX,
                                         (unsigned char)shift))) {
          code_generator_set_error(
              generator,
              "Out of memory while emitting integer expression chain in "
              "function '%s'",
              context->function_name);
          return 0;
        }
      } else if (shift == 0) {
        if (!binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0)) {
          code_generator_set_error(
              generator,
              "Out of memory while emitting integer expression chain in "
              "function '%s'",
              context->function_name);
          return 0;
        }
      } else if (!binary_emit_mov_reg_reg(&context->code, BINARY_GP_R11,
                                          BINARY_GP_RAX) ||
                 !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                          BINARY_GP_RAX) ||
                 !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RCX,
                                             63) ||
                 !code_generator_binary_emit_and_mask(context, BINARY_GP_RCX,
                                                      mask) ||
                 !binary_emit_alu_reg_reg(&context->code, 0x01,
                                          BINARY_GP_RAX, BINARY_GP_RCX) ||
                 !binary_emit_shift_reg_imm8(&context->code, 7, BINARY_GP_RAX,
                                             (unsigned char)shift) ||
                 !binary_emit_shift_reg_imm8(&context->code, 4, BINARY_GP_RAX,
                                             (unsigned char)shift) ||
                 !binary_emit_alu_reg_reg(&context->code, 0x29,
                                          BINARY_GP_R11, BINARY_GP_RAX) ||
                 !binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                          BINARY_GP_R11)) {
        code_generator_set_error(
            generator,
            "Out of memory while emitting integer expression chain in function "
            "'%s'",
            context->function_name);
        return 0;
      }
      return 1;
    }
  }

  if (instruction->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          instruction->rhs.int_value) &&
      (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
       strcmp(op, "*") == 0 || strcmp(op, "&") == 0 ||
       strcmp(op, "|") == 0 || strcmp(op, "^") == 0 ||
       ((strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) &&
         instruction->rhs.int_value >= 0 && instruction->rhs.int_value < 64))) {
    long long immediate = instruction->rhs.int_value;
    if (immediate == 0 &&
        (strcmp(op, "*") == 0 || strcmp(op, "&") == 0)) {
      return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0);
    }
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->lhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }

    if (strcmp(op, "+") == 0) {
      return binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "-") == 0) {
      return binary_emit_sub_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "*") == 0) {
      if (immediate == 1) {
        return 1;
      }
      if (immediate == -1) {
        return binary_emit_neg_reg(&context->code, BINARY_GP_RAX);
      }
      if (immediate == 2 || immediate == 4 || immediate == 8) {
        unsigned char shift = immediate == 2 ? 1 : (immediate == 4 ? 2 : 3);
        return binary_emit_shift_reg_imm8(&context->code, 4, BINARY_GP_RAX,
                                          shift);
      }
      if (immediate == -2 || immediate == -4 || immediate == -8) {
        unsigned char shift = immediate == -2 ? 1 : (immediate == -4 ? 2 : 3);
        return binary_emit_shift_reg_imm8(&context->code, 4, BINARY_GP_RAX,
                                          shift) &&
               binary_emit_neg_reg(&context->code, BINARY_GP_RAX);
      }
      if (immediate == 3 || immediate == 5 || immediate == 9) {
        int scale = immediate == 3 ? 2 : (immediate == 5 ? 4 : 8);
        return binary_emit_lea_reg_base_index_scale_disp(
            &context->code, BINARY_GP_RAX, BINARY_GP_RAX, BINARY_GP_RAX,
            scale, 0);
      }
      return binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "&") == 0) {
      return binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "|") == 0) {
      return binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "^") == 0) {
      return binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    return binary_emit_shift_reg_imm8(
        &context->code, strcmp(op, "<<") == 0 ? 4 : 7, BINARY_GP_RAX,
        (unsigned char)immediate);
  }

  if (instruction->lhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          instruction->lhs.int_value) &&
      (strcmp(op, "+") == 0 || strcmp(op, "*") == 0 ||
       strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
       strcmp(op, "^") == 0)) {
    long long immediate = instruction->lhs.int_value;
    if (immediate == 0 &&
        (strcmp(op, "*") == 0 || strcmp(op, "&") == 0)) {
      return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0);
    }
    if (!code_generator_binary_emit_operand_load(generator, context,
                                                 &instruction->rhs,
                                                 BINARY_GP_RAX)) {
      return 0;
    }
    if (strcmp(op, "+") == 0) {
      return binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "*") == 0) {
      if (immediate == 1) {
        return 1;
      }
      if (immediate == -1) {
        return binary_emit_neg_reg(&context->code, BINARY_GP_RAX);
      }
      if (immediate == 2 || immediate == 4 || immediate == 8) {
        unsigned char shift = immediate == 2 ? 1 : (immediate == 4 ? 2 : 3);
        return binary_emit_shift_reg_imm8(&context->code, 4, BINARY_GP_RAX,
                                          shift);
      }
      if (immediate == -2 || immediate == -4 || immediate == -8) {
        unsigned char shift = immediate == -2 ? 1 : (immediate == -4 ? 2 : 3);
        return binary_emit_shift_reg_imm8(&context->code, 4, BINARY_GP_RAX,
                                          shift) &&
               binary_emit_neg_reg(&context->code, BINARY_GP_RAX);
      }
      if (immediate == 3 || immediate == 5 || immediate == 9) {
        int scale = immediate == 3 ? 2 : (immediate == 5 ? 4 : 8);
        return binary_emit_lea_reg_base_index_scale_disp(
            &context->code, BINARY_GP_RAX, BINARY_GP_RAX, BINARY_GP_RAX,
            scale, 0);
      }
      return binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "&") == 0) {
      return binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "|") == 0) {
      return binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate);
    }
    return binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                     (uint32_t)(int32_t)immediate);
  }

  if (!code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->rhs,
                                               BINARY_GP_R10) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &instruction->lhs,
                                               BINARY_GP_RAX)) {
    return 0;
  }

  if (strcmp(op, "+") == 0) {
    return binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RAX, BINARY_GP_R10) ||
           binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "-") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x29, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "*") == 0) {
    return binary_emit_imul_reg_reg(&context->code, BINARY_GP_RAX,
                                    BINARY_GP_R10);
  }
  if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
    if (!binary_emit_cqo(&context->code) ||
        !binary_emit_idiv_reg(&context->code, BINARY_GP_R10)) {
      return 0;
    }
    if (strcmp(op, "%") == 0) {
      return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                     BINARY_GP_RDX);
    }
    return 1;
  }
  if (strcmp(op, "&") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x21, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "|") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x09, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "^") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x31, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
    return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                   BINARY_GP_R10) &&
           binary_emit_shift_reg_cl(&context->code,
                                    strcmp(op, "<<") == 0 ? 4 : 7,
                                    BINARY_GP_RAX);
  }

  return 0;
}

int code_generator_binary_emit_compare_false_branch_from_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *compare, const IROperand *rhs,
    const char *target_label) {
  unsigned char branch_opcode = 0;
  size_t displacement_offset = 0;

  if (!generator || !context || !compare || !compare->text || !rhs ||
      !target_label ||
      !code_generator_binary_compare_false_jcc(compare->text, &branch_opcode)) {
    return 0;
  }

  if (rhs->kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(rhs->int_value)) {
    if (!binary_emit_cmp_reg_imm32(&context->code, BINARY_GP_RAX,
                                   (uint32_t)(int32_t)rhs->int_value)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_operand_load(generator, context, rhs,
                                                      BINARY_GP_R10) ||
             !code_generator_binary_emit_reg_reg_compare(
                 &context->code, BINARY_GP_RAX, BINARY_GP_R10,
                 code_generator_binary_instruction_compare_width(generator,
                                                                 context,
                                                                 compare))) {
    return 0;
  }

  if (!binary_emit_jcc_placeholder(&context->code, branch_opcode,
                                   &displacement_offset) ||
      !binary_label_fixup_table_add(&context->label_fixups, target_label,
                                    displacement_offset)) {
    code_generator_set_error(
        generator,
        "Out of memory while emitting chained compare branch in function '%s'",
        context->function_name);
    return 0;
  }

  return 1;
}

int code_generator_binary_try_emit_compare_branch_zero(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *compare = NULL;
  const IRInstruction *branch = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  compare = &function->instructions[instruction_index];
  branch = &function->instructions[instruction_index + 1];
  if (!compare || compare->op != IR_OP_BINARY || compare->is_float ||
      !code_generator_binary_is_compare_operator(compare->text) ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      branch->op != IR_OP_BRANCH_ZERO ||
      !code_generator_binary_operand_uses_temp(&branch->lhs,
                                               compare->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    compare->dest.name) != 1) {
    return 0;
  }

  if (!code_generator_binary_emit_compare_false_branch(generator, context,
                                                       compare,
                                                       branch->text)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

static int code_generator_binary_try_emit_and_mask_compare_false_branch(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *producer, const IRInstruction *compare,
    const IROperand *other_operand, const char *target_label) {
  const IROperand *value_operand = NULL;
  long long mask_value = 0;
  BinaryGpRegister source_register = BINARY_GP_RAX;
  int has_source_register = 0;
  unsigned char branch_opcode = 0;
  size_t displacement_offset = 0;

  if (!generator || !context || !producer || !compare || !other_operand ||
      !target_label || strcmp(producer->text, "&") != 0 ||
      (strcmp(compare->text, "==") != 0 && strcmp(compare->text, "!=") != 0) ||
      other_operand->kind != IR_OPERAND_INT || other_operand->int_value != 0) {
    return 0;
  }

  if (producer->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(
          producer->rhs.int_value)) {
    value_operand = &producer->lhs;
    mask_value = producer->rhs.int_value;
  } else if (producer->lhs.kind == IR_OPERAND_INT &&
             code_generator_binary_immediate_fits_signed_32(
                 producer->lhs.int_value)) {
    value_operand = &producer->rhs;
    mask_value = producer->lhs.int_value;
  } else {
    return 0;
  }

  if (value_operand->kind == IR_OPERAND_SYMBOL &&
      code_generator_binary_symbol_assigned_register(
          generator, context, value_operand->name, &source_register)) {
    has_source_register = 1;
  } else if (!code_generator_binary_emit_operand_load(
                 generator, context, value_operand, BINARY_GP_RAX)) {
    return 0;
  }

  if (!binary_emit_test_reg_imm32(
          &context->code, has_source_register ? source_register : BINARY_GP_RAX,
          (uint32_t)(int32_t)mask_value) ||
      !code_generator_binary_compare_false_jcc(compare->text, &branch_opcode) ||
      !binary_emit_jcc_placeholder(&context->code, branch_opcode,
                                   &displacement_offset) ||
      !binary_label_fixup_table_add(&context->label_fixups, target_label,
                                    displacement_offset)) {
    return 0;
  }

  return 1;
}

int code_generator_binary_chain_producer_supported(const char *op) {
  return op &&
         (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
          strcmp(op, "*") == 0 || strcmp(op, "/") == 0 ||
          strcmp(op, "%") == 0 || strcmp(op, "&") == 0 ||
          strcmp(op, "|") == 0 || strcmp(op, "^") == 0 ||
          strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0);
}

int code_generator_binary_try_emit_binary_compare_branch_chain(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *producer = NULL;
  const IRInstruction *compare = NULL;
  const IRInstruction *branch = NULL;
  const IROperand *other_operand = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 2 >= function->instruction_count) {
    return 0;
  }

  producer = &function->instructions[instruction_index];
  compare = &function->instructions[instruction_index + 1];
  branch = &function->instructions[instruction_index + 2];

  if (!producer || producer->op != IR_OP_BINARY || producer->is_float ||
      !code_generator_binary_chain_producer_supported(producer->text) ||
      producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
      !compare || compare->op != IR_OP_BINARY || compare->is_float ||
      !code_generator_binary_is_compare_operator(compare->text) ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      !branch || branch->op != IR_OP_BRANCH_ZERO ||
      !code_generator_binary_operand_uses_temp(&branch->lhs,
                                               compare->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    producer->dest.name) != 1 ||
      code_generator_binary_function_temp_use_count(function,
                                                    compare->dest.name) != 1) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&compare->lhs,
                                              producer->dest.name)) {
    other_operand = &compare->rhs;
  } else if ((strcmp(compare->text, "==") == 0 ||
              strcmp(compare->text, "!=") == 0) &&
             code_generator_binary_operand_uses_temp(&compare->rhs,
                                                     producer->dest.name)) {
    other_operand = &compare->lhs;
  } else {
    return 0;
  }

  /* Relational compares against an immediate belong to compare_branch_zero,
   * not producer+compare fusion. Fusing them here can miscompile when a nearby
   * db_page_size() temp is still live (e.g. cache_load's `if (n <= 0)` after
   * db_pread). */
  if (compare->lhs.kind == IR_OPERAND_SYMBOL &&
      other_operand->kind == IR_OPERAND_INT &&
      (strcmp(compare->text, "<=") == 0 || strcmp(compare->text, ">=") == 0 ||
       strcmp(compare->text, "<") == 0 || strcmp(compare->text, ">") == 0)) {
    return 0;
  }

  if (code_generator_binary_try_emit_and_mask_compare_false_branch(
          generator, context, producer, compare, other_operand,
          branch->text)) {
    *consumed_out = 3;
    return 1;
  }

  if (!code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                        producer)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Failed to emit chained integer expression in function '%s'",
          context->function_name);
    }
    return 0;
  }

  if (!code_generator_binary_emit_compare_false_branch_from_rax(
          generator, context, compare, other_operand, branch->text)) {
    return 0;
  }

  *consumed_out = 3;
  return 1;
}

int code_generator_binary_try_emit_compare_assign_diamond(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *compare = NULL;
  const IRInstruction *branch = NULL;
  const IRInstruction *assign = NULL;
  const IRInstruction *jump = NULL;
  const IRInstruction *false_label = NULL;
  const IRInstruction *end_label = NULL;
  const IRInstruction *value_assign = NULL;
  unsigned char cmov_opcode = 0;
  IRInstruction compare_for_emit = {0};

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 5 >= function->instruction_count) {
    return 0;
  }

  compare = &function->instructions[instruction_index];
  branch = &function->instructions[instruction_index + 1];
  assign = &function->instructions[instruction_index + 2];
  jump = &function->instructions[instruction_index + 3];
  false_label = &function->instructions[instruction_index + 4];
  end_label = &function->instructions[instruction_index + 5];

  if (!compare || compare->op != IR_OP_BINARY || compare->is_float ||
      !code_generator_binary_compare_true_cmov(compare->text, &cmov_opcode) ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      !branch || branch->op != IR_OP_BRANCH_ZERO || !branch->text ||
      !code_generator_binary_operand_uses_temp(&branch->lhs,
                                               compare->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    compare->dest.name) != 1 ||
      !assign || assign->op != IR_OP_ASSIGN ||
      assign->dest.kind == IR_OPERAND_NONE ||
      assign->dest.kind == IR_OPERAND_INT ||
      !jump || jump->op != IR_OP_JUMP || !jump->text ||
      !false_label || false_label->op != IR_OP_LABEL || !false_label->text ||
      !end_label || end_label->op != IR_OP_LABEL || !end_label->text ||
      strcmp(branch->text, false_label->text) != 0 ||
      strcmp(jump->text, end_label->text) != 0 ||
      code_generator_binary_label_reference_count(function,
                                                  false_label->text) != 1 ||
      code_generator_binary_label_reference_count(function, end_label->text) !=
          1) {
    return 0;
  }

  compare_for_emit = *compare;
  if (compare->lhs.kind == IR_OPERAND_SYMBOL && compare->lhs.name &&
      assign->lhs.kind == IR_OPERAND_TEMP && assign->lhs.name) {
    for (size_t ai = instruction_index; ai > 0 && ai > instruction_index - 8;
         ai--) {
      const IRInstruction *prev = &function->instructions[ai - 1];
      if (prev->op == IR_OP_ASSIGN &&
          prev->dest.kind == IR_OPERAND_SYMBOL && prev->dest.name &&
          compare->lhs.name &&
          strcmp(prev->dest.name, compare->lhs.name) == 0 &&
          prev->lhs.kind == IR_OPERAND_TEMP && prev->lhs.name &&
          assign->lhs.name &&
          strcmp(prev->lhs.name, assign->lhs.name) == 0) {
        value_assign = prev;
        break;
      }
    }
  }
  if (value_assign && compare->rhs.kind == IR_OPERAND_SYMBOL) {
    compare_for_emit.lhs = assign->lhs;
  }

  if (!code_generator_binary_emit_compare_flags(generator, context,
                                                &compare_for_emit) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &assign->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &assign->lhs,
                                               BINARY_GP_R10) ||
      !binary_emit_cmovcc_reg_reg(&context->code, cmov_opcode, BINARY_GP_RAX,
                                  BINARY_GP_R10) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &assign->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 6;
  return 1;
}

static int code_generator_binary_next_non_nop_index(const IRFunction *function,
                                                    size_t start,
                                                    size_t *out_index) {
  if (!function || !out_index) {
    return 0;
  }
  for (size_t i = start; i < function->instruction_count; i++) {
    if (function->instructions[i].op != IR_OP_NOP) {
      *out_index = i;
      return 1;
    }
  }
  return 0;
}

static int code_generator_binary_compare_false_cmov(const char *op,
                                                    unsigned char *opcode_out) {
  if (!op || !opcode_out) {
    return 0;
  }
  if (strcmp(op, "==") == 0) {
    *opcode_out = 0x45; /* cmovne */
    return 1;
  }
  if (strcmp(op, "!=") == 0) {
    *opcode_out = 0x44; /* cmove */
    return 1;
  }
  if (strcmp(op, "<") == 0) {
    *opcode_out = 0x4D; /* cmovge */
    return 1;
  }
  if (strcmp(op, "<=") == 0) {
    *opcode_out = 0x4F; /* cmovg */
    return 1;
  }
  if (strcmp(op, ">") == 0) {
    *opcode_out = 0x4E; /* cmovle */
    return 1;
  }
  if (strcmp(op, ">=") == 0) {
    *opcode_out = 0x4C; /* cmovl */
    return 1;
  }
  return 0;
}

static int code_generator_binary_emit_value_to_r10(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *instruction) {
  if (!generator || !context || !instruction) {
    return 0;
  }
  if (instruction->op == IR_OP_ASSIGN) {
    return code_generator_binary_emit_operand_load(generator, context,
                                                   &instruction->lhs,
                                                   BINARY_GP_R10);
  }
  if (instruction->op == IR_OP_BINARY && !instruction->is_float &&
      code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                       instruction) &&
      binary_emit_mov_reg_reg(&context->code, BINARY_GP_R10, BINARY_GP_RAX)) {
    return 1;
  }
  return 0;
}

int code_generator_binary_try_emit_compare_update_pair_diamond(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  size_t i_compare = 0;
  size_t i_branch = 0;
  size_t i_true_assign = 0;
  size_t i_true_jump = 0;
  size_t i_false_label = 0;
  size_t i_false_assign = 0;
  size_t i_false_jump = 0;
  unsigned char true_cmov = 0;
  unsigned char false_cmov = 0;
  const IRInstruction *compare = NULL;
  const IRInstruction *branch = NULL;
  const IRInstruction *true_assign = NULL;
  const IRInstruction *true_jump = NULL;
  const IRInstruction *false_label = NULL;
  const IRInstruction *false_assign = NULL;
  const IRInstruction *false_jump = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out) {
    return 0;
  }
  if (!code_generator_binary_next_non_nop_index(function, instruction_index,
                                                &i_compare) ||
      i_compare != instruction_index ||
      !code_generator_binary_next_non_nop_index(function, i_compare + 1,
                                                &i_branch) ||
      !code_generator_binary_next_non_nop_index(function, i_branch + 1,
                                                &i_true_assign) ||
      !code_generator_binary_next_non_nop_index(function, i_true_assign + 1,
                                                &i_true_jump) ||
      !code_generator_binary_next_non_nop_index(function, i_true_jump + 1,
                                                &i_false_label) ||
      !code_generator_binary_next_non_nop_index(function, i_false_label + 1,
                                                &i_false_assign) ||
      !code_generator_binary_next_non_nop_index(function, i_false_assign + 1,
                                                &i_false_jump)) {
    return 0;
  }

  compare = &function->instructions[i_compare];
  branch = &function->instructions[i_branch];
  true_assign = &function->instructions[i_true_assign];
  true_jump = &function->instructions[i_true_jump];
  false_label = &function->instructions[i_false_label];
  false_assign = &function->instructions[i_false_assign];
  false_jump = &function->instructions[i_false_jump];

  if (!compare || compare->op != IR_OP_BINARY || compare->is_float ||
      !compare->text ||
      !code_generator_binary_compare_true_cmov(compare->text, &true_cmov) ||
      !code_generator_binary_compare_false_cmov(compare->text, &false_cmov) ||
      compare->dest.kind != IR_OPERAND_TEMP || !compare->dest.name ||
      !branch || branch->op != IR_OP_BRANCH_ZERO || !branch->text ||
      !code_generator_binary_operand_uses_temp(&branch->lhs,
                                               compare->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    compare->dest.name) != 1 ||
      !true_assign ||
      !(true_assign->op == IR_OP_ASSIGN ||
        (true_assign->op == IR_OP_BINARY && !true_assign->is_float)) ||
      true_assign->dest.kind != IR_OPERAND_SYMBOL || !true_assign->dest.name ||
      !true_jump || true_jump->op != IR_OP_JUMP || !true_jump->text ||
      !false_label || false_label->op != IR_OP_LABEL || !false_label->text ||
      strcmp(branch->text, false_label->text) != 0 ||
      !false_assign || false_assign->op != IR_OP_ASSIGN ||
      false_assign->dest.kind != IR_OPERAND_SYMBOL || !false_assign->dest.name ||
      !false_jump || false_jump->op != IR_OP_JUMP || !false_jump->text ||
      strcmp(true_jump->text, false_jump->text) != 0 ||
      code_generator_binary_label_reference_count(function, false_label->text) !=
          1 ||
      strcmp(true_assign->dest.name, false_assign->dest.name) == 0) {
    return 0;
  }

  if (!code_generator_binary_emit_compare_flags(generator, context, compare) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &true_assign->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_value_to_r10(generator, context,
                                               true_assign) ||
      !binary_emit_cmovcc_reg_reg(&context->code, true_cmov, BINARY_GP_RAX,
                                  BINARY_GP_R10) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &true_assign->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  if (!code_generator_binary_emit_compare_flags(generator, context, compare) ||
      !code_generator_binary_emit_operand_load(generator, context,
                                               &false_assign->dest,
                                               BINARY_GP_RAX) ||
      !code_generator_binary_emit_value_to_r10(generator, context,
                                               false_assign) ||
      !binary_emit_cmovcc_reg_reg(&context->code, false_cmov, BINARY_GP_RAX,
                                  BINARY_GP_R10) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &false_assign->dest,
                                                    BINARY_GP_RAX) ||
      !code_generator_binary_emit_instruction(generator, context, false_jump)) {
    return 0;
  }

  *consumed_out = i_false_jump - instruction_index + 1;
  return 1;
}

int code_generator_binary_emit_address_add_to_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRInstruction *address) {
  if (!generator || !context || !address || address->op != IR_OP_BINARY ||
      address->is_float || !address->text ||
      strcmp(address->text, "+") != 0) {
    return 0;
  }

  if (address->rhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(address->rhs.int_value)) {
    return code_generator_binary_emit_operand_load(generator, context,
                                                   &address->lhs,
                                                   BINARY_GP_RAX) &&
           binary_emit_add_reg_imm32(
               &context->code, BINARY_GP_RAX,
               (uint32_t)(int32_t)address->rhs.int_value);
  }
  if (address->lhs.kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(address->lhs.int_value)) {
    return code_generator_binary_emit_operand_load(generator, context,
                                                   &address->rhs,
                                                   BINARY_GP_RAX) &&
           binary_emit_add_reg_imm32(
               &context->code, BINARY_GP_RAX,
               (uint32_t)(int32_t)address->lhs.int_value);
  }

  return code_generator_binary_emit_operand_load(generator, context,
                                                 &address->rhs,
                                                 BINARY_GP_R10) &&
         code_generator_binary_emit_operand_load(generator, context,
                                                 &address->lhs,
                                                 BINARY_GP_RAX) &&
         (binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                  BINARY_GP_RAX, BINARY_GP_R10) ||
          binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                  BINARY_GP_R10));
}



int code_generator_binary_try_emit_address_add_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *load = NULL;
  const IROperand *base = NULL;
  const IROperand *index = NULL;
  int scale = 0;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  address = &function->instructions[instruction_index];
  load = &function->instructions[instruction_index + 1];
  if (!address || address->op != IR_OP_BINARY || address->is_float ||
      !address->text || strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name ||
      !load || load->op != IR_OP_LOAD ||
      !code_generator_binary_operand_uses_temp(&load->lhs,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &load->rhs);
  if (size <= 0) {
    return 0;
  }

  if (code_generator_binary_try_match_scaled_temp_address(
          function, instruction_index, address, &base, &index, &scale)) {
    if (!code_generator_binary_emit_scaled_address_to_rax(
            generator, context, base, index, scale)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_address_add_to_rax(generator, context,
                                                            address)) {
    return 0;
  }

  if (!code_generator_binary_emit_load_from_address(generator, context,
                                                    BINARY_GP_RAX, size,
                                                    BINARY_GP_RAX)) {
    return 0;
  }
  if (size == 4 && !load->is_float &&
      code_generator_binary_load_needs_sign_extend(generator, context,
                                                   &load->dest, size) &&
      !binary_emit_movsxd_rax_eax(&context->code)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Out of memory while emitting fused address load in function '%s'",
          context->function_name);
    }
    return 0;
  }
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &load->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

int code_generator_binary_try_emit_address_add_store(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *store = NULL;
  const IROperand *base = NULL;
  const IROperand *index = NULL;
  int scale = 0;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  address = &function->instructions[instruction_index];
  store = &function->instructions[instruction_index + 1];
  if (!address || address->op != IR_OP_BINARY || address->is_float ||
      !address->text || strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name ||
      !store || store->op != IR_OP_STORE ||
      !code_generator_binary_operand_uses_temp(&store->dest,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &store->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_operand_load(generator, context, &store->lhs,
                                               BINARY_GP_STORE_VALUE)) {
    return 0;
  }

  if (store->is_float && store->float_bits) {
    int value_bits = code_generator_binary_operand_float_bits(
        generator, context, &store->lhs);
    if (value_bits &&
        !code_generator_binary_emit_float_reg_convert(
            context, BINARY_GP_STORE_VALUE, value_bits, store->float_bits)) {
      code_generator_set_error(generator,
                               "Out of memory while converting float store "
                               "precision in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  if (code_generator_binary_try_match_scaled_temp_address(
          function, instruction_index, address, &base, &index, &scale)) {
    if (!code_generator_binary_emit_scaled_address_to_rax(
            generator, context, base, index, scale)) {
      return 0;
    }
  } else if (!code_generator_binary_emit_address_add_to_rax(generator, context,
                                                            address)) {
    return 0;
  }

  if (!code_generator_binary_emit_store_to_address(generator, context,
                                                  BINARY_GP_RAX, size,
                                                  BINARY_GP_STORE_VALUE)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

int code_generator_binary_try_match_scaled_address(
    const IRFunction *function, size_t instruction_index,
    const IRInstruction **address_out, const IROperand **base_out,
    const IROperand **index_out, int *scale_out) {
  const IRInstruction *shift = NULL;
  const IRInstruction *address = NULL;
  int scale = 0;

  if (!function || !address_out || !base_out || !index_out || !scale_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  shift = &function->instructions[instruction_index];
  address = &function->instructions[instruction_index + 1];
  if (!shift ||
      shift->dest.kind != IR_OPERAND_TEMP || !shift->dest.name ||
      shift->lhs.kind == IR_OPERAND_INT ||
      !code_generator_binary_shift_scale(shift, &scale) ||
      code_generator_binary_function_temp_use_count(function,
                                                    shift->dest.name) != 1 ||
      !address || address->op != IR_OP_BINARY || address->is_float ||
      !address->text || strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&address->lhs,
                                              shift->dest.name)) {
    if (address->rhs.kind == IR_OPERAND_INT) {
      return 0;
    }
    *base_out = &address->rhs;
  } else if (code_generator_binary_operand_uses_temp(&address->rhs,
                                                     shift->dest.name)) {
    if (address->lhs.kind == IR_OPERAND_INT) {
      return 0;
    }
    *base_out = &address->lhs;
  } else {
    return 0;
  }

  *address_out = address;
  *index_out = &shift->lhs;
  *scale_out = scale;
  return 1;
}

int code_generator_binary_try_match_scaled_temp_address(
    const IRFunction *function, size_t instruction_index,
    const IRInstruction *address, const IROperand **base_out,
    const IROperand **index_out, int *scale_out) {
  const IROperand *scaled_operand = NULL;
  const IROperand *base_operand = NULL;
  const IRInstruction *shift = NULL;
  int scale = 0;

  if (!function || !address || !base_out || !index_out || !scale_out ||
      address->op != IR_OP_BINARY || address->is_float || !address->text ||
      strcmp(address->text, "+") != 0 ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name) {
    return 0;
  }

  /* Identify which operand is the scaled index: it must be a temp whose
   * producer is a scaling shift (`idx << k`). When both operands are temps
   * (e.g. `addr_of_base + (i << 2)`), we cannot assume lhs is the index -- the
   * base may equally be a temp -- so probe each candidate and accept the one
   * that actually resolves to a shift. Picking the wrong operand here silently
   * dropped the scale to 1 and re-read an unmaterialized slot, collapsing every
   * a[i] access onto a[0]. */
  const IROperand *candidates[2] = {NULL, NULL};
  const IROperand *others[2] = {NULL, NULL};
  int candidate_count = 0;
  if (address->lhs.kind == IR_OPERAND_TEMP && address->lhs.name &&
      address->rhs.kind != IR_OPERAND_INT) {
    candidates[candidate_count] = &address->lhs;
    others[candidate_count] = &address->rhs;
    candidate_count++;
  }
  if (address->rhs.kind == IR_OPERAND_TEMP && address->rhs.name &&
      address->lhs.kind != IR_OPERAND_INT) {
    candidates[candidate_count] = &address->rhs;
    others[candidate_count] = &address->lhs;
    candidate_count++;
  }
  if (candidate_count == 0) {
    return 0;
  }

  for (int c = 0; c < candidate_count; c++) {
    const IRInstruction *producer =
        code_generator_binary_find_temp_producer_before(
            function, instruction_index, candidates[c]->name);
    if (producer && producer->lhs.kind != IR_OPERAND_INT &&
        code_generator_binary_shift_scale(producer, &scale)) {
      scaled_operand = candidates[c];
      base_operand = others[c];
      shift = producer;
      break;
    }
  }
  if (!shift) {
    return 0;
  }

  /* This fold reaches back past the shift and re-derives the address as
   * `base + (shift->lhs)*scale`, loading shift->lhs from its home. That is
   * only valid if shift->lhs is reliably materialized there. A symbol
   * (param/local) always owns a stack slot. A temp, however, may have been
   * folded into the shift by the binary-expression chain peephole (e.g.
   * `t0 = n - 1; t1 = t0 << 2`), in which case t0 was never spilled and its
   * slot holds stale data — re-reading it yields the wrong index (observed:
   * prefix_sum's `dst[n-1]` loaded dst[0]). Only fold pre-shift when the
   * index is a symbol; otherwise fall back to the plain address-add path,
   * which consumes the already-computed scaled temp directly. */
  if (shift->lhs.kind != IR_OPERAND_SYMBOL) {
    return 0;
  }

  *base_out = base_operand;
  *index_out = &shift->lhs;
  *scale_out = scale;
  return 1;
}

int code_generator_binary_address_consumed_by_adjacent_memory(
    const IRFunction *function, size_t address_index) {
  const IRInstruction *address = NULL;
  const IRInstruction *consumer = NULL;

  if (!function || address_index + 1 >= function->instruction_count) {
    return 0;
  }

  address = &function->instructions[address_index];
  consumer = &function->instructions[address_index + 1];
  if (!address || address->op != IR_OP_BINARY ||
      address->dest.kind != IR_OPERAND_TEMP || !address->dest.name ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  if (consumer->op == IR_OP_LOAD) {
    return code_generator_binary_operand_uses_temp(&consumer->lhs,
                                                   address->dest.name);
  }
  if (consumer->op == IR_OP_STORE) {
    return code_generator_binary_operand_uses_temp(&consumer->dest,
                                                   address->dest.name);
  }

  return 0;
}

int code_generator_binary_shift_only_feeds_scaled_addresses(
    const IRFunction *function, size_t shift_index) {
  const IRInstruction *shift = NULL;
  const char *name = NULL;
  int total_uses = 0;

  if (!function || shift_index >= function->instruction_count) {
    return 0;
  }

  shift = &function->instructions[shift_index];
  /* Only skip the shift if the consuming scaled-address fold can re-derive it.
   * That fold re-reads the shift's source operand from its home slot and so
   * requires a SYMBOL (which always owns a stack slot); a temp source may have
   * been folded away and never materialized. Keeping this precondition in lock-
   * step with try_match_scaled_temp_address prevents skipping a shift the fold
   * will then decline -- which left the index uncomputed and silently scaled
   * the address by 1. */
  if (!shift || shift->dest.kind != IR_OPERAND_TEMP || !shift->dest.name ||
      shift->lhs.kind != IR_OPERAND_SYMBOL ||
      !code_generator_binary_shift_scale(shift, NULL)) {
    return 0;
  }
  name = shift->dest.name;

  for (size_t i = 0; i < function->instruction_count; i++) {
    const IRInstruction *instruction = &function->instructions[i];
    int uses_here = code_generator_binary_instruction_temp_use_count(instruction,
                                                                     name);
    if (uses_here == 0) {
      continue;
    }
    total_uses += uses_here;
    if (!instruction || instruction->op != IR_OP_BINARY ||
        instruction->is_float || !instruction->text ||
        strcmp(instruction->text, "+") != 0 ||
        instruction->dest.kind != IR_OPERAND_TEMP ||
        !instruction->dest.name ||
        !code_generator_binary_address_consumed_by_adjacent_memory(function,
                                                                   i)) {
      return 0;
    }
    if (code_generator_binary_operand_uses_temp(&instruction->lhs, name)) {
      if (instruction->rhs.kind == IR_OPERAND_INT) {
        return 0;
      }
    } else if (code_generator_binary_operand_uses_temp(&instruction->rhs,
                                                       name)) {
      if (instruction->lhs.kind == IR_OPERAND_INT) {
        return 0;
      }
    } else {
      return 0;
    }
  }

  return total_uses > 0;
}

int code_generator_binary_try_skip_scaled_address_shift(
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!function || !consumed_out ||
      !code_generator_binary_shift_only_feeds_scaled_addresses(
          function, instruction_index)) {
    return 0;
  }

  *consumed_out = 1;
  return 1;
}

int code_generator_binary_emit_scaled_address_to_rax(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IROperand *base, const IROperand *index, int scale) {
  if (!generator || !context || !base || !index) {
    return 0;
  }

  return code_generator_binary_emit_operand_load(generator, context, index,
                                                 BINARY_GP_R10) &&
         code_generator_binary_emit_operand_load(generator, context, base,
                                                 BINARY_GP_RAX) &&
         binary_emit_lea_reg_base_index_scale_disp(
             &context->code, BINARY_GP_RAX, BINARY_GP_RAX, BINARY_GP_R10,
             scale, 0);
}

int code_generator_binary_try_emit_scaled_address_load(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *load = NULL;
  const IROperand *base = NULL;
  const IROperand *index = NULL;
  int scale = 0;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 2 >= function->instruction_count ||
      !code_generator_binary_try_match_scaled_address(
          function, instruction_index, &address, &base, &index, &scale)) {
    return 0;
  }

  load = &function->instructions[instruction_index + 2];
  if (!load || load->op != IR_OP_LOAD ||
      !code_generator_binary_operand_uses_temp(&load->lhs,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &load->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_scaled_address_to_rax(
          generator, context, base, index, scale) ||
      !code_generator_binary_emit_load_from_address(generator, context,
                                                    BINARY_GP_RAX, size,
                                                    BINARY_GP_RAX)) {
    return 0;
  }
  if (size == 4 && !load->is_float &&
      code_generator_binary_load_needs_sign_extend(generator, context,
                                                   &load->dest, size) &&
      !binary_emit_movsxd_rax_eax(&context->code)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Out of memory while emitting fused scaled address load in function "
          "'%s'",
          context->function_name);
    }
    return 0;
  }
  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &load->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 3;
  return 1;
}

int code_generator_binary_try_emit_scaled_address_store(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *address = NULL;
  const IRInstruction *store = NULL;
  const IROperand *base = NULL;
  const IROperand *index = NULL;
  int scale = 0;
  int size = 0;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 2 >= function->instruction_count ||
      !code_generator_binary_try_match_scaled_address(
          function, instruction_index, &address, &base, &index, &scale)) {
    return 0;
  }

  store = &function->instructions[instruction_index + 2];
  if (!store || store->op != IR_OP_STORE ||
      !code_generator_binary_operand_uses_temp(&store->dest,
                                               address->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    address->dest.name) != 1) {
    return 0;
  }

  size = code_generator_binary_get_access_size(generator, context, &store->rhs);
  if (size <= 0) {
    return 0;
  }

  if (!code_generator_binary_emit_operand_load(generator, context, &store->lhs,
                                               BINARY_GP_STORE_VALUE)) {
    return 0;
  }

  if (store->is_float && store->float_bits) {
    int value_bits = code_generator_binary_operand_float_bits(
        generator, context, &store->lhs);
    if (value_bits &&
        !code_generator_binary_emit_float_reg_convert(
            context, BINARY_GP_STORE_VALUE, value_bits, store->float_bits)) {
      code_generator_set_error(generator,
                               "Out of memory while converting float store "
                               "precision in function '%s'",
                               context->function_name);
      return 0;
    }
  }

  if (!code_generator_binary_emit_scaled_address_to_rax(
          generator, context, base, index, scale) ||
      !code_generator_binary_emit_store_to_address(generator, context,
                                                   BINARY_GP_RAX, size,
                                                   BINARY_GP_STORE_VALUE)) {
    return 0;
  }

  *consumed_out = 3;
  return 1;
}

int code_generator_binary_try_emit_binary_cast_chain(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *producer = NULL;
  const IRInstruction *cast = NULL;
  Type *target_type = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  producer = &function->instructions[instruction_index];
  cast = &function->instructions[instruction_index + 1];
  if (!producer || producer->op != IR_OP_BINARY || producer->is_float ||
      !code_generator_binary_chain_producer_supported(producer->text) ||
      producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
      !cast || cast->op != IR_OP_CAST || cast->is_float || !cast->text ||
      !code_generator_binary_operand_uses_temp(&cast->lhs,
                                               producer->dest.name) ||
      code_generator_binary_function_temp_use_count(function,
                                                    producer->dest.name) != 1) {
    return 0;
  }

  target_type = generator->type_checker
                    ? type_checker_get_type_by_name(generator->type_checker,
                                                    cast->text)
                    : NULL;
  if (target_type &&
      (code_generator_is_floating_point_type(target_type) ||
       (target_type->kind != TYPE_POINTER &&
        target_type->kind != TYPE_FUNCTION_POINTER &&
        target_type->size != 8))) {
    return 0;
  }

  if (!code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                        producer) ||
      !code_generator_binary_emit_destination_store(generator, context,
                                                    &cast->dest,
                                                    BINARY_GP_RAX)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Failed to emit chained integer cast in function '%s'",
          context->function_name);
    }
    return 0;
  }

  *consumed_out = 2;
  return 1;
}

int code_generator_binary_operator_is_commutative(const char *op) {
  return op &&
         (strcmp(op, "+") == 0 || strcmp(op, "*") == 0 ||
          strcmp(op, "&") == 0 || strcmp(op, "|") == 0 ||
          strcmp(op, "^") == 0);
}

int code_generator_binary_emit_rax_binary_rhs(
    CodeGenerator *generator, BinaryFunctionContext *context, const char *op,
    const IROperand *rhs) {
  if (!generator || !context || !op || !rhs) {
    return 0;
  }

  if (rhs->kind == IR_OPERAND_INT &&
      code_generator_binary_immediate_fits_signed_32(rhs->int_value)) {
    long long immediate = rhs->int_value;
    if (immediate == 0 &&
        (strcmp(op, "*") == 0 || strcmp(op, "&") == 0)) {
      return binary_emit_mov_reg_imm64(&context->code, BINARY_GP_RAX, 0);
    }
    if (strcmp(op, "+") == 0) {
      return binary_emit_add_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "-") == 0) {
      return binary_emit_sub_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "*") == 0) {
      return binary_emit_imul_reg_reg_imm32(&context->code, BINARY_GP_RAX,
                                            BINARY_GP_RAX,
                                            (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "&") == 0) {
      return binary_emit_and_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "|") == 0) {
      return binary_emit_or_reg_imm32(&context->code, BINARY_GP_RAX,
                                      (uint32_t)(int32_t)immediate);
    }
    if (strcmp(op, "^") == 0) {
      return binary_emit_xor_reg_imm32(&context->code, BINARY_GP_RAX,
                                       (uint32_t)(int32_t)immediate);
    }
    if ((strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) &&
        immediate >= 0 && immediate < 64) {
      return binary_emit_shift_reg_imm8(
          &context->code, strcmp(op, "<<") == 0 ? 4 : 7, BINARY_GP_RAX,
          (unsigned char)immediate);
    }
  }

  if (!code_generator_binary_emit_operand_load(generator, context, rhs,
                                               BINARY_GP_R10)) {
    return 0;
  }

  if (strcmp(op, "+") == 0) {
    return binary_emit_lea_reg_reg(&context->code, BINARY_GP_RAX,
                                   BINARY_GP_RAX, BINARY_GP_R10) ||
           binary_emit_alu_reg_reg(&context->code, 0x01, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "-") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x29, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "*") == 0) {
    return binary_emit_imul_reg_reg(&context->code, BINARY_GP_RAX,
                                    BINARY_GP_R10);
  }
  if (strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {
    if (!binary_emit_cqo(&context->code) ||
        !binary_emit_idiv_reg(&context->code, BINARY_GP_R10)) {
      return 0;
    }
    if (strcmp(op, "%") == 0) {
      return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RAX,
                                     BINARY_GP_RDX);
    }
    return 1;
  }
  if (strcmp(op, "&") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x21, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "|") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x09, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "^") == 0) {
    return binary_emit_alu_reg_reg(&context->code, 0x31, BINARY_GP_RAX,
                                   BINARY_GP_R10);
  }
  if (strcmp(op, "<<") == 0 || strcmp(op, ">>") == 0) {
    return binary_emit_mov_reg_reg(&context->code, BINARY_GP_RCX,
                                   BINARY_GP_R10) &&
           binary_emit_shift_reg_cl(&context->code,
                                    strcmp(op, "<<") == 0 ? 4 : 7,
                                    BINARY_GP_RAX);
  }

  return 0;
}

int code_generator_binary_try_emit_binary_expression_chain(
    CodeGenerator *generator, BinaryFunctionContext *context,
    const IRFunction *function, size_t instruction_index,
    size_t *consumed_out) {
  const IRInstruction *producer = NULL;
  const IRInstruction *consumer = NULL;
  const IROperand *rhs = NULL;

  if (consumed_out) {
    *consumed_out = 0;
  }
  if (!generator || !context || !function || !consumed_out ||
      instruction_index + 1 >= function->instruction_count) {
    return 0;
  }

  producer = &function->instructions[instruction_index];
  consumer = &function->instructions[instruction_index + 1];
  if (!producer || producer->op != IR_OP_BINARY || producer->is_float ||
      !code_generator_binary_chain_producer_supported(producer->text) ||
      producer->dest.kind != IR_OPERAND_TEMP || !producer->dest.name ||
      !consumer || consumer->op != IR_OP_BINARY || consumer->is_float ||
      code_generator_binary_is_compare_operator(consumer->text) ||
      !code_generator_binary_chain_producer_supported(consumer->text) ||
      code_generator_binary_function_temp_use_count(function,
                                                    producer->dest.name) != 1) {
    return 0;
  }

  if (code_generator_binary_operand_uses_temp(&consumer->lhs,
                                              producer->dest.name)) {
    rhs = &consumer->rhs;
  } else if (code_generator_binary_operator_is_commutative(consumer->text) &&
             code_generator_binary_operand_uses_temp(&consumer->rhs,
                                                     producer->dest.name)) {
    rhs = &consumer->lhs;
  } else {
    return 0;
  }

  if (!code_generator_binary_emit_integer_binary_to_rax(generator, context,
                                                        producer) ||
      !code_generator_binary_emit_rax_binary_rhs(generator, context,
                                                 consumer->text, rhs)) {
    if (!generator->has_error) {
      code_generator_set_error(
          generator,
          "Failed to emit chained integer expression in function '%s'",
          context->function_name);
    }
    return 0;
  }

  if (!code_generator_binary_emit_destination_store(generator, context,
                                                    &consumer->dest,
                                                    BINARY_GP_RAX)) {
    return 0;
  }

  *consumed_out = 2;
  return 1;
}
