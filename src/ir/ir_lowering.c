#include "ir.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *break_label;
  char *continue_label;
} IRControlFrame;

typedef struct {
  int next_temp_id;
  int next_label_id;
  IRControlFrame *control_stack;
  size_t control_count;
  size_t control_capacity;
  char *error_message;
  TypeChecker *type_checker;
  SymbolTable *symbol_table;
} IRLoweringContext;

static char *ir_strdup_local(const char *text) {
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

static void ir_set_error(IRLoweringContext *context, const char *format, ...) {
  if (!context || context->error_message || !format) {
    return;
  }

  va_list args;
  va_start(args, format);
  va_list copy;
  va_copy(copy, args);
  int needed = vsnprintf(NULL, 0, format, copy);
  va_end(copy);

  if (needed > 0) {
    context->error_message = malloc((size_t)needed + 1);
    if (context->error_message) {
      vsnprintf(context->error_message, (size_t)needed + 1, format, args);
    }
  }
  va_end(args);
}

static char *ir_new_temp_name(IRLoweringContext *context) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "t%d", context->next_temp_id++);
  return ir_strdup_local(buffer);
}

static char *ir_new_label_name(IRLoweringContext *context, const char *prefix) {
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "ir_%s_%d", prefix ? prefix : "label",
           context->next_label_id++);
  return ir_strdup_local(buffer);
}

static int ir_emit(IRLoweringContext *context, IRFunction *function,
                   const IRInstruction *instruction) {
  if (!ir_function_append_instruction(function, instruction)) {
    ir_set_error(context, "Out of memory while appending IR instruction");
    return 0;
  }
  return 1;
}

static int ir_push_control_frame(IRLoweringContext *context,
                                 const char *break_label,
                                 const char *continue_label) {
  if (!context) {
    return 0;
  }

  if (context->control_count >= context->control_capacity) {
    size_t new_capacity =
        context->control_capacity == 0 ? 8 : context->control_capacity * 2;
    IRControlFrame *new_stack =
        realloc(context->control_stack, new_capacity * sizeof(IRControlFrame));
    if (!new_stack) {
      ir_set_error(context,
                   "Out of memory while growing IR control-flow stack");
      return 0;
    }
    context->control_stack = new_stack;
    context->control_capacity = new_capacity;
  }

  IRControlFrame *frame = &context->control_stack[context->control_count++];
  frame->break_label = ir_strdup_local(break_label);
  frame->continue_label = ir_strdup_local(continue_label);
  return 1;
}

static void ir_pop_control_frame(IRLoweringContext *context) {
  if (!context || context->control_count == 0) {
    return;
  }

  IRControlFrame *frame = &context->control_stack[context->control_count - 1];
  free(frame->break_label);
  free(frame->continue_label);
  frame->break_label = NULL;
  frame->continue_label = NULL;
  context->control_count--;
}

static const char *ir_current_break_label(IRLoweringContext *context) {
  if (!context || context->control_count == 0) {
    return NULL;
  }
  return context->control_stack[context->control_count - 1].break_label;
}

static const char *ir_current_continue_label(IRLoweringContext *context) {
  if (!context || context->control_count == 0) {
    return NULL;
  }

  for (size_t i = context->control_count; i > 0; i--) {
    const char *label = context->control_stack[i - 1].continue_label;
    if (label) {
      return label;
    }
  }
  return NULL;
}

static int ir_lower_expression(IRLoweringContext *context, IRFunction *function,
                               ASTNode *expression, IROperand *out_value);
static int ir_lower_lvalue_address(IRLoweringContext *context,
                                   IRFunction *function, ASTNode *expression,
                                   IROperand *out_address, Type **out_type);

static int ir_expression_is_floating(IRLoweringContext *context,
                                     ASTNode *expression) {
  if (!context || !context->type_checker || !expression) {
    return 0;
  }

  Type *type = type_checker_infer_type(context->type_checker, expression);
  if (!type) {
    return 0;
  }

  return type->kind == TYPE_FLOAT32 || type->kind == TYPE_FLOAT64;
}

static int ir_type_storage_size(Type *type) {
  if (!type || type->size == 0) {
    return 8;
  }

  if (type->size == 1 || type->size == 2 || type->size == 4 ||
      type->size == 8) {
    return (int)type->size;
  }

  return 8;
}

static int ir_make_temp_operand(IRLoweringContext *context,
                                IROperand *out_temp) {
  if (!context || !out_temp) {
    return 0;
  }

  char *temp_name = ir_new_temp_name(context);
  if (!temp_name) {
    ir_set_error(context, "Out of memory while allocating IR temp");
    return 0;
  }

  *out_temp = ir_operand_temp(temp_name);
  free(temp_name);
  if (out_temp->kind != IR_OPERAND_TEMP || !out_temp->name) {
    ir_set_error(context, "Failed to create IR temp operand");
    return 0;
  }

  return 1;
}

static int ir_lower_call_expression(IRLoweringContext *context,
                                    IRFunction *function, ASTNode *expression,
                                    IROperand *out_value) {
  CallExpression *call = (CallExpression *)expression->data;
  if (!call || !call->function_name) {
    ir_set_error(context, "Malformed call expression");
    return 0;
  }
  if (call->object) {
    ir_set_error(context, "Method calls should be mangled directly, unresolved "
                          "method object in lower pass");
    return 0;
  }

  IROperand destination = ir_operand_none();
  if (!ir_make_temp_operand(context, &destination)) {
    return 0;
  }

  IROperand *arguments = NULL;
  if (call->argument_count > 0) {
    arguments = calloc(call->argument_count, sizeof(IROperand));
    if (!arguments) {
      ir_operand_destroy(&destination);
      ir_set_error(context, "Out of memory while lowering call arguments");
      return 0;
    }
  }

  for (size_t i = 0; i < call->argument_count; i++) {
    if (!ir_lower_expression(context, function, call->arguments[i],
                             &arguments[i])) {
      for (size_t j = 0; j < i; j++) {
        ir_operand_destroy(&arguments[j]);
      }
      free(arguments);
      ir_operand_destroy(&destination);
      return 0;
    }
  }

  IRInstruction instruction = {0};
  instruction.op = IR_OP_CALL;
  instruction.location = expression->location;
  instruction.dest = destination;
  instruction.text = call->function_name;
  instruction.arguments = arguments;
  instruction.argument_count = call->argument_count;

  if (!ir_emit(context, function, &instruction)) {
    for (size_t i = 0; i < call->argument_count; i++) {
      ir_operand_destroy(&arguments[i]);
    }
    free(arguments);
    ir_operand_destroy(&destination);
    return 0;
  }

  for (size_t i = 0; i < call->argument_count; i++) {
    ir_operand_destroy(&arguments[i]);
  }
  free(arguments);

  *out_value = destination;
  return 1;
}

static Type *ir_infer_expression_type(IRLoweringContext *context,
                                      ASTNode *expression) {
  if (!context || !context->type_checker || !expression) {
    return NULL;
  }
  return type_checker_infer_type(context->type_checker, expression);
}

static int ir_emit_address_of_symbol(IRLoweringContext *context,
                                     IRFunction *function, const char *name,
                                     SourceLocation location,
                                     IROperand *out_address) {
  if (!context || !function || !name || !out_address) {
    return 0;
  }

  IROperand destination = ir_operand_none();
  if (!ir_make_temp_operand(context, &destination)) {
    return 0;
  }

  IROperand symbol = ir_operand_symbol(name);
  if (symbol.kind != IR_OPERAND_SYMBOL || !symbol.name) {
    ir_operand_destroy(&destination);
    ir_set_error(context, "Out of memory while lowering symbol address");
    return 0;
  }

  IRInstruction instruction = {0};
  instruction.op = IR_OP_ADDRESS_OF;
  instruction.location = location;
  instruction.dest = destination;
  instruction.lhs = symbol;
  if (!ir_emit(context, function, &instruction)) {
    ir_operand_destroy(&destination);
    ir_operand_destroy(&symbol);
    return 0;
  }

  ir_operand_destroy(&symbol);
  *out_address = destination;
  return 1;
}

static int ir_lower_lvalue_address(IRLoweringContext *context,
                                   IRFunction *function, ASTNode *expression,
                                   IROperand *out_address, Type **out_type) {
  if (!context || !function || !expression || !out_address) {
    return 0;
  }

  *out_address = ir_operand_none();
  if (out_type) {
    *out_type = NULL;
  }

  switch (expression->type) {
  case AST_IDENTIFIER: {
    Identifier *identifier = (Identifier *)expression->data;
    if (!identifier || !identifier->name) {
      ir_set_error(context, "Malformed identifier lvalue");
      return 0;
    }

    if (out_type) {
      Symbol *symbol =
          context->symbol_table
              ? symbol_table_lookup(context->symbol_table, identifier->name)
              : NULL;
      if (symbol && (symbol->kind == SYMBOL_VARIABLE ||
                     symbol->kind == SYMBOL_PARAMETER)) {
        *out_type = symbol->type;
      } else {
        *out_type = ir_infer_expression_type(context, expression);
      }
    }
    return ir_emit_address_of_symbol(context, function, identifier->name,
                                     expression->location, out_address);
  }

  case AST_MEMBER_ACCESS: {
    MemberAccess *member = (MemberAccess *)expression->data;
    if (!member || !member->object || !member->member) {
      ir_set_error(context, "Malformed member access lvalue");
      return 0;
    }

    IROperand object_address = ir_operand_none();
    Type *object_type = ir_infer_expression_type(context, member->object);

    if (object_type && object_type->kind == TYPE_POINTER) {
      if (!ir_lower_expression(context, function, member->object,
                               &object_address)) {
        return 0;
      }
      object_type = object_type->base_type;
    } else {
      if (!ir_lower_lvalue_address(context, function, member->object,
                                   &object_address, &object_type)) {
        return 0;
      }
    }
    if (!object_type || (object_type->kind != TYPE_STRUCT &&
                         object_type->kind != TYPE_STRING)) {
      ir_operand_destroy(&object_address);
      ir_set_error(context,
                   "Member access requires struct or string lvalue object");
      return 0;
    }

    Type *field_type = type_get_field_type(object_type, member->member);
    size_t field_offset = type_get_field_offset(object_type, member->member);
    if (!field_type || field_offset == (size_t)-1) {
      ir_operand_destroy(&object_address);
      ir_set_error(context, "Unknown struct field '%s'", member->member);
      return 0;
    }

    if (out_type) {
      *out_type = field_type;
    }

    IROperand field_address = ir_operand_none();
    if (!ir_make_temp_operand(context, &field_address)) {
      ir_operand_destroy(&object_address);
      return 0;
    }

    IRInstruction add = {0};
    add.op = IR_OP_BINARY;
    add.location = expression->location;
    add.dest = field_address;
    add.lhs = object_address;
    add.rhs = ir_operand_int((long long)field_offset);
    add.text = "+";
    if (!ir_emit(context, function, &add)) {
      ir_operand_destroy(&field_address);
      ir_operand_destroy(&object_address);
      return 0;
    }

    ir_operand_destroy(&object_address);
    *out_address = field_address;
    return 1;
  }

  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *index_expression =
        (ArrayIndexExpression *)expression->data;
    if (!index_expression || !index_expression->array ||
        !index_expression->index) {
      ir_set_error(context, "Malformed index lvalue");
      return 0;
    }

    Type *array_type =
        ir_infer_expression_type(context, index_expression->array);
    if (!array_type ||
        (array_type->kind != TYPE_ARRAY && array_type->kind != TYPE_POINTER) ||
        !array_type->base_type) {
      ir_set_error(context, "Index lvalue requires array or pointer type");
      return 0;
    }

    if (out_type) {
      *out_type = array_type->base_type;
    }

    IROperand base = ir_operand_none();
    IROperand index = ir_operand_none();
    if (!ir_lower_expression(context, function, index_expression->array,
                             &base) ||
        !ir_lower_expression(context, function, index_expression->index,
                             &index)) {
      ir_operand_destroy(&base);
      ir_operand_destroy(&index);
      return 0;
    }

    IROperand scaled = ir_operand_none();
    if (!ir_make_temp_operand(context, &scaled)) {
      ir_operand_destroy(&base);
      ir_operand_destroy(&index);
      return 0;
    }

    int element_size = ir_type_storage_size(array_type->base_type);
    IRInstruction multiply = {0};
    multiply.op = IR_OP_BINARY;
    multiply.location = expression->location;
    multiply.dest = scaled;
    multiply.lhs = index;
    multiply.rhs = ir_operand_int(element_size);
    multiply.text = "*";
    if (!ir_emit(context, function, &multiply)) {
      ir_operand_destroy(&scaled);
      ir_operand_destroy(&base);
      ir_operand_destroy(&index);
      return 0;
    }

    IROperand address = ir_operand_none();
    if (!ir_make_temp_operand(context, &address)) {
      ir_operand_destroy(&scaled);
      ir_operand_destroy(&base);
      ir_operand_destroy(&index);
      return 0;
    }

    IRInstruction add = {0};
    add.op = IR_OP_BINARY;
    add.location = expression->location;
    add.dest = address;
    add.lhs = base;
    add.rhs = scaled;
    add.text = "+";
    if (!ir_emit(context, function, &add)) {
      ir_operand_destroy(&address);
      ir_operand_destroy(&scaled);
      ir_operand_destroy(&base);
      ir_operand_destroy(&index);
      return 0;
    }

    ir_operand_destroy(&scaled);
    ir_operand_destroy(&base);
    ir_operand_destroy(&index);
    *out_address = address;
    return 1;
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary = (UnaryExpression *)expression->data;
    if (!unary || !unary->operator || !unary->operand ||
        strcmp(unary->operator, "*") != 0) {
      ir_set_error(context, "Unsupported unary lvalue");
      return 0;
    }

    Type *operand_type = ir_infer_expression_type(context, unary->operand);
    if (operand_type &&
        (operand_type->kind != TYPE_POINTER || !operand_type->base_type)) {
      ir_set_error(context, "Dereference lvalue requires pointer operand");
      return 0;
    }

    IROperand pointer_value = ir_operand_none();
    if (!ir_lower_expression(context, function, unary->operand,
                             &pointer_value)) {
      return 0;
    }

    if (out_type && operand_type && operand_type->kind == TYPE_POINTER &&
        operand_type->base_type) {
      *out_type = operand_type->base_type;
    }
    *out_address = pointer_value;
    return 1;
  }

  default:
    ir_set_error(context, "Expression is not assignable in IR lowering");
    return 0;
  }
}

static int ir_lower_expression(IRLoweringContext *context, IRFunction *function,
                               ASTNode *expression, IROperand *out_value) {
  if (!context || !function || !expression || !out_value) {
    return 0;
  }

  *out_value = ir_operand_none();

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (!literal) {
      ir_set_error(context, "Malformed number literal");
      return 0;
    }
    if (literal->is_float) {
      *out_value = ir_operand_float(literal->float_value);
    } else {
      *out_value = ir_operand_int(literal->int_value);
    }
    return 1;
  }

  case AST_STRING_LITERAL: {
    StringLiteral *literal = (StringLiteral *)expression->data;
    if (!literal || !literal->value) {
      ir_set_error(context, "Malformed string literal");
      return 0;
    }
    *out_value = ir_operand_string(literal->value);
    if (!out_value->name) {
      ir_set_error(context, "Out of memory while lowering string literal");
      return 0;
    }
    return 1;
  }

  case AST_IDENTIFIER: {
    Identifier *identifier = (Identifier *)expression->data;
    if (!identifier || !identifier->name) {
      ir_set_error(context, "Malformed identifier expression");
      return 0;
    }
    *out_value = ir_operand_symbol(identifier->name);
    if (!out_value->name) {
      ir_set_error(context, "Out of memory while lowering identifier");
      return 0;
    }
    return 1;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binary = (BinaryExpression *)expression->data;
    if (!binary || !binary->left || !binary->right || !binary->operator) {
      ir_set_error(context, "Malformed binary expression");
      return 0;
    }

    IROperand left = ir_operand_none();
    IROperand right = ir_operand_none();
    if (!ir_lower_expression(context, function, binary->left, &left) ||
        !ir_lower_expression(context, function, binary->right, &right)) {
      ir_operand_destroy(&left);
      ir_operand_destroy(&right);
      return 0;
    }

    IROperand destination = ir_operand_none();
    if (!ir_make_temp_operand(context, &destination)) {
      ir_operand_destroy(&left);
      ir_operand_destroy(&right);
      return 0;
    }

    IRInstruction instruction = {0};
    instruction.op = IR_OP_BINARY;
    instruction.location = expression->location;
    instruction.dest = destination;
    instruction.lhs = left;
    instruction.rhs = right;
    instruction.text = binary->operator;
    instruction.is_float = ir_expression_is_floating(context, expression);

    if (!ir_emit(context, function, &instruction)) {
      ir_operand_destroy(&destination);
      ir_operand_destroy(&left);
      ir_operand_destroy(&right);
      return 0;
    }

    ir_operand_destroy(&left);
    ir_operand_destroy(&right);
    *out_value = destination;
    return 1;
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary = (UnaryExpression *)expression->data;
    if (!unary || !unary->operator || !unary->operand) {
      ir_set_error(context, "Malformed unary expression");
      return 0;
    }

    if (strcmp(unary->operator, "&") == 0) {
      Type *target_type = NULL;
      if (!ir_lower_lvalue_address(context, function, unary->operand, out_value,
                                   &target_type)) {
        return 0;
      }
      return 1;
    }

    if (strcmp(unary->operator, "*") == 0) {
      IROperand address = ir_operand_none();
      Type *target_type = NULL;
      if (!ir_lower_lvalue_address(context, function, expression, &address,
                                   &target_type)) {
        return 0;
      }
      if (!target_type) {
        ir_operand_destroy(&address);
        ir_set_error(context, "Cannot dereference unknown type");
        return 0;
      }

      IROperand destination = ir_operand_none();
      if (!ir_make_temp_operand(context, &destination)) {
        ir_operand_destroy(&address);
        return 0;
      }

      IRInstruction load = {0};
      load.op = IR_OP_LOAD;
      load.location = expression->location;
      load.dest = destination;
      load.lhs = address;
      load.rhs = ir_operand_int(ir_type_storage_size(target_type));
      if (!ir_emit(context, function, &load)) {
        ir_operand_destroy(&destination);
        ir_operand_destroy(&address);
        return 0;
      }

      ir_operand_destroy(&address);
      *out_value = destination;
      return 1;
    }

    IROperand operand = ir_operand_none();
    if (!ir_lower_expression(context, function, unary->operand, &operand)) {
      return 0;
    }

    IROperand destination = ir_operand_none();
    if (!ir_make_temp_operand(context, &destination)) {
      ir_operand_destroy(&operand);
      return 0;
    }

    IRInstruction instruction = {0};
    instruction.op = IR_OP_UNARY;
    instruction.location = expression->location;
    instruction.dest = destination;
    instruction.lhs = operand;
    instruction.text = unary->operator;
    instruction.is_float = ir_expression_is_floating(context, expression);

    if (!ir_emit(context, function, &instruction)) {
      ir_operand_destroy(&destination);
      ir_operand_destroy(&operand);
      return 0;
    }

    ir_operand_destroy(&operand);
    *out_value = destination;
    return 1;
  }

  case AST_MEMBER_ACCESS:
  case AST_INDEX_EXPRESSION: {
    IROperand address = ir_operand_none();
    Type *value_type = NULL;
    if (!ir_lower_lvalue_address(context, function, expression, &address,
                                 &value_type)) {
      return 0;
    }
    if (!value_type) {
      ir_operand_destroy(&address);
      ir_set_error(context, "Cannot determine type for load");
      return 0;
    }

    IROperand destination = ir_operand_none();
    if (!ir_make_temp_operand(context, &destination)) {
      ir_operand_destroy(&address);
      return 0;
    }

    IRInstruction load = {0};
    load.op = IR_OP_LOAD;
    load.location = expression->location;
    load.dest = destination;
    load.lhs = address;
    load.rhs = ir_operand_int(ir_type_storage_size(value_type));
    if (!ir_emit(context, function, &load)) {
      ir_operand_destroy(&destination);
      ir_operand_destroy(&address);
      return 0;
    }

    ir_operand_destroy(&address);
    *out_value = destination;
    return 1;
  }

  case AST_NEW_EXPRESSION: {
    NewExpression *new_expression = (NewExpression *)expression->data;
    if (!new_expression || !new_expression->type_name) {
      ir_set_error(context, "Invalid new expression");
      return 0;
    }

    Type *allocated_type =
        context->type_checker
            ? type_checker_get_type_by_name(context->type_checker,
                                            new_expression->type_name)
            : NULL;
    int allocation_size = ir_type_storage_size(allocated_type);

    IROperand destination = ir_operand_none();
    if (!ir_make_temp_operand(context, &destination)) {
      return 0;
    }

    IRInstruction instruction = {0};
    instruction.op = IR_OP_NEW;
    instruction.location = expression->location;
    instruction.dest = destination;
    instruction.rhs = ir_operand_int(allocation_size);
    instruction.text = new_expression->type_name;
    if (!ir_emit(context, function, &instruction)) {
      ir_operand_destroy(&destination);
      return 0;
    }

    *out_value = destination;
    return 1;
  }

  case AST_FUNCTION_CALL:
    return ir_lower_call_expression(context, function, expression, out_value);

  default:
    ir_set_error(context, "Unsupported expression type in pure IR lowering");
    return 0;
  }
}

static int ir_lower_statement(IRLoweringContext *context, IRFunction *function,
                              ASTNode *statement);

static int ir_lower_statement_or_expression(IRLoweringContext *context,
                                            IRFunction *function,
                                            ASTNode *node) {
  if (!node) {
    return 1;
  }

  switch (node->type) {
  case AST_PROGRAM:
  case AST_VAR_DECLARATION:
  case AST_ASSIGNMENT:
  case AST_FUNCTION_CALL:
  case AST_RETURN_STATEMENT:
  case AST_IF_STATEMENT:
  case AST_WHILE_STATEMENT:
  case AST_FOR_STATEMENT:
  case AST_SWITCH_STATEMENT:
  case AST_BREAK_STATEMENT:
  case AST_CONTINUE_STATEMENT:
  case AST_INLINE_ASM:
    return ir_lower_statement(context, function, node);
  default: {
    IROperand value = ir_operand_none();
    int ok = ir_lower_expression(context, function, node, &value);
    ir_operand_destroy(&value);
    return ok;
  }
  }
}

static int ir_emit_label_instruction(IRLoweringContext *context,
                                     IRFunction *function, const char *label,
                                     SourceLocation location) {
  IRInstruction instruction = {0};
  instruction.op = IR_OP_LABEL;
  instruction.location = location;
  instruction.text = (char *)label;
  return ir_emit(context, function, &instruction);
}

static int ir_emit_jump_instruction(IRLoweringContext *context,
                                    IRFunction *function, const char *target,
                                    SourceLocation location) {
  IRInstruction instruction = {0};
  instruction.op = IR_OP_JUMP;
  instruction.location = location;
  instruction.text = (char *)target;
  return ir_emit(context, function, &instruction);
}

static int ir_lower_switch_statement(IRLoweringContext *context,
                                     IRFunction *function, ASTNode *statement) {
  SwitchStatement *switch_data = (SwitchStatement *)statement->data;
  if (!switch_data || !switch_data->expression) {
    ir_set_error(context, "Malformed switch statement");
    return 0;
  }

  char *switch_end_label = ir_new_label_name(context, "switch_end");
  if (!switch_end_label) {
    ir_set_error(context, "Out of memory while allocating switch labels");
    return 0;
  }

  IROperand switch_value = ir_operand_none();
  if (!ir_lower_expression(context, function, switch_data->expression,
                           &switch_value)) {
    free(switch_end_label);
    return 0;
  }

  if (!ir_push_control_frame(context, switch_end_label, NULL)) {
    ir_operand_destroy(&switch_value);
    free(switch_end_label);
    return 0;
  }

  size_t case_count = switch_data->case_count;
  char **case_labels = NULL;
  if (case_count > 0) {
    case_labels = calloc(case_count, sizeof(char *));
    if (!case_labels) {
      ir_operand_destroy(&switch_value);
      ir_pop_control_frame(context);
      free(switch_end_label);
      ir_set_error(context, "Out of memory while lowering switch cases");
      return 0;
    }
  }

  size_t default_index = (size_t)-1;
  for (size_t i = 0; i < case_count; i++) {
    case_labels[i] = ir_new_label_name(context, "case");
    if (!case_labels[i]) {
      ir_set_error(context, "Out of memory while creating switch labels");
      for (size_t j = 0; j < i; j++) {
        free(case_labels[j]);
      }
      free(case_labels);
      ir_operand_destroy(&switch_value);
      ir_pop_control_frame(context);
      free(switch_end_label);
      return 0;
    }

    ASTNode *case_node = switch_data->cases ? switch_data->cases[i] : NULL;
    CaseClause *clause = case_node ? (CaseClause *)case_node->data : NULL;
    if (clause && clause->is_default) {
      default_index = i;
    }
  }

  for (size_t i = 0; i < case_count; i++) {
    ASTNode *case_node = switch_data->cases ? switch_data->cases[i] : NULL;
    CaseClause *clause = case_node ? (CaseClause *)case_node->data : NULL;
    if (!clause || clause->is_default) {
      continue;
    }

    IROperand case_value = ir_operand_none();
    if (!ir_lower_expression(context, function, clause->value, &case_value)) {
      for (size_t j = 0; j < case_count; j++) {
        free(case_labels[j]);
      }
      free(case_labels);
      ir_operand_destroy(&switch_value);
      ir_pop_control_frame(context);
      free(switch_end_label);
      return 0;
    }

    IRInstruction compare = {0};
    compare.op = IR_OP_BRANCH_EQ;
    compare.location = case_node->location;
    compare.lhs = switch_value;
    compare.rhs = case_value;
    compare.text = case_labels[i];
    if (!ir_emit(context, function, &compare)) {
      ir_operand_destroy(&case_value);
      for (size_t j = 0; j < case_count; j++) {
        free(case_labels[j]);
      }
      free(case_labels);
      ir_operand_destroy(&switch_value);
      ir_pop_control_frame(context);
      free(switch_end_label);
      return 0;
    }

    ir_operand_destroy(&case_value);
  }

  const char *default_label = (default_index != (size_t)-1)
                                  ? case_labels[default_index]
                                  : switch_end_label;
  if (!ir_emit_jump_instruction(context, function, default_label,
                                statement->location)) {
    for (size_t j = 0; j < case_count; j++) {
      free(case_labels[j]);
    }
    free(case_labels);
    ir_operand_destroy(&switch_value);
    ir_pop_control_frame(context);
    free(switch_end_label);
    return 0;
  }

  for (size_t i = 0; i < case_count; i++) {
    ASTNode *case_node = switch_data->cases ? switch_data->cases[i] : NULL;
    CaseClause *clause = case_node ? (CaseClause *)case_node->data : NULL;
    if (!case_node || !clause) {
      continue;
    }

    if (!ir_emit_label_instruction(context, function, case_labels[i],
                                   case_node->location)) {
      for (size_t j = 0; j < case_count; j++) {
        free(case_labels[j]);
      }
      free(case_labels);
      ir_operand_destroy(&switch_value);
      ir_pop_control_frame(context);
      free(switch_end_label);
      return 0;
    }

    if (clause->body && !ir_lower_statement(context, function, clause->body)) {
      for (size_t j = 0; j < case_count; j++) {
        free(case_labels[j]);
      }
      free(case_labels);
      ir_operand_destroy(&switch_value);
      ir_pop_control_frame(context);
      free(switch_end_label);
      return 0;
    }
  }

  ir_pop_control_frame(context);
  if (!ir_emit_label_instruction(context, function, switch_end_label,
                                 statement->location)) {
    for (size_t j = 0; j < case_count; j++) {
      free(case_labels[j]);
    }
    free(case_labels);
    ir_operand_destroy(&switch_value);
    free(switch_end_label);
    return 0;
  }

  for (size_t j = 0; j < case_count; j++) {
    free(case_labels[j]);
  }
  free(case_labels);
  ir_operand_destroy(&switch_value);
  free(switch_end_label);
  return 1;
}

static int ir_lower_statement(IRLoweringContext *context, IRFunction *function,
                              ASTNode *statement) {
  if (!context || !function || !statement) {
    return 1;
  }

  switch (statement->type) {
  case AST_PROGRAM: {
    Program *program = (Program *)statement->data;
    if (!program) {
      return 1;
    }
    for (size_t i = 0; i < program->declaration_count; i++) {
      if (!ir_lower_statement(context, function, program->declarations[i])) {
        return 0;
      }
    }
    return 1;
  }

  case AST_VAR_DECLARATION: {
    VarDeclaration *declaration = (VarDeclaration *)statement->data;
    if (!declaration || !declaration->name) {
      ir_set_error(context, "Malformed variable declaration");
      return 0;
    }

    IRInstruction local = {0};
    local.op = IR_OP_DECLARE_LOCAL;
    local.location = statement->location;
    local.dest = ir_operand_symbol(declaration->name);
    local.text = declaration->type_name;
    if (!local.dest.name) {
      ir_set_error(context,
                   "Out of memory while lowering variable declaration");
      return 0;
    }
    if (!ir_emit(context, function, &local)) {
      ir_operand_destroy(&local.dest);
      return 0;
    }
    ir_operand_destroy(&local.dest);

    if (declaration->initializer) {
      IROperand value = ir_operand_none();
      if (!ir_lower_expression(context, function, declaration->initializer,
                               &value)) {
        return 0;
      }
      IRInstruction assign = {0};
      assign.op = IR_OP_ASSIGN;
      assign.location = statement->location;
      assign.dest = ir_operand_symbol(declaration->name);
      assign.lhs = value;
      if (!assign.dest.name) {
        ir_operand_destroy(&value);
        ir_set_error(context,
                     "Out of memory while lowering variable initializer");
        return 0;
      }
      if (!ir_emit(context, function, &assign)) {
        ir_operand_destroy(&assign.dest);
        ir_operand_destroy(&value);
        return 0;
      }
      ir_operand_destroy(&assign.dest);
      ir_operand_destroy(&value);
    }
    return 1;
  }

  case AST_ASSIGNMENT: {
    Assignment *assignment = (Assignment *)statement->data;
    if (!assignment || !assignment->value) {
      ir_set_error(context, "Malformed assignment statement");
      return 0;
    }

    IROperand value = ir_operand_none();
    if (!ir_lower_expression(context, function, assignment->value, &value)) {
      return 0;
    }

    if (assignment->variable_name) {
      IRInstruction assign = {0};
      assign.op = IR_OP_ASSIGN;
      assign.location = statement->location;
      assign.dest = ir_operand_symbol(assignment->variable_name);
      assign.lhs = value;
      if (!assign.dest.name) {
        ir_operand_destroy(&value);
        ir_set_error(context, "Out of memory while lowering assignment target");
        return 0;
      }

      if (!ir_emit(context, function, &assign)) {
        ir_operand_destroy(&assign.dest);
        ir_operand_destroy(&value);
        return 0;
      }

      ir_operand_destroy(&assign.dest);
      ir_operand_destroy(&value);
      return 1;
    }

    if (!assignment->target) {
      ir_operand_destroy(&value);
      ir_set_error(context, "Assignment target is missing");
      return 0;
    }

    IROperand address = ir_operand_none();
    Type *target_type = NULL;
    if (!ir_lower_lvalue_address(context, function, assignment->target,
                                 &address, &target_type)) {
      ir_operand_destroy(&value);
      return 0;
    }

    if (!target_type) {
      ir_operand_destroy(&address);
      ir_operand_destroy(&value);
      ir_set_error(context, "Cannot assign to unknown target type");
      return 0;
    }

    IRInstruction store = {0};
    store.op = IR_OP_STORE;
    store.location = statement->location;
    store.dest = address;
    store.lhs = value;
    store.rhs = ir_operand_int(ir_type_storage_size(target_type));
    if (!ir_emit(context, function, &store)) {
      ir_operand_destroy(&address);
      ir_operand_destroy(&value);
      return 0;
    }

    ir_operand_destroy(&address);
    ir_operand_destroy(&value);
    return 1;
  }

  case AST_FUNCTION_CALL: {
    IROperand ignored = ir_operand_none();
    int ok = ir_lower_expression(context, function, statement, &ignored);
    ir_operand_destroy(&ignored);
    return ok;
  }

  case AST_RETURN_STATEMENT: {
    ReturnStatement *ret = (ReturnStatement *)statement->data;
    IROperand value = ir_operand_none();
    if (ret && ret->value) {
      if (!ir_lower_expression(context, function, ret->value, &value)) {
        return 0;
      }
    }
    IRInstruction instruction = {0};
    instruction.op = IR_OP_RETURN;
    instruction.location = statement->location;
    instruction.lhs = value;
    if (!ir_emit(context, function, &instruction)) {
      ir_operand_destroy(&value);
      return 0;
    }
    ir_operand_destroy(&value);
    return 1;
  }

  case AST_INLINE_ASM: {
    InlineAsm *inline_asm = (InlineAsm *)statement->data;
    if (!inline_asm || !inline_asm->assembly_code) {
      ir_set_error(context, "Malformed inline assembly statement");
      return 0;
    }
    IRInstruction instruction = {0};
    instruction.op = IR_OP_INLINE_ASM;
    instruction.location = statement->location;
    instruction.text = inline_asm->assembly_code;
    return ir_emit(context, function, &instruction);
  }

  case AST_IF_STATEMENT: {
    IfStatement *if_data = (IfStatement *)statement->data;
    if (!if_data || !if_data->condition || !if_data->then_branch) {
      ir_set_error(context, "Malformed if statement");
      return 0;
    }

    char *else_label = ir_new_label_name(context, "if_else");
    char *end_label = ir_new_label_name(context, "if_end");
    if (!else_label || !end_label) {
      free(else_label);
      free(end_label);
      ir_set_error(context, "Out of memory while allocating if labels");
      return 0;
    }

    IROperand condition = ir_operand_none();
    if (!ir_lower_expression(context, function, if_data->condition,
                             &condition)) {
      free(else_label);
      free(end_label);
      return 0;
    }

    IRInstruction branch = {0};
    branch.op = IR_OP_BRANCH_ZERO;
    branch.location = statement->location;
    branch.lhs = condition;
    branch.text = else_label;
    if (!ir_emit(context, function, &branch)) {
      ir_operand_destroy(&condition);
      free(else_label);
      free(end_label);
      return 0;
    }
    ir_operand_destroy(&condition);

    if (!ir_lower_statement(context, function, if_data->then_branch)) {
      free(else_label);
      free(end_label);
      return 0;
    }

    if (!ir_emit_jump_instruction(context, function, end_label,
                                  statement->location)) {
      free(else_label);
      free(end_label);
      return 0;
    }

    if (!ir_emit_label_instruction(context, function, else_label,
                                   statement->location)) {
      free(else_label);
      free(end_label);
      return 0;
    }

    if (if_data->else_branch &&
        !ir_lower_statement(context, function, if_data->else_branch)) {
      free(else_label);
      free(end_label);
      return 0;
    }

    if (!ir_emit_label_instruction(context, function, end_label,
                                   statement->location)) {
      free(else_label);
      free(end_label);
      return 0;
    }

    free(else_label);
    free(end_label);
    return 1;
  }

  case AST_WHILE_STATEMENT: {
    WhileStatement *while_data = (WhileStatement *)statement->data;
    if (!while_data || !while_data->condition || !while_data->body) {
      ir_set_error(context, "Malformed while statement");
      return 0;
    }

    char *loop_start = ir_new_label_name(context, "while");
    char *loop_end = ir_new_label_name(context, "while_end");
    if (!loop_start || !loop_end) {
      free(loop_start);
      free(loop_end);
      ir_set_error(context, "Out of memory while allocating while labels");
      return 0;
    }

    if (!ir_emit_label_instruction(context, function, loop_start,
                                   statement->location)) {
      free(loop_start);
      free(loop_end);
      return 0;
    }

    IROperand condition = ir_operand_none();
    if (!ir_lower_expression(context, function, while_data->condition,
                             &condition)) {
      free(loop_start);
      free(loop_end);
      return 0;
    }

    IRInstruction branch = {0};
    branch.op = IR_OP_BRANCH_ZERO;
    branch.location = statement->location;
    branch.lhs = condition;
    branch.text = loop_end;
    if (!ir_emit(context, function, &branch)) {
      ir_operand_destroy(&condition);
      free(loop_start);
      free(loop_end);
      return 0;
    }
    ir_operand_destroy(&condition);

    if (!ir_push_control_frame(context, loop_end, loop_start)) {
      free(loop_start);
      free(loop_end);
      return 0;
    }

    int body_ok = ir_lower_statement(context, function, while_data->body);
    ir_pop_control_frame(context);
    if (!body_ok) {
      free(loop_start);
      free(loop_end);
      return 0;
    }

    if (!ir_emit_jump_instruction(context, function, loop_start,
                                  statement->location) ||
        !ir_emit_label_instruction(context, function, loop_end,
                                   statement->location)) {
      free(loop_start);
      free(loop_end);
      return 0;
    }

    free(loop_start);
    free(loop_end);
    return 1;
  }

  case AST_FOR_STATEMENT: {
    ForStatement *for_data = (ForStatement *)statement->data;
    if (!for_data || !for_data->body) {
      ir_set_error(context, "Malformed for statement");
      return 0;
    }

    char *condition_label = ir_new_label_name(context, "for_cond");
    char *step_label = ir_new_label_name(context, "for_step");
    char *end_label = ir_new_label_name(context, "for_end");
    if (!condition_label || !step_label || !end_label) {
      free(condition_label);
      free(step_label);
      free(end_label);
      ir_set_error(context, "Out of memory while allocating for-loop labels");
      return 0;
    }

    if (!ir_lower_statement_or_expression(context, function,
                                          for_data->initializer)) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    if (!ir_emit_label_instruction(context, function, condition_label,
                                   statement->location)) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    if (for_data->condition) {
      IROperand condition = ir_operand_none();
      if (!ir_lower_expression(context, function, for_data->condition,
                               &condition)) {
        free(condition_label);
        free(step_label);
        free(end_label);
        return 0;
      }
      IRInstruction branch = {0};
      branch.op = IR_OP_BRANCH_ZERO;
      branch.location = statement->location;
      branch.lhs = condition;
      branch.text = end_label;
      if (!ir_emit(context, function, &branch)) {
        ir_operand_destroy(&condition);
        free(condition_label);
        free(step_label);
        free(end_label);
        return 0;
      }
      ir_operand_destroy(&condition);
    }

    if (!ir_push_control_frame(context, end_label, step_label)) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    int body_ok = ir_lower_statement(context, function, for_data->body);
    ir_pop_control_frame(context);
    if (!body_ok) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    if (!ir_emit_label_instruction(context, function, step_label,
                                   statement->location)) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    if (!ir_lower_statement_or_expression(context, function,
                                          for_data->increment)) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    if (!ir_emit_jump_instruction(context, function, condition_label,
                                  statement->location) ||
        !ir_emit_label_instruction(context, function, end_label,
                                   statement->location)) {
      free(condition_label);
      free(step_label);
      free(end_label);
      return 0;
    }

    free(condition_label);
    free(step_label);
    free(end_label);
    return 1;
  }

  case AST_SWITCH_STATEMENT:
    return ir_lower_switch_statement(context, function, statement);

  case AST_BREAK_STATEMENT: {
    const char *target = ir_current_break_label(context);
    if (!target) {
      ir_set_error(context, "'break' used outside loop/switch");
      return 0;
    }
    return ir_emit_jump_instruction(context, function, target,
                                    statement->location);
  }

  case AST_CONTINUE_STATEMENT: {
    const char *target = ir_current_continue_label(context);
    if (!target) {
      ir_set_error(context, "'continue' used outside loop");
      return 0;
    }
    return ir_emit_jump_instruction(context, function, target,
                                    statement->location);
  }

  default: {
    if (statement->type >= AST_IDENTIFIER &&
        statement->type <= AST_NEW_EXPRESSION) {
      IROperand ignored = ir_operand_none();
      int ok = ir_lower_expression(context, function, statement, &ignored);
      ir_operand_destroy(&ignored);
      return ok;
    }

    ir_set_error(context, "Unsupported statement type in pure IR lowering");
    return 0;
  }
  }
}

static IRFunction *ir_lower_function(IRLoweringContext *context,
                                     ASTNode *declaration) {
  if (!declaration || declaration->type != AST_FUNCTION_DECLARATION) {
    return NULL;
  }

  FunctionDeclaration *function_data = (FunctionDeclaration *)declaration->data;
  if (!function_data || !function_data->name) {
    ir_set_error(context, "Malformed function declaration");
    return NULL;
  }

  IRFunction *function = ir_function_create(function_data->name);
  if (!function) {
    ir_set_error(context, "Out of memory while creating IR function");
    return NULL;
  }

  char *entry_label = ir_new_label_name(context, "entry");
  if (!entry_label) {
    ir_set_error(context,
                 "Out of memory while allocating function entry label");
    ir_function_destroy(function);
    return NULL;
  }
  if (!ir_emit_label_instruction(context, function, entry_label,
                                 declaration->location)) {
    free(entry_label);
    ir_function_destroy(function);
    return NULL;
  }
  free(entry_label);

  if (function_data->body &&
      !ir_lower_statement(context, function, function_data->body)) {
    ir_function_destroy(function);
    return NULL;
  }

  return function;
}

IRProgram *ir_lower_program(ASTNode *program, TypeChecker *type_checker,
                            SymbolTable *symbol_table, char **error_message) {
  if (error_message) {
    *error_message = NULL;
  }

  if (!program || program->type != AST_PROGRAM) {
    if (error_message) {
      *error_message =
          ir_strdup_local("Expected AST_PROGRAM root for IR lowering");
    }
    return NULL;
  }

  IRProgram *ir_program = ir_program_create();
  if (!ir_program) {
    if (error_message) {
      *error_message = ir_strdup_local("Failed to allocate IR program");
    }
    return NULL;
  }

  IRLoweringContext context = {0};
  context.type_checker = type_checker;
  context.symbol_table = symbol_table;

  Program *program_data = (Program *)program->data;
  if (!program_data) {
    return ir_program;
  }

  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *declaration = program_data->declarations[i];
    if (!declaration || declaration->type != AST_FUNCTION_DECLARATION) {
      continue;
    }
    FunctionDeclaration *function_data = (FunctionDeclaration *)declaration->data;
    if (!function_data) {
      ir_set_error(&context, "Malformed function declaration");
      ir_program_destroy(ir_program);
      for (size_t j = 0; j < context.control_count; j++) {
        free(context.control_stack[j].break_label);
        free(context.control_stack[j].continue_label);
      }
      free(context.control_stack);
      if (error_message) {
        *error_message = context.error_message
                             ? context.error_message
                             : ir_strdup_local("Unknown IR lowering error");
      } else {
        free(context.error_message);
      }
      return NULL;
    }
    if (!function_data->body) {
      continue;
    }

    IRFunction *function = ir_lower_function(&context, declaration);
    if (!function) {
      if (!context.error_message) {
        ir_set_error(&context, "Failed to lower function declaration to IR");
      }
      ir_program_destroy(ir_program);
      for (size_t j = 0; j < context.control_count; j++) {
        free(context.control_stack[j].break_label);
        free(context.control_stack[j].continue_label);
      }
      free(context.control_stack);
      if (error_message) {
        *error_message = context.error_message
                             ? context.error_message
                             : ir_strdup_local("Unknown IR lowering error");
      } else {
        free(context.error_message);
      }
      return NULL;
    }

    if (!ir_program_add_function(ir_program, function)) {
      ir_function_destroy(function);
      ir_set_error(&context, "Out of memory while appending IR function");
      ir_program_destroy(ir_program);
      for (size_t j = 0; j < context.control_count; j++) {
        free(context.control_stack[j].break_label);
        free(context.control_stack[j].continue_label);
      }
      free(context.control_stack);
      if (error_message) {
        *error_message = context.error_message
                             ? context.error_message
                             : ir_strdup_local("Unknown IR lowering error");
      } else {
        free(context.error_message);
      }
      return NULL;
    }
  }

  for (size_t j = 0; j < context.control_count; j++) {
    free(context.control_stack[j].break_label);
    free(context.control_stack[j].continue_label);
  }
  free(context.control_stack);

  if (context.error_message) {
    if (error_message) {
      *error_message = context.error_message;
    } else {
      free(context.error_message);
    }
  }

  return ir_program;
}
