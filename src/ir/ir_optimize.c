#include "ir_optimize.h"
#include <string.h>

static int ir_optimize_instruction(IRInstruction *instruction) {
  if (!instruction)
    return 1;

  if (instruction->op == IR_OP_BINARY && !instruction->is_float) {
    if (instruction->lhs.kind == IR_OPERAND_INT &&
        instruction->rhs.kind == IR_OPERAND_INT) {
      if (!instruction->text)
        return 1;
      long long l = instruction->lhs.int_value;
      long long r = instruction->rhs.int_value;
      long long result = 0;
      int folded = 0;

      if (strcmp(instruction->text, "+") == 0) {
        result = l + r;
        folded = 1;
      } else if (strcmp(instruction->text, "-") == 0) {
        result = l - r;
        folded = 1;
      } else if (strcmp(instruction->text, "*") == 0) {
        result = l * r;
        folded = 1;
      } else if (strcmp(instruction->text, "/") == 0 && r != 0) {
        result = l / r;
        folded = 1;
      } else if (strcmp(instruction->text, "%") == 0 && r != 0) {
        result = l % r;
        folded = 1;
      } else if (strcmp(instruction->text, "==") == 0) {
        result = l == r;
        folded = 1;
      } else if (strcmp(instruction->text, "!=") == 0) {
        result = l != r;
        folded = 1;
      } else if (strcmp(instruction->text, "<") == 0) {
        result = l < r;
        folded = 1;
      } else if (strcmp(instruction->text, "<=") == 0) {
        result = l <= r;
        folded = 1;
      } else if (strcmp(instruction->text, ">") == 0) {
        result = l > r;
        folded = 1;
      } else if (strcmp(instruction->text, ">=") == 0) {
        result = l >= r;
        folded = 1;
      } else if (strcmp(instruction->text, "&&") == 0) {
        result = l && r;
        folded = 1;
      } else if (strcmp(instruction->text, "||") == 0) {
        result = l || r;
        folded = 1;
      } else if (strcmp(instruction->text, "&") == 0) {
        result = l & r;
        folded = 1;
      } else if (strcmp(instruction->text, "|") == 0) {
        result = l | r;
        folded = 1;
      } else if (strcmp(instruction->text, "^") == 0) {
        result = l ^ r;
        folded = 1;
      } else if (strcmp(instruction->text, "<<") == 0) {
        result = l << r;
        folded = 1;
      } else if (strcmp(instruction->text, ">>") == 0) {
        result = l >> r;
        folded = 1;
      }

      if (folded) {
        instruction->op = IR_OP_ASSIGN;
        instruction->lhs = ir_operand_int(result);
        ir_operand_destroy(&instruction->rhs);
        instruction->rhs = ir_operand_none();
      }
    }
  }

  return 1;
}

static int ir_fold_constants(IRFunction *function) {
  for (size_t i = 0; i < function->instruction_count; i++) {
    if (!ir_optimize_instruction(&function->instructions[i])) {
      return 0;
    }
  }
  return 1;
}

int ir_optimize_program(IRProgram *program) {
  if (!program)
    return 0;
  for (size_t i = 0; i < program->function_count; i++) {
    if (!ir_fold_constants(program->functions[i])) {
      return 0;
    }
  }
  return 1;
}
