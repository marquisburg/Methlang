#include "type_checker.h"
#include "../error/error_reporter.h"
#include "symbol_table.h"
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static Type *type_checker_parse_array_type(TypeChecker *checker,
                                           const char *name) {
  if (!checker || !name)
    return NULL;

  const char *lbracket = strchr(name, '[');
  const char *rbracket = lbracket ? strchr(lbracket, ']') : NULL;
  if (!lbracket || !rbracket || rbracket[1] != '\0') {
    return NULL;
  }

  size_t base_len = (size_t)(lbracket - name);
  if (base_len == 0) {
    return NULL;
  }

  char *base_name = malloc(base_len + 1);
  if (!base_name) {
    return NULL;
  }
  memcpy(base_name, name, base_len);
  base_name[base_len] = '\0';

  Type *base_type = type_checker_get_type_by_name(checker, base_name);
  free(base_name);
  if (!base_type) {
    return NULL;
  }

  const char *size_start = lbracket + 1;
  if (size_start == rbracket) {
    return NULL;
  }

  errno = 0;
  char *end_ptr = NULL;
  unsigned long long array_size_ull = strtoull(size_start, &end_ptr, 10);
  if (errno != 0 || !end_ptr || end_ptr != rbracket || array_size_ull == 0 ||
      array_size_ull > SIZE_MAX) {
    return NULL;
  }

  size_t array_size = (size_t)array_size_ull;
  if (base_type->size > 0 && array_size > SIZE_MAX / base_type->size) {
    return NULL;
  }

  Type *array_type = type_create(TYPE_ARRAY, name);
  if (!array_type) {
    return NULL;
  }

  array_type->base_type = base_type;
  array_type->array_size = array_size;
  array_type->size = base_type->size * array_size;
  array_type->alignment = base_type->alignment;

  return array_type;
}

static Type *type_checker_parse_pointer_type(TypeChecker *checker,
                                             const char *name) {
  if (!checker || !name) {
    return NULL;
  }

  size_t name_len = strlen(name);
  size_t pointer_depth = 0;
  while (name_len > 0 && name[name_len - 1] == '*') {
    pointer_depth++;
    name_len--;
  }

  if (pointer_depth == 0 || name_len == 0) {
    return NULL;
  }

  char *base_name = malloc(name_len + 1);
  if (!base_name) {
    return NULL;
  }
  memcpy(base_name, name, name_len);
  base_name[name_len] = '\0';

  Type *base_type = type_checker_get_type_by_name(checker, base_name);
  free(base_name);
  if (!base_type) {
    return NULL;
  }

  Type *current = base_type;
  for (size_t i = 0; i < pointer_depth; i++) {
    const char *current_name = current && current->name ? current->name : "ptr";
    size_t pointer_name_len = strlen(current_name) + 2;
    char *pointer_name = malloc(pointer_name_len);
    if (!pointer_name) {
      return NULL;
    }
    snprintf(pointer_name, pointer_name_len, "%s*", current_name);

    Type *pointer_type = type_create(TYPE_POINTER, pointer_name);
    free(pointer_name);
    if (!pointer_type) {
      return NULL;
    }

    pointer_type->base_type = current;
    pointer_type->size = 8;
    pointer_type->alignment = 8;
    current = pointer_type;
  }

  return current;
}

static int type_checker_types_equal(const Type *lhs, const Type *rhs) {
  if (lhs == rhs) {
    return 1;
  }
  if (!lhs || !rhs) {
    return 0;
  }
  if (lhs->kind != rhs->kind) {
    return 0;
  }

  switch (lhs->kind) {
  case TYPE_POINTER:
    return type_checker_types_equal(lhs->base_type, rhs->base_type);
  case TYPE_ARRAY:
    return lhs->array_size == rhs->array_size &&
           type_checker_types_equal(lhs->base_type, rhs->base_type);
  case TYPE_STRUCT:
    if (lhs->name && rhs->name) {
      return strcmp(lhs->name, rhs->name) == 0;
    }
    return lhs->name == rhs->name;
  default:
    return 1;
  }
}

static int type_checker_is_lvalue_expression(ASTNode *expression) {
  if (!expression) {
    return 0;
  }

  switch (expression->type) {
  case AST_IDENTIFIER:
  case AST_MEMBER_ACCESS:
  case AST_INDEX_EXPRESSION:
    return 1;
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary = (UnaryExpression *)expression->data;
    return unary && unary->operator && strcmp(unary->operator, "*") == 0;
  }
  default:
    return 0;
  }
}

static int type_checker_eval_integer_constant(ASTNode *expression,
                                              long long *out_value) {
  if (!expression || !out_value) {
    return 0;
  }

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (!literal || literal->is_float) {
      return 0;
    }
    *out_value = literal->int_value;
    return 1;
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unary_expr = (UnaryExpression *)expression->data;
    long long operand = 0;
    if (!unary_expr || !unary_expr->operator || !unary_expr->operand ||
        !type_checker_eval_integer_constant(unary_expr->operand, &operand)) {
      return 0;
    }

    if (strcmp(unary_expr->operator, "+") == 0) {
      *out_value = operand;
      return 1;
    }
    if (strcmp(unary_expr->operator, "-") == 0) {
      *out_value = -operand;
      return 1;
    }
    return 0;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binary_expr = (BinaryExpression *)expression->data;
    long long left = 0;
    long long right = 0;
    if (!binary_expr || !binary_expr->operator || !binary_expr->left ||
        !binary_expr->right ||
        !type_checker_eval_integer_constant(binary_expr->left, &left) ||
        !type_checker_eval_integer_constant(binary_expr->right, &right)) {
      return 0;
    }

    if (strcmp(binary_expr->operator, "+") == 0) {
      *out_value = left + right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "-") == 0) {
      *out_value = left - right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "*") == 0) {
      *out_value = left * right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "/") == 0) {
      if (right == 0) {
        return 0;
      }
      *out_value = left / right;
      return 1;
    }
    if (strcmp(binary_expr->operator, "%") == 0) {
      if (right == 0) {
        return 0;
      }
      *out_value = left % right;
      return 1;
    }
    return 0;
  }

  default:
    return 0;
  }
}

static int type_checker_is_null_pointer_constant(ASTNode *expression) {
  long long value = 0;
  return type_checker_eval_integer_constant(expression, &value) && value == 0;
}

static const char *type_checker_decl_link_name(const char *name, int is_extern,
                                               const char *link_name) {
  if (!is_extern) {
    return name;
  }
  if (link_name && link_name[0] != '\0') {
    return link_name;
  }
  return name;
}

static const char *type_checker_symbol_link_name(const Symbol *symbol) {
  if (!symbol) {
    return NULL;
  }
  if (symbol->is_extern && symbol->link_name && symbol->link_name[0] != '\0') {
    return symbol->link_name;
  }
  return symbol->name;
}

static int type_checker_link_name_matches_symbol(const Symbol *symbol,
                                                 const char *decl_name,
                                                 int decl_is_extern,
                                                 const char *decl_link_name) {
  const char *existing = type_checker_symbol_link_name(symbol);
  const char *incoming =
      type_checker_decl_link_name(decl_name, decl_is_extern, decl_link_name);
  if (!existing || !incoming) {
    return existing == incoming;
  }
  return strcmp(existing, incoming) == 0;
}

TypeChecker *type_checker_create(SymbolTable *symbol_table) {
  return type_checker_create_with_error_reporter(symbol_table, NULL);
}

TypeChecker *
type_checker_create_with_error_reporter(SymbolTable *symbol_table,
                                        ErrorReporter *error_reporter) {
  TypeChecker *checker = malloc(sizeof(TypeChecker));
  if (!checker)
    return NULL;

  checker->symbol_table = symbol_table;
  checker->has_error = 0;
  checker->error_message = NULL;
  checker->error_reporter = error_reporter;
  checker->current_function = NULL;
  checker->loop_depth = 0;
  checker->switch_depth = 0;

  // Initialize built-in type pointers to NULL
  checker->builtin_int8 = NULL;
  checker->builtin_int16 = NULL;
  checker->builtin_int32 = NULL;
  checker->builtin_int64 = NULL;
  checker->builtin_uint8 = NULL;
  checker->builtin_uint16 = NULL;
  checker->builtin_uint32 = NULL;
  checker->builtin_uint64 = NULL;
  checker->builtin_float32 = NULL;
  checker->builtin_float64 = NULL;
  checker->builtin_string = NULL;
  checker->builtin_cstring = NULL;
  checker->builtin_void = NULL;

  // Initialize built-in types
  type_checker_init_builtin_types(checker);

  return checker;
}

void type_checker_destroy(TypeChecker *checker) {
  if (checker) {
    // Clean up built-in types
    type_destroy(checker->builtin_int8);
    type_destroy(checker->builtin_int16);
    type_destroy(checker->builtin_int32);
    type_destroy(checker->builtin_int64);
    type_destroy(checker->builtin_uint8);
    type_destroy(checker->builtin_uint16);
    type_destroy(checker->builtin_uint32);
    type_destroy(checker->builtin_uint64);
    type_destroy(checker->builtin_float32);
    type_destroy(checker->builtin_float64);
    type_destroy(checker->builtin_string);
    type_destroy(checker->builtin_cstring);
    type_destroy(checker->builtin_void);

    free(checker->error_message);
    free(checker);
  }
}

int type_checker_check_program(TypeChecker *checker, ASTNode *program) {
  if (!checker || !program || program->type != AST_PROGRAM) {
    return 0;
  }

  Program *prog = (Program *)program->data;
  if (!prog)
    return 0;

  // First pass: Process struct declarations to register struct types
  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    if (decl && decl->type == AST_STRUCT_DECLARATION) {
      if (!type_checker_process_struct_declaration(checker, decl)) {
        return 0; // Error in struct declaration
      }
    }
  }

  // Second pass: Process other declarations and type check them
  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    if (decl && decl->type != AST_STRUCT_DECLARATION) {
      if (!type_checker_process_declaration(checker, decl)) {
        return 0; // Error in declaration
      }
    }
  }

  return 1; // Success
}

static Type *type_checker_infer_type_internal(TypeChecker *checker,
                                              ASTNode *expression);

Type *type_checker_infer_type(TypeChecker *checker, ASTNode *expression) {
  if (!checker || !expression)
    return NULL;

  if (expression->resolved_type) {
    return expression->resolved_type;
  }

  Type *type = type_checker_infer_type_internal(checker, expression);
  expression->resolved_type = type;
  return type;
}

static Type *type_checker_infer_type_internal(TypeChecker *checker,
                                              ASTNode *expression) {
  if (!checker || !expression)
    return NULL;

  switch (expression->type) {
  case AST_NUMBER_LITERAL: {
    NumberLiteral *literal = (NumberLiteral *)expression->data;
    if (literal->is_float) {
      // Floating literals default to float64
      return checker->builtin_float64;
    } else {
      // Integer literals default to int32
      return checker->builtin_int32;
    }
  }

  case AST_STRING_LITERAL:
    // String literals are string type
    return checker->builtin_string;

  case AST_IDENTIFIER: {
    Identifier *id = (Identifier *)expression->data;
    Symbol *symbol = symbol_table_lookup(checker->symbol_table, id->name);
    if (!symbol) {
      type_checker_report_undefined_symbol(checker, expression->location,
                                           id->name, "variable");
      return NULL;
    }
    return symbol->type;
  }

  case AST_BINARY_EXPRESSION: {
    BinaryExpression *binop = (BinaryExpression *)expression->data;
    return type_checker_check_binary_expression(checker, binop,
                                                expression->location);
  }

  case AST_UNARY_EXPRESSION: {
    UnaryExpression *unop = (UnaryExpression *)expression->data;
    if (!unop || !unop->operator || !unop->operand) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid unary expression");
      return NULL;
    }

    if (strcmp(unop->operator, "&") == 0) {
      if (!type_checker_is_lvalue_expression(unop->operand)) {
        type_checker_set_error_at_location(
            checker, unop->operand->location,
            "Address-of operator requires an assignable expression");
        return NULL;
      }

      Type *operand_type = type_checker_infer_type(checker, unop->operand);
      if (!operand_type) {
        return NULL;
      }

      const char *operand_name =
          operand_type->name ? operand_type->name : "unknown";
      size_t pointer_name_len = strlen(operand_name) + 2;
      char *pointer_name = malloc(pointer_name_len);
      if (!pointer_name) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Memory allocation failed");
        return NULL;
      }
      snprintf(pointer_name, pointer_name_len, "%s*", operand_name);

      Type *pointer_type = type_checker_get_type_by_name(checker, pointer_name);
      free(pointer_name);
      if (!pointer_type) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Failed to resolve pointer type");
        return NULL;
      }

      return pointer_type;
    }

    if (strcmp(unop->operator, "*") == 0) {
      Type *operand_type = type_checker_infer_type(checker, unop->operand);
      if (!operand_type) {
        return NULL;
      }
      if (operand_type->kind != TYPE_POINTER || !operand_type->base_type) {
        type_checker_set_error_at_location(
            checker, expression->location,
            "Dereference operator requires a pointer operand");
        return NULL;
      }
      return operand_type->base_type;
    }

    Type *operand_type = type_checker_infer_type(checker, unop->operand);
    if (!operand_type) {
      return NULL;
    }

    if (strcmp(unop->operator, "+") == 0 || strcmp(unop->operator, "-") == 0) {
      if (!type_checker_is_numeric_type(operand_type)) {
        type_checker_report_type_mismatch(checker, unop->operand->location,
                                          "numeric type", operand_type->name);
        return NULL;
      }
      return operand_type;
    }

    type_checker_set_error_at_location(checker, expression->location,
                                       "Unsupported unary operator '%s'",
                                       unop->operator);
    return NULL;
  }

  case AST_FUNCTION_CALL: {
    CallExpression *call = (CallExpression *)expression->data;
    Symbol *func_symbol =
        symbol_table_lookup(checker->symbol_table, call->function_name);
    if (!func_symbol) {
      type_checker_report_undefined_symbol(checker, expression->location,
                                           call->function_name, "function");
      return NULL;
    }

    if (func_symbol->kind != SYMBOL_FUNCTION) {
      const char *symbol_type =
          (func_symbol->kind == SYMBOL_VARIABLE) ? "variable"
          : (func_symbol->kind == SYMBOL_STRUCT) ? "struct"
                                                 : "symbol";
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg), "'%s' is a %s, not a function",
               call->function_name, symbol_type);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    // Check argument count
    if (call->argument_count != func_symbol->data.function.parameter_count) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Function '%s' expects %llu arguments, got %llu",
               call->function_name,
               (unsigned long long)func_symbol->data.function.parameter_count,
               (unsigned long long)call->argument_count);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    // Check each argument type
    for (size_t i = 0; i < call->argument_count; i++) {
      Type *arg_type = type_checker_infer_type(checker, call->arguments[i]);
      if (!arg_type) {
        // Error already set by type inference
        return NULL;
      }

      Type *param_type = func_symbol->data.function.parameter_types[i];
      int is_null_pointer_arg =
          (param_type && param_type->kind == TYPE_POINTER &&
           type_checker_is_null_pointer_constant(call->arguments[i]));
      if (!is_null_pointer_arg &&
          !type_checker_is_assignable(checker, param_type, arg_type)) {
        type_checker_report_type_mismatch(checker, call->arguments[i]->location,
                                          param_type->name, arg_type->name);
        return NULL;
      }
    }

    return func_symbol->data.function.return_type;
  }

  case AST_MEMBER_ACCESS: {
    MemberAccess *member = (MemberAccess *)expression->data;
    Type *object_type = type_checker_infer_type(checker, member->object);
    if (object_type && (object_type->kind == TYPE_STRUCT ||
                        object_type->kind == TYPE_STRING)) {
      // Look up the field type in the struct
      Type *field_type = type_get_field_type(object_type, member->member);
      if (field_type) {
        return field_type;
      } else {
        // Field not found in struct - this is an error
        SourceLocation location = expression->location;
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg),
                 "Field '%s' not found in type '%s'", member->member,
                 object_type->name);
        type_checker_set_error_at_location(checker, location, error_msg);
        return NULL;
      }
    } else if (object_type) {
      // Trying to access member on non-struct type
      SourceLocation location = expression->location;
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Cannot access field on non-struct type '%s'",
               object_type->name);
      type_checker_set_error_at_location(checker, location, error_msg);
      return NULL;
    }
    return NULL;
  }

  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *idx = (ArrayIndexExpression *)expression->data;
    if (!idx || !idx->array || !idx->index) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid array indexing expression");
      return NULL;
    }

    Type *array_type = type_checker_infer_type(checker, idx->array);
    if (!array_type) {
      return NULL;
    }

    Type *index_type = type_checker_infer_type(checker, idx->index);
    if (!index_type) {
      return NULL;
    }

    if (!type_checker_is_integer_type(index_type)) {
      type_checker_report_type_mismatch(checker, idx->index->location,
                                        "integer type", index_type->name);
      return NULL;
    }

    if (array_type->kind == TYPE_ARRAY || array_type->kind == TYPE_POINTER) {
      if (!array_type->base_type) {
        type_checker_set_error_at_location(checker, expression->location,
                                           "Indexed type has no element type");
        return NULL;
      }
      return array_type->base_type;
    }

    type_checker_set_error_at_location(checker, expression->location,
                                       "Cannot index non-array type '%s'",
                                       array_type->name);
    return NULL;
  }

  case AST_ASSIGNMENT: {
    Assignment *assignment = (Assignment *)expression->data;
    if (assignment && assignment->value) {
      return type_checker_infer_type(checker, assignment->value);
    }
    return NULL;
  }

  case AST_NEW_EXPRESSION: {
    NewExpression *new_expr = (NewExpression *)expression->data;
    if (!new_expr || !new_expr->type_name) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Invalid 'new' expression");
      return NULL;
    }

    // Look up the type by name
    Symbol *type_symbol =
        symbol_table_lookup(checker->symbol_table, new_expr->type_name);
    if (!type_symbol || type_symbol->kind != SYMBOL_STRUCT) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Struct type '%s' not found for allocation",
               new_expr->type_name);
      type_checker_set_error_at_location(checker, expression->location,
                                         error_msg);
      return NULL;
    }

    size_t pointer_name_len = strlen(new_expr->type_name) + 2;
    char *pointer_name = malloc(pointer_name_len);
    if (!pointer_name) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Memory allocation failed");
      return NULL;
    }
    snprintf(pointer_name, pointer_name_len, "%s*", new_expr->type_name);

    Type *pointer_type = type_checker_get_type_by_name(checker, pointer_name);
    free(pointer_name);
    if (!pointer_type) {
      type_checker_set_error_at_location(checker, expression->location,
                                         "Failed to resolve pointer type");
      return NULL;
    }

    return pointer_type;
  }

  default:
    return NULL;
  }
}

int type_checker_are_compatible(Type *type1, Type *type2) {
  if (!type1 || !type2)
    return 0;

  if (type_checker_types_equal(type1, type2)) {
    return 1;
  }

  if (type1->kind == TYPE_POINTER || type2->kind == TYPE_POINTER ||
      type1->kind == TYPE_ARRAY || type2->kind == TYPE_ARRAY ||
      type1->kind == TYPE_STRUCT || type2->kind == TYPE_STRUCT) {
    return 0;
  }

  // Check for implicit numeric conversions
  return type_checker_is_implicitly_convertible(type1, type2) ||
         type_checker_is_implicitly_convertible(type2, type1);
}

// Built-in type system functions implementation

void type_checker_init_builtin_types(TypeChecker *checker) {
  if (!checker)
    return;

  // Create built-in integer types
  checker->builtin_int8 = type_create(TYPE_INT8, "int8");
  checker->builtin_int16 = type_create(TYPE_INT16, "int16");
  checker->builtin_int32 = type_create(TYPE_INT32, "int32");
  checker->builtin_int64 = type_create(TYPE_INT64, "int64");

  // Create built-in unsigned integer types
  checker->builtin_uint8 = type_create(TYPE_UINT8, "uint8");
  checker->builtin_uint16 = type_create(TYPE_UINT16, "uint16");
  checker->builtin_uint32 = type_create(TYPE_UINT32, "uint32");
  checker->builtin_uint64 = type_create(TYPE_UINT64, "uint64");

  // Create built-in floating-point types
  checker->builtin_float32 = type_create(TYPE_FLOAT32, "float32");
  checker->builtin_float64 = type_create(TYPE_FLOAT64, "float64");

  // C interop alias: cstring -> uint8*
  checker->builtin_cstring = type_create(TYPE_POINTER, "cstring");
  if (checker->builtin_cstring) {
    checker->builtin_cstring->base_type = checker->builtin_uint8;
    checker->builtin_cstring->size = 8;
    checker->builtin_cstring->alignment = 8;
  }

  // Create built-in string type backed by a uint8* and length
  checker->builtin_string = type_create(TYPE_STRING, "string");
  if (checker->builtin_string) {
    checker->builtin_string->size = 16;
    checker->builtin_string->alignment = 8;

    checker->builtin_string->field_count = 2;
    checker->builtin_string->field_names = malloc(2 * sizeof(char *));
    checker->builtin_string->field_types = malloc(2 * sizeof(Type *));
    checker->builtin_string->field_offsets = malloc(2 * sizeof(size_t));

    checker->builtin_string->field_names[0] = strdup("chars");
    checker->builtin_string->field_types[0] =
        type_create(TYPE_POINTER, "uint8*");
    checker->builtin_string->field_types[0]->base_type = checker->builtin_uint8;
    checker->builtin_string->field_types[0]->size = 8;
    checker->builtin_string->field_types[0]->alignment = 8;
    checker->builtin_string->field_offsets[0] = 0;

    checker->builtin_string->field_names[1] = strdup("length");
    checker->builtin_string->field_types[1] = checker->builtin_uint64;
    checker->builtin_string->field_offsets[1] = 8;
  }

  // Create built-in void type
  checker->builtin_void = type_create(TYPE_VOID, "void");
  if (checker->builtin_void) {
    checker->builtin_void->size = 0;
    checker->builtin_void->alignment = 1;
  }
}

Type *type_checker_get_builtin_type(TypeChecker *checker, TypeKind kind) {
  if (!checker)
    return NULL;

  switch (kind) {
  case TYPE_INT8:
    return checker->builtin_int8;
  case TYPE_INT16:
    return checker->builtin_int16;
  case TYPE_INT32:
    return checker->builtin_int32;
  case TYPE_INT64:
    return checker->builtin_int64;
  case TYPE_UINT8:
    return checker->builtin_uint8;
  case TYPE_UINT16:
    return checker->builtin_uint16;
  case TYPE_UINT32:
    return checker->builtin_uint32;
  case TYPE_UINT64:
    return checker->builtin_uint64;
  case TYPE_FLOAT32:
    return checker->builtin_float32;
  case TYPE_FLOAT64:
    return checker->builtin_float64;
  case TYPE_STRING:
    return checker->builtin_string;
  case TYPE_VOID:
    return checker->builtin_void;
  default:
    return NULL;
  }
}

Type *type_checker_get_type_by_name(TypeChecker *checker, const char *name) {
  if (!checker || !name)
    return NULL;

  // Check built-in types by name
  if (strcmp(name, "int8") == 0)
    return checker->builtin_int8;
  if (strcmp(name, "int16") == 0)
    return checker->builtin_int16;
  if (strcmp(name, "int32") == 0)
    return checker->builtin_int32;
  if (strcmp(name, "int64") == 0)
    return checker->builtin_int64;
  if (strcmp(name, "uint8") == 0)
    return checker->builtin_uint8;
  if (strcmp(name, "uint16") == 0)
    return checker->builtin_uint16;
  if (strcmp(name, "uint32") == 0)
    return checker->builtin_uint32;
  if (strcmp(name, "uint64") == 0)
    return checker->builtin_uint64;
  if (strcmp(name, "float32") == 0)
    return checker->builtin_float32;
  if (strcmp(name, "float64") == 0)
    return checker->builtin_float64;
  if (strcmp(name, "string") == 0)
    return checker->builtin_string;
  if (strcmp(name, "cstring") == 0)
    return checker->builtin_cstring;
  if (strcmp(name, "void") == 0)
    return checker->builtin_void;

  if (strchr(name, '[') && strchr(name, ']')) {
    Type *array_type = type_checker_parse_array_type(checker, name);
    if (array_type) {
      return array_type;
    }
  }

  if (strchr(name, '*')) {
    Type *pointer_type = type_checker_parse_pointer_type(checker, name);
    if (pointer_type) {
      return pointer_type;
    }
  }

  // Check for user-defined struct types in symbol table
  Symbol *struct_symbol = symbol_table_lookup(checker->symbol_table, name);
  if (struct_symbol && struct_symbol->kind == SYMBOL_STRUCT) {
    return struct_symbol->type;
  }

  return NULL;
}

int type_checker_is_integer_type(Type *type) {
  if (!type)
    return 0;

  switch (type->kind) {
  case TYPE_INT8:
  case TYPE_INT16:
  case TYPE_INT32:
  case TYPE_INT64:
  case TYPE_UINT8:
  case TYPE_UINT16:
  case TYPE_UINT32:
  case TYPE_UINT64:
    return 1;
  default:
    return 0;
  }
}

int type_checker_is_floating_type(Type *type) {
  if (!type)
    return 0;

  switch (type->kind) {
  case TYPE_FLOAT32:
  case TYPE_FLOAT64:
    return 1;
  default:
    return 0;
  }
}

int type_checker_is_numeric_type(Type *type) {
  return type_checker_is_integer_type(type) ||
         type_checker_is_floating_type(type);
}

size_t type_checker_get_type_size(Type *type) {
  if (!type)
    return 0;
  return type->size;
}

size_t type_checker_get_type_alignment(Type *type) {
  if (!type)
    return 1;
  return type->alignment;
}

// Type inference and promotion functions implementation

Type *type_checker_promote_types(TypeChecker *checker, Type *left, Type *right,
                                 const char *operator) {
  if (!checker || !left || !right || !operator)
    return NULL;

  // For comparison operators, result is always int32 (boolean represented as
  // int)
  if (strcmp(operator, "==") == 0 || strcmp(operator, "!=") == 0 ||
      strcmp(operator, "<") == 0 || strcmp(operator, "<=") == 0 ||
      strcmp(operator, ">") == 0 || strcmp(operator, ">=") == 0) {
    return checker->builtin_int32;
  }

  // For arithmetic operators, promote to larger type
  if (strcmp(operator, "+") == 0 || strcmp(operator, "-") == 0 ||
      strcmp(operator, "*") == 0 || strcmp(operator, "/") == 0 ||
      strcmp(operator, "%") == 0) {

    // If either operand is floating-point, result is floating-point
    if (type_checker_is_floating_type(left) ||
        type_checker_is_floating_type(right)) {
      return type_checker_get_larger_type(checker, left, right);
    }

    // Both are integers, promote to larger integer type
    if (type_checker_is_integer_type(left) &&
        type_checker_is_integer_type(right)) {
      return type_checker_get_larger_type(checker, left, right);
    }
  }

  // For logical operators, result is int32 (boolean)
  if (strcmp(operator, "&&") == 0 || strcmp(operator, "||") == 0) {
    return checker->builtin_int32;
  }

  // Default: return left type
  return left;
}

Type *type_checker_get_larger_type(TypeChecker *checker, Type *type1,
                                   Type *type2) {
  if (!checker || !type1 || !type2)
    return NULL;

  int rank1 = type_checker_get_type_rank(type1);
  int rank2 = type_checker_get_type_rank(type2);

  // Return the type with higher rank
  return (rank1 >= rank2) ? type1 : type2;
}

int type_checker_get_type_rank(Type *type) {
  if (!type)
    return -1;

  // Type promotion ranking (higher number = higher rank)
  switch (type->kind) {
  case TYPE_INT8:
  case TYPE_UINT8:
    return 1;
  case TYPE_INT16:
  case TYPE_UINT16:
    return 2;
  case TYPE_INT32:
  case TYPE_UINT32:
    return 3;
  case TYPE_FLOAT32:
    return 4;
  case TYPE_INT64:
  case TYPE_UINT64:
    return 5;
  case TYPE_FLOAT64:
    return 6;
  case TYPE_STRING:
    return 10; // Special case - strings don't promote with numbers
  default:
    return 0;
  }
}

Type *type_checker_infer_variable_type(TypeChecker *checker,
                                       ASTNode *initializer) {
  if (!checker || !initializer)
    return NULL;

  // Use the general type inference function
  return type_checker_infer_type(checker, initializer);
}

// Type compatibility and conversion functions implementation

int type_checker_is_assignable(TypeChecker *checker, Type *dest_type,
                               Type *src_type) {
  if (!checker || !dest_type || !src_type)
    return 0;

  if (type_checker_types_equal(dest_type, src_type)) {
    return 1;
  }

  if (dest_type->kind == TYPE_POINTER || src_type->kind == TYPE_POINTER ||
      dest_type->kind == TYPE_ARRAY || src_type->kind == TYPE_ARRAY ||
      dest_type->kind == TYPE_STRUCT || src_type->kind == TYPE_STRUCT) {
    return 0;
  }

  // Check for safe implicit conversions
  return type_checker_is_implicitly_convertible(src_type, dest_type);
}

int type_checker_is_implicitly_convertible(Type *from_type, Type *to_type) {
  if (!from_type || !to_type)
    return 0;

  // Same type is always convertible
  if (from_type->kind == to_type->kind) {
    return type_checker_types_equal(from_type, to_type);
  }

  // Integer to integer conversions, including narrowing.
  if (type_checker_is_integer_type(from_type) &&
      type_checker_is_integer_type(to_type)) {
    return 1;
  }

  // Integer to floating point conversions
  if (type_checker_is_integer_type(from_type) &&
      type_checker_is_floating_type(to_type)) {
    return 1; // Generally safe
  }

  // Floating point to floating point conversions, including narrowing.
  if (type_checker_is_floating_type(from_type) &&
      type_checker_is_floating_type(to_type)) {
    return 1;
  }

  // No other implicit conversions are allowed
  return 0;
}

int type_checker_validate_function_call(TypeChecker *checker,
                                        CallExpression *call,
                                        Symbol *func_symbol) {
  if (!checker || !call || !func_symbol)
    return 0;

  // Check argument count
  if (call->argument_count != func_symbol->data.function.parameter_count) {
    type_checker_set_error(
        checker, "Function '%s' expects %llu arguments, got %llu",
        call->function_name,
        (unsigned long long)func_symbol->data.function.parameter_count,
        (unsigned long long)call->argument_count);
    return 0;
  }

  // Check each argument type
  for (size_t i = 0; i < call->argument_count; i++) {
    Type *arg_type = type_checker_infer_type(checker, call->arguments[i]);
    if (!arg_type) {
      type_checker_set_error(
          checker, "Cannot infer type for argument %zu in call to '%s'", i + 1,
          call->function_name);
      return 0;
    }

    Type *param_type = func_symbol->data.function.parameter_types[i];
    int is_null_pointer_arg =
        (param_type && param_type->kind == TYPE_POINTER &&
         type_checker_is_null_pointer_constant(call->arguments[i]));
    if (!is_null_pointer_arg &&
        !type_checker_is_assignable(checker, param_type, arg_type)) {
      type_checker_set_error(
          checker,
          "Argument %zu in call to '%s': cannot convert from '%s' to '%s'",
          i + 1, call->function_name,
          arg_type->name ? arg_type->name : "unknown",
          param_type->name ? param_type->name : "unknown");
      return 0;
    }
  }

  return 1;
}

int type_checker_validate_assignment(TypeChecker *checker, Type *dest_type,
                                     ASTNode *src_expr) {
  if (!checker || !dest_type || !src_expr)
    return 0;

  Type *src_type = type_checker_infer_type(checker, src_expr);
  if (!src_type) {
    if (!checker->has_error) {
      type_checker_set_error(checker, "Cannot infer type of assignment value");
    }
    return 0;
  }

  if (dest_type->kind == TYPE_POINTER &&
      type_checker_is_null_pointer_constant(src_expr)) {
    return 1;
  }

  if (!type_checker_is_assignable(checker, dest_type, src_type)) {
    type_checker_set_error(
        checker, "Cannot assign value of type '%s' to variable of type '%s'",
        src_type->name ? src_type->name : "unknown",
        dest_type->name ? dest_type->name : "unknown");
    return 0;
  }

  return 1;
}

void type_checker_set_error(TypeChecker *checker, const char *format, ...) {
  if (!checker || !format)
    return;

  // Free previous error message
  free(checker->error_message);

  // Calculate required buffer size
  va_list args1, args2;
  va_start(args1, format);
  va_copy(args2, args1);

  int size = vsnprintf(NULL, 0, format, args1);
  va_end(args1);

  if (size < 0) {
    checker->error_message = NULL;
    checker->has_error = 1;
    va_end(args2);
    return;
  }

  // Allocate and format the message
  checker->error_message = malloc(size + 1);
  if (checker->error_message) {
    vsnprintf(checker->error_message, size + 1, format, args2);
  }

  va_end(args2);
  checker->has_error = 1;
}

// Struct type processing functions

int type_checker_process_struct_declaration(TypeChecker *checker,
                                            ASTNode *struct_decl) {
  if (!checker || !struct_decl || struct_decl->type != AST_STRUCT_DECLARATION) {
    return 0;
  }

  StructDeclaration *decl = (StructDeclaration *)struct_decl->data;
  if (!decl || !decl->name) {
    return 0;
  }

  // Check if struct already exists
  Symbol *existing =
      symbol_table_lookup_current_scope(checker->symbol_table, decl->name);
  if (existing) {
    type_checker_report_duplicate_declaration(checker, struct_decl->location,
                                              decl->name);
    return 0;
  }

  // Resolve field types
  Type **field_types = malloc(decl->field_count * sizeof(Type *));
  if (!field_types)
    return 0;

  for (size_t i = 0; i < decl->field_count; i++) {
    field_types[i] =
        type_checker_get_type_by_name(checker, decl->field_types[i]);
    if (!field_types[i]) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg),
               "Unknown type '%s' in struct field", decl->field_types[i]);
      type_checker_set_error_at_location(checker, struct_decl->location,
                                         error_msg);
      free(field_types);
      return 0;
    }
  }

  // Create struct type
  Type *struct_type = type_create_struct(decl->name, decl->field_names,
                                         field_types, decl->field_count);
  if (!struct_type) {
    free(field_types);
    return 0;
  }

  // Create and register struct symbol
  Symbol *struct_symbol = symbol_create(decl->name, SYMBOL_STRUCT, struct_type);
  if (!struct_symbol) {
    type_destroy(struct_type);
    free(field_types);
    return 0;
  }

  if (!symbol_table_declare(checker->symbol_table, struct_symbol)) {
    symbol_destroy(struct_symbol);
    free(field_types);
    return 0;
  }

  free(field_types);
  return 1;
}

int type_checker_process_declaration(TypeChecker *checker,
                                     ASTNode *declaration) {
  if (!checker || !declaration) {
    return 0;
  }

  switch (declaration->type) {
  case AST_VAR_DECLARATION: {
    VarDeclaration *var_decl = (VarDeclaration *)declaration->data;
    if (!var_decl || !var_decl->name) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid variable declaration");
      return 0;
    }

    if (var_decl->link_name && !var_decl->is_extern) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Link-name suffix is only allowed on extern declarations");
      return 0;
    }

    Scope *current_scope = symbol_table_get_current_scope(checker->symbol_table);
    if (var_decl->is_extern &&
        (!current_scope || current_scope->type != SCOPE_GLOBAL)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern declarations are only allowed at top level");
      return 0;
    }

    if (var_decl->is_extern && var_decl->initializer) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern variable '%s' cannot have an initializer", var_decl->name);
      return 0;
    }
    if (var_decl->is_extern && !var_decl->type_name) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern variable '%s' requires an explicit type annotation",
          var_decl->name);
      return 0;
    }

    Type *var_type = NULL;

    // If type is explicitly specified, resolve it
    if (var_decl->type_name) {
      var_type = type_checker_get_type_by_name(checker, var_decl->type_name);
      if (!var_type) {
        type_checker_report_undefined_symbol(checker, declaration->location,
                                             var_decl->type_name, "type");
        return 0;
      }
    }

    // If there's an initializer, validate it
    if (var_decl->initializer) {
      Type *init_type = type_checker_infer_type(checker, var_decl->initializer);
      if (!init_type) {
        if (!checker->has_error) {
          type_checker_set_error_at_location(
              checker, var_decl->initializer->location,
              "Cannot infer type of initializer for variable '%s'",
              var_decl->name);
        }
        return 0;
      }
      if (var_type) {
        // Type specified: validate assignment compatibility
        if (!(var_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(var_decl->initializer)) &&
            !type_checker_is_assignable(checker, var_type, init_type)) {
          type_checker_report_type_mismatch(checker,
                                            var_decl->initializer->location,
                                            var_type->name, init_type->name);
          return 0;
        }
      } else {
        // Type inference: use initializer type
        var_type = init_type;
      }
    } else if (!var_type) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Variable '%s' must have either a type annotation or an initializer",
          var_decl->name);
      return 0;
    }

    // Check for duplicate declaration in current scope.
    Symbol *existing = symbol_table_lookup_current_scope(checker->symbol_table,
                                                         var_decl->name);
    if (existing) {
      if (existing->kind != SYMBOL_VARIABLE) {
        type_checker_report_duplicate_declaration(checker, declaration->location,
                                                  var_decl->name);
        return 0;
      }
      if (existing->is_extern != var_decl->is_extern) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Variable '%s' redeclared with conflicting extern/non-extern "
            "linkage",
            var_decl->name);
        return 0;
      }
      if (!var_decl->is_extern) {
        type_checker_report_duplicate_declaration(checker, declaration->location,
                                                  var_decl->name);
        return 0;
      }
      if (!type_checker_types_equal(existing->type, var_type)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Extern variable '%s' redeclared with conflicting type",
            var_decl->name);
        return 0;
      }
      if (!type_checker_link_name_matches_symbol(existing, var_decl->name,
                                                 var_decl->is_extern,
                                                 var_decl->link_name)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Extern variable '%s' redeclared with conflicting link name",
            var_decl->name);
        return 0;
      }
      return 1;
    }

    // Create and declare the symbol
    Symbol *var_symbol =
        symbol_create(var_decl->name, SYMBOL_VARIABLE, var_type);
    if (!var_symbol) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Failed to create symbol for variable '%s'", var_decl->name);
      return 0;
    }

    var_symbol->is_extern = var_decl->is_extern;
    if (var_decl->is_extern) {
      const char *effective_link_name = type_checker_decl_link_name(
          var_decl->name, var_decl->is_extern, var_decl->link_name);
      var_symbol->link_name = effective_link_name ? strdup(effective_link_name)
                                                  : NULL;
      if (!var_symbol->link_name) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Failed to allocate link name for extern variable '%s'",
            var_decl->name);
        symbol_destroy(var_symbol);
        return 0;
      }
    }

    if (!symbol_table_declare(checker->symbol_table, var_symbol)) {
      type_checker_report_duplicate_declaration(checker, declaration->location,
                                                var_decl->name);
      symbol_destroy(var_symbol);
      return 0;
    }

    return 1;
  }

  case AST_FUNCTION_DECLARATION: {
    FunctionDeclaration *func_decl = (FunctionDeclaration *)declaration->data;
    if (!func_decl || !func_decl->name) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid function declaration");
      return 0;
    }

    if (func_decl->link_name && !func_decl->is_extern) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Link-name suffix is only allowed on extern declarations");
      return 0;
    }

    Scope *current_scope = symbol_table_get_current_scope(checker->symbol_table);
    if (func_decl->is_extern &&
        (!current_scope || current_scope->type != SCOPE_GLOBAL)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern declarations are only allowed at top level");
      return 0;
    }
    if (func_decl->is_extern && func_decl->body) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Extern function '%s' must not have a body", func_decl->name);
      return 0;
    }

    // Resolve return type
    Type *return_type = NULL;
    if (func_decl->return_type) {
      return_type =
          type_checker_get_type_by_name(checker, func_decl->return_type);
      if (!return_type) {
        type_checker_report_undefined_symbol(checker, declaration->location,
                                             func_decl->return_type, "type");
        return 0;
      }
    } else {
      return_type = checker->builtin_void;
    }

    // Resolve parameter types and check for duplicate parameter names
    Type **param_types = NULL;
    if (func_decl->parameter_count > 0) {
      param_types = malloc(func_decl->parameter_count * sizeof(Type *));
      if (!param_types) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Memory allocation failed for function parameters");
        return 0;
      }

      // Check for duplicate parameter names
      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        for (size_t j = i + 1; j < func_decl->parameter_count; j++) {
          if (strcmp(func_decl->parameter_names[i],
                     func_decl->parameter_names[j]) == 0) {
            type_checker_report_duplicate_declaration(
                checker, declaration->location, func_decl->parameter_names[i]);
            free(param_types);
            return 0;
          }
        }
      }

      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        param_types[i] = type_checker_get_type_by_name(
            checker, func_decl->parameter_types[i]);
        if (!param_types[i]) {
          type_checker_report_undefined_symbol(checker, declaration->location,
                                               func_decl->parameter_types[i],
                                               "type");
          free(param_types);
          return 0;
        }
      }
    }

    // Copy parameter names so function symbols own their metadata.
    char **param_names_copy = NULL;
    if (func_decl->parameter_count > 0) {
      param_names_copy = malloc(func_decl->parameter_count * sizeof(char *));
      if (!param_names_copy) {
        if (param_types)
          free(param_types);
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Memory allocation failed for function parameter names");
        return 0;
      }
      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        param_names_copy[i] = strdup(func_decl->parameter_names[i]);
        if (!param_names_copy[i]) {
          for (size_t j = 0; j < i; j++) {
            free(param_names_copy[j]);
          }
          free(param_names_copy);
          if (param_types)
            free(param_types);
          type_checker_set_error_at_location(
              checker, declaration->location,
              "Memory allocation failed for parameter name copy");
          return 0;
        }
      }
    }

    // Create function symbol
    Symbol *func_symbol =
        symbol_create(func_decl->name, SYMBOL_FUNCTION, return_type);
    if (!func_symbol) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Memory allocation failed for function symbol");
      if (param_names_copy) {
        for (size_t i = 0; i < func_decl->parameter_count; i++) {
          free(param_names_copy[i]);
        }
        free(param_names_copy);
      }
      if (param_types)
        free(param_types);
      return 0;
    }

    // Set function-specific data
    func_symbol->data.function.parameter_count = func_decl->parameter_count;
    func_symbol->data.function.parameter_names = param_names_copy;
    func_symbol->data.function.parameter_types = param_types;
    func_symbol->data.function.return_type = return_type;
    func_symbol->is_extern = func_decl->is_extern;
    if (func_decl->is_extern) {
      const char *effective_link_name = type_checker_decl_link_name(
          func_decl->name, func_decl->is_extern, func_decl->link_name);
      func_symbol->link_name = effective_link_name ? strdup(effective_link_name)
                                                   : NULL;
      if (!func_symbol->link_name) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Failed to allocate link name for extern function '%s'",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
    }

    Symbol *existing_before = symbol_table_lookup_current_scope(
        checker->symbol_table, func_decl->name);
    int is_resolving_forward =
        (existing_before && existing_before->kind == SYMBOL_FUNCTION &&
         existing_before->is_forward_declaration);

    if (existing_before && existing_before->kind != SYMBOL_FUNCTION) {
      type_checker_report_duplicate_declaration(checker, declaration->location,
                                                func_decl->name);
      symbol_destroy(func_symbol);
      return 0;
    }

    if (existing_before && existing_before->kind == SYMBOL_FUNCTION) {
      if (existing_before->is_extern != func_decl->is_extern) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Function '%s' redeclared with conflicting extern/non-extern "
            "linkage",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
      if ((existing_before->is_extern || func_decl->is_extern) &&
          !type_checker_link_name_matches_symbol(
              existing_before, func_decl->name, func_decl->is_extern,
              func_decl->link_name)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Function '%s' redeclared with conflicting link name",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
    }

    // Forward declaration: no body
    if (!func_decl->body) {
      func_symbol->is_initialized = 0;
      if (!symbol_table_declare_forward(checker->symbol_table, func_symbol)) {
        type_checker_set_error_at_location(
            checker, declaration->location,
            "Invalid or conflicting forward declaration for function '%s'",
            func_decl->name);
        symbol_destroy(func_symbol);
        return 0;
      }
      if (is_resolving_forward) {
        symbol_destroy(func_symbol);
      }
      return 1;
    }

    func_symbol->is_initialized = 1;
    if (!symbol_table_resolve_forward_declaration(checker->symbol_table,
                                                  func_symbol)) {
      type_checker_set_error_at_location(
          checker, declaration->location,
          "Function definition for '%s' does not match existing declaration",
          func_decl->name);
      symbol_destroy(func_symbol);
      return 0;
    }

    if (is_resolving_forward) {
      checker->current_function = existing_before;
      symbol_destroy(func_symbol); // not inserted, existing symbol was updated
      func_symbol = existing_before;
    } else {
      checker->current_function = func_symbol;
    }

    // Enter a new scope for the function body
    symbol_table_enter_scope(checker->symbol_table, SCOPE_FUNCTION);

    // Add parameters to the new scope
    Type **active_param_types =
        checker->current_function->data.function.parameter_types;
    if (func_decl->parameter_count > 0) {
      for (size_t i = 0; i < func_decl->parameter_count; i++) {
        Symbol *param_symbol =
            symbol_create(func_decl->parameter_names[i], SYMBOL_VARIABLE,
                          active_param_types ? active_param_types[i] : NULL);
        if (!param_symbol) {
          type_checker_set_error_at_location(
              checker, declaration->location,
              "Failed to create parameter symbol");
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
        if (!symbol_table_declare(checker->symbol_table, param_symbol)) {
          type_checker_report_duplicate_declaration(
              checker, declaration->location, func_decl->parameter_names[i]);
          symbol_destroy(param_symbol);
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
      }
    }

    // Process the function body
    if (func_decl->body) {
      for (size_t i = 0; i < func_decl->body->child_count; i++) {
        if (!type_checker_check_statement(checker,
                                          func_decl->body->children[i])) {
          // Error already reported
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
      }
    }

    // Exit the function's scope
    symbol_table_exit_scope(checker->symbol_table);

    // Reset the current function in the type checker
    checker->current_function = NULL;

    return 1;
  }

  case AST_METHOD_DECLARATION:
    // Method declarations are handled within struct processing
    // This case shouldn't normally be reached during standalone processing
    return 1;

  case AST_INLINE_ASM:
    // Top-level inline assembly is permitted.
    return 1;

  case AST_ASSIGNMENT: {
    Assignment *assignment = (Assignment *)declaration->data;
    if (!assignment || !assignment->value) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid assignment statement");
      return 0;
    }

    // Complex assignment target: obj.field = value or arr[i] = value
    if (assignment->target) {
      if (assignment->target->type == AST_MEMBER_ACCESS) {
        MemberAccess *member = (MemberAccess *)assignment->target->data;
        if (!member || !member->object || !member->member) {
          type_checker_set_error_at_location(checker,
                                             assignment->target->location,
                                             "Invalid field assignment target");
          return 0;
        }

        Type *object_type = type_checker_infer_type(checker, member->object);
        if (!object_type) {
          return 0;
        }

        if (object_type->kind != TYPE_STRUCT) {
          char error_msg[512];
          snprintf(error_msg, sizeof(error_msg),
                   "Cannot assign field '%s' on non-struct type '%s'",
                   member->member, object_type->name);
          type_checker_set_error_at_location(
              checker, assignment->target->location, error_msg);
          return 0;
        }

        Type *field_type = type_get_field_type(object_type, member->member);
        if (!field_type) {
          char error_msg[512];
          snprintf(error_msg, sizeof(error_msg),
                   "Field '%s' not found in struct '%s'", member->member,
                   object_type->name);
          type_checker_set_error_at_location(
              checker, assignment->target->location, error_msg);
          return 0;
        }

        Type *value_type = type_checker_infer_type(checker, assignment->value);
        if (!value_type) {
          if (!checker->has_error) {
            type_checker_set_error_at_location(
                checker, assignment->value->location,
                "Cannot infer type of assignment value");
          }
          return 0;
        }

        if (!(field_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(assignment->value)) &&
            !type_checker_is_assignable(checker, field_type, value_type)) {
          type_checker_report_type_mismatch(checker,
                                            assignment->value->location,
                                            field_type->name, value_type->name);
          return 0;
        }

        return 1;
      } else if (assignment->target->type == AST_INDEX_EXPRESSION) {
        Type *element_type =
            type_checker_infer_type(checker, assignment->target);
        if (!element_type) {
          return 0;
        }

        Type *value_type = type_checker_infer_type(checker, assignment->value);
        if (!value_type) {
          if (!checker->has_error) {
            type_checker_set_error_at_location(
                checker, assignment->value->location,
                "Cannot infer type of assignment value");
          }
          return 0;
        }

        if (!(element_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(assignment->value)) &&
            !type_checker_is_assignable(checker, element_type, value_type)) {
          type_checker_report_type_mismatch(
              checker, assignment->value->location, element_type->name,
              value_type->name);
          return 0;
        }

        return 1;
      } else if (assignment->target->type == AST_UNARY_EXPRESSION) {
        UnaryExpression *target_unary =
            (UnaryExpression *)assignment->target->data;
        if (!target_unary || !target_unary->operator ||
            strcmp(target_unary->operator, "*") != 0) {
          type_checker_set_error_at_location(checker,
                                             assignment->target->location,
                                             "Invalid assignment target");
          return 0;
        }

        Type *target_type =
            type_checker_infer_type(checker, assignment->target);
        if (!target_type) {
          return 0;
        }

        Type *value_type = type_checker_infer_type(checker, assignment->value);
        if (!value_type) {
          if (!checker->has_error) {
            type_checker_set_error_at_location(
                checker, assignment->value->location,
                "Cannot infer type of assignment value");
          }
          return 0;
        }

        if (!(target_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(assignment->value)) &&
            !type_checker_is_assignable(checker, target_type, value_type)) {
          type_checker_report_type_mismatch(
              checker, assignment->value->location, target_type->name,
              value_type->name);
          return 0;
        }

        return 1;
      }

      type_checker_set_error_at_location(checker, assignment->target->location,
                                         "Invalid assignment target");
      return 0;
    }

    // Simple variable assignment: name = value
    if (!assignment->variable_name) {
      type_checker_set_error_at_location(checker, declaration->location,
                                         "Invalid assignment statement");
      return 0;
    }

    // Look up the variable
    Symbol *var_symbol =
        symbol_table_lookup(checker->symbol_table, assignment->variable_name);
    if (!var_symbol) {
      type_checker_report_undefined_symbol(checker, declaration->location,
                                           assignment->variable_name,
                                           "variable");
      return 0;
    }

    if (var_symbol->kind != SYMBOL_VARIABLE &&
        var_symbol->kind != SYMBOL_PARAMETER) {
      char error_msg[512];
      const char *symbol_type =
          (var_symbol->kind == SYMBOL_FUNCTION) ? "function"
          : (var_symbol->kind == SYMBOL_STRUCT) ? "struct"
                                                : "symbol";
      snprintf(error_msg, sizeof(error_msg),
               "'%s' is a %s and cannot be assigned to",
               assignment->variable_name, symbol_type);
      type_checker_set_error_at_location(checker, declaration->location,
                                         error_msg);
      return 0;
    }

    // Infer the type of the assignment value
    Type *value_type = type_checker_infer_type(checker, assignment->value);
    if (!value_type) {
      if (!checker->has_error) {
        type_checker_set_error_at_location(
            checker, assignment->value->location,
            "Cannot infer type of assignment value");
      }
      return 0;
    }

    // Validate assignment compatibility
    if (!(var_symbol->type->kind == TYPE_POINTER &&
          type_checker_is_null_pointer_constant(assignment->value)) &&
        !type_checker_is_assignable(checker, var_symbol->type, value_type)) {
      type_checker_report_type_mismatch(checker, assignment->value->location,
                                        var_symbol->type->name,
                                        value_type->name);
      return 0;
    }

    return 1;
  }

  default:
    type_checker_set_error_at_location(
        checker, declaration->location,
        "Unsupported top-level construct in declaration context");
    return 0;
  }
}

// Enhanced error reporting functions

void type_checker_set_error_at_location(TypeChecker *checker,
                                        SourceLocation location,
                                        const char *format, ...) {
  if (!checker || !format)
    return;

  checker->has_error = 1;
  free(checker->error_message);

  va_list args;
  va_start(args, format);

  // Calculate required buffer size
  va_list args_copy;
  va_copy(args_copy, args);
  int size = vsnprintf(NULL, 0, format, args_copy);
  va_end(args_copy);

  if (size > 0) {
    checker->error_message = malloc(size + 1);
    if (checker->error_message) {
      vsnprintf(checker->error_message, size + 1, format, args);
    }
  }

  // If we have an error reporter, add the error to it
  if (checker->error_reporter) {
    char *message = checker->error_message;
    error_reporter_add_error(checker->error_reporter, ERROR_SEMANTIC, location,
                             message);
  }

  va_end(args);
}

void type_checker_report_type_mismatch(TypeChecker *checker,
                                       SourceLocation location,
                                       const char *expected,
                                       const char *actual) {
  if (!checker || !expected || !actual)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg),
           "Type mismatch: expected '%s', found '%s'", expected, actual);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    const char *suggestion =
        error_reporter_suggest_for_type_mismatch(expected, actual);
    if (suggestion) {
      error_reporter_add_error_with_suggestion(
          checker->error_reporter, ERROR_TYPE, location, error_msg, suggestion);
    } else {
      error_reporter_add_error(checker->error_reporter, ERROR_TYPE, location,
                               error_msg);
    }
  }
}

void type_checker_report_undefined_symbol(TypeChecker *checker,
                                          SourceLocation location,
                                          const char *symbol_name,
                                          const char *symbol_type) {
  if (!checker || !symbol_name || !symbol_type)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Undefined %s '%s'", symbol_type,
           symbol_name);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    char suggestion[256];
    snprintf(suggestion, sizeof(suggestion), "declare '%s' before using it",
             symbol_name);
    error_reporter_add_error_with_suggestion(checker->error_reporter,
                                             ERROR_SEMANTIC, location,
                                             error_msg, suggestion);
  }
}

void type_checker_report_duplicate_declaration(TypeChecker *checker,
                                               SourceLocation location,
                                               const char *symbol_name) {
  if (!checker || !symbol_name)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Duplicate declaration of '%s'",
           symbol_name);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    char suggestion[256];
    snprintf(suggestion, sizeof(suggestion),
             "use a different name or remove the duplicate declaration");
    error_reporter_add_error_with_suggestion(checker->error_reporter,
                                             ERROR_SEMANTIC, location,
                                             error_msg, suggestion);
  }
}

void type_checker_report_scope_violation(TypeChecker *checker,
                                         SourceLocation location,
                                         const char *symbol_name,
                                         const char *violation_type) {
  if (!checker || !symbol_name || !violation_type)
    return;

  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Scope violation: %s '%s'",
           violation_type, symbol_name);

  checker->has_error = 1;
  free(checker->error_message);
  checker->error_message = strdup(error_msg);

  if (checker->error_reporter) {
    char suggestion[256];
    if (strcmp(violation_type,
               "cannot access local variable from outer scope") == 0) {
      snprintf(suggestion, sizeof(suggestion),
               "declare '%s' in the current scope or pass it as a parameter",
               symbol_name);
    } else if (strcmp(violation_type, "cannot redeclare parameter") == 0) {
      snprintf(suggestion, sizeof(suggestion), "use a different variable name");
    } else {
      snprintf(suggestion, sizeof(suggestion), "check the scope rules for '%s'",
               symbol_name);
    }
    error_reporter_add_error_with_suggestion(
        checker->error_reporter, ERROR_SCOPE, location, error_msg, suggestion);
  }
}

// Validation functions for semantic analysis

// Statement and expression validation functions

int type_checker_check_statement(TypeChecker *checker, ASTNode *statement) {
  if (!checker || !statement)
    return 0;

  switch (statement->type) {
  case AST_VAR_DECLARATION:
  case AST_FUNCTION_DECLARATION:
  case AST_STRUCT_DECLARATION:
  case AST_ASSIGNMENT:
    // These are handled by process_declaration
    return type_checker_process_declaration(checker, statement);

  case AST_FUNCTION_CALL: {
    // Function call as statement (no return value used)
    Type *return_type = type_checker_infer_type(checker, statement);
    return return_type != NULL; // Error already reported if NULL
  }

  case AST_RETURN_STATEMENT: {
    ReturnStatement *ret_stmt = (ReturnStatement *)statement->data;
    if (ret_stmt && ret_stmt->value) {
      // Check if return value type matches function return type
      Type *value_type = type_checker_infer_type(checker, ret_stmt->value);
      if (!value_type) {
        // Error already reported by type_checker_infer_type if it failed
        // Only set generic error if no specific error was set
        if (!checker->has_error) {
          type_checker_set_error_at_location(
              checker, ret_stmt->value->location,
              "Cannot infer type of return value");
        }
        return 0;
      }

      if (checker->current_function) {
        Type *func_return_type =
            checker->current_function->data.function.return_type;
        if (!(func_return_type->kind == TYPE_POINTER &&
              type_checker_is_null_pointer_constant(ret_stmt->value)) &&
            !type_checker_is_assignable(checker, func_return_type,
                                        value_type)) {
          type_checker_report_type_mismatch(checker, ret_stmt->value->location,
                                            func_return_type->name,
                                            value_type->name);
          return 0;
        }
      } else {
        type_checker_set_error_at_location(
            checker, statement->location,
            "Return statement outside of a function");
        return 0;
      }
    }
    return 1;
  }

  case AST_IF_STATEMENT: {
    IfStatement *if_stmt = (IfStatement *)statement->data;
    if (!if_stmt || !if_stmt->condition) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid if statement");
      return 0;
    }

    // Check condition type
    Type *condition_type = type_checker_infer_type(checker, if_stmt->condition);
    if (!condition_type) {
      return 0; // Error already reported
    }

    // Condition should be a numeric type (treated as boolean)
    if (!type_checker_is_numeric_type(condition_type)) {
      type_checker_report_type_mismatch(checker, if_stmt->condition->location,
                                        "numeric type", condition_type->name);
      return 0;
    }

    // Check then branch
    if (if_stmt->then_branch &&
        !type_checker_check_statement(checker, if_stmt->then_branch)) {
      return 0;
    }

    // Check else branch if present
    if (if_stmt->else_branch &&
        !type_checker_check_statement(checker, if_stmt->else_branch)) {
      return 0;
    }

    return 1;
  }

  case AST_WHILE_STATEMENT: {
    WhileStatement *while_stmt = (WhileStatement *)statement->data;
    if (!while_stmt || !while_stmt->condition) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid while statement");
      return 0;
    }

    // Check condition type
    Type *condition_type =
        type_checker_infer_type(checker, while_stmt->condition);
    if (!condition_type) {
      return 0; // Error already reported
    }

    // Condition should be a numeric type (treated as boolean)
    if (!type_checker_is_numeric_type(condition_type)) {
      type_checker_report_type_mismatch(checker,
                                        while_stmt->condition->location,
                                        "numeric type", condition_type->name);
      return 0;
    }

    checker->loop_depth++;
    if (while_stmt->body &&
        !type_checker_check_statement(checker, while_stmt->body)) {
      checker->loop_depth--;
      return 0;
    }
    checker->loop_depth--;

    return 1;
  }

  case AST_FOR_STATEMENT: {
    ForStatement *for_stmt = (ForStatement *)statement->data;
    if (!for_stmt) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid for statement");
      return 0;
    }

    symbol_table_enter_scope(checker->symbol_table, SCOPE_BLOCK);

    if (for_stmt->initializer) {
      int init_ok = 0;
      if (for_stmt->initializer->type == AST_VAR_DECLARATION ||
          for_stmt->initializer->type == AST_ASSIGNMENT ||
          for_stmt->initializer->type == AST_FUNCTION_CALL) {
        init_ok = type_checker_check_statement(checker, for_stmt->initializer);
      } else {
        init_ok = type_checker_check_expression(checker, for_stmt->initializer);
      }
      if (!init_ok) {
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }
    }

    if (for_stmt->condition) {
      Type *cond_type = type_checker_infer_type(checker, for_stmt->condition);
      if (!cond_type) {
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }
      if (!type_checker_is_numeric_type(cond_type)) {
        type_checker_report_type_mismatch(checker,
                                          for_stmt->condition->location,
                                          "numeric type", cond_type->name);
        symbol_table_exit_scope(checker->symbol_table);
        return 0;
      }
    }

    if (for_stmt->increment &&
        !type_checker_check_expression(checker, for_stmt->increment)) {
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }

    checker->loop_depth++;
    if (for_stmt->body &&
        !type_checker_check_statement(checker, for_stmt->body)) {
      checker->loop_depth--;
      symbol_table_exit_scope(checker->symbol_table);
      return 0;
    }
    checker->loop_depth--;

    symbol_table_exit_scope(checker->symbol_table);
    return 1;
  }

  case AST_SWITCH_STATEMENT: {
    SwitchStatement *switch_stmt = (SwitchStatement *)statement->data;
    if (!switch_stmt || !switch_stmt->expression) {
      type_checker_set_error_at_location(checker, statement->location,
                                         "Invalid switch statement");
      return 0;
    }

    Type *switch_type =
        type_checker_infer_type(checker, switch_stmt->expression);
    if (!switch_type) {
      return 0;
    }
    if (!type_checker_is_integer_type(switch_type)) {
      type_checker_report_type_mismatch(checker,
                                        switch_stmt->expression->location,
                                        "integer type", switch_type->name);
      return 0;
    }

    long long *case_values = NULL;
    size_t case_value_count = 0;
    int seen_default = 0;

    if (switch_stmt->case_count > 0) {
      case_values = malloc(switch_stmt->case_count * sizeof(long long));
      if (!case_values) {
        type_checker_set_error_at_location(
            checker, statement->location,
            "Memory allocation failed in switch validation");
        return 0;
      }
    }

    checker->switch_depth++;
    for (size_t i = 0; i < switch_stmt->case_count; i++) {
      ASTNode *case_node = switch_stmt->cases ? switch_stmt->cases[i] : NULL;
      if (!case_node || case_node->type != AST_CASE_CLAUSE) {
        type_checker_set_error_at_location(checker, statement->location,
                                           "Invalid case clause in switch");
        checker->switch_depth--;
        free(case_values);
        return 0;
      }

      CaseClause *case_clause = (CaseClause *)case_node->data;
      if (!case_clause) {
        type_checker_set_error_at_location(checker, case_node->location,
                                           "Invalid case clause");
        checker->switch_depth--;
        free(case_values);
        return 0;
      }

      if (case_clause->is_default) {
        if (seen_default) {
          type_checker_set_error_at_location(
              checker, case_node->location,
              "Switch may only contain one default clause");
          checker->switch_depth--;
          free(case_values);
          return 0;
        }
        seen_default = 1;
      } else {
        if (!case_clause->value) {
          type_checker_set_error_at_location(
              checker, case_node->location,
              "Case clause is missing a value expression");
          checker->switch_depth--;
          free(case_values);
          return 0;
        }

        Type *case_type = type_checker_infer_type(checker, case_clause->value);
        if (!case_type) {
          checker->switch_depth--;
          free(case_values);
          return 0;
        }
        if (!type_checker_is_integer_type(case_type)) {
          type_checker_report_type_mismatch(checker,
                                            case_clause->value->location,
                                            "integer type", case_type->name);
          checker->switch_depth--;
          free(case_values);
          return 0;
        }
        if (!type_checker_is_assignable(checker, switch_type, case_type)) {
          type_checker_report_type_mismatch(checker,
                                            case_clause->value->location,
                                            switch_type->name, case_type->name);
          checker->switch_depth--;
          free(case_values);
          return 0;
        }

        long long case_value = 0;
        if (!type_checker_eval_integer_constant(case_clause->value,
                                                &case_value)) {
          type_checker_set_error_at_location(
              checker, case_clause->value->location,
              "Case value must be a compile-time integer constant expression");
          checker->switch_depth--;
          free(case_values);
          return 0;
        }

        for (size_t j = 0; j < case_value_count; j++) {
          if (case_values[j] == case_value) {
            type_checker_set_error_at_location(
                checker, case_clause->value->location,
                "Duplicate case value '%lld' in switch", case_value);
            checker->switch_depth--;
            free(case_values);
            return 0;
          }
        }
        case_values[case_value_count++] = case_value;
      }

      if (!case_clause->body) {
        type_checker_set_error_at_location(checker, case_node->location,
                                           "Case clause must have a body");
        checker->switch_depth--;
        free(case_values);
        return 0;
      }

      if (!type_checker_check_statement(checker, case_clause->body)) {
        checker->switch_depth--;
        free(case_values);
        return 0;
      }
    }
    checker->switch_depth--;
    free(case_values);
    return 1;
  }

  case AST_BREAK_STATEMENT:
    if (checker->loop_depth <= 0 && checker->switch_depth <= 0) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "'break' can only be used inside a loop or switch");
      return 0;
    }
    return 1;

  case AST_CONTINUE_STATEMENT:
    if (checker->loop_depth <= 0) {
      type_checker_set_error_at_location(
          checker, statement->location,
          "'continue' can only be used inside a loop");
      return 0;
    }
    return 1;

  case AST_INLINE_ASM:
    // Inline assembly is passed through without type checking
    return 1;
    break;

  case AST_PROGRAM: {
    // A block of statements
    Program *block = (Program *)statement->data;
    if (block) {
      // Enter a new nested scope
      symbol_table_enter_scope(checker->symbol_table, SCOPE_BLOCK);

      for (size_t i = 0; i < statement->child_count; i++) {
        if (!type_checker_check_statement(checker, statement->children[i])) {
          symbol_table_exit_scope(checker->symbol_table);
          return 0;
        }
      }

      symbol_table_exit_scope(checker->symbol_table);
    }
    return 1;
  }
  default:
    // Unknown statement type
    type_checker_set_error_at_location(checker, statement->location,
                                       "Unknown statement type");
    return 0;
  }
}

int type_checker_check_expression(TypeChecker *checker, ASTNode *expression) {
  if (!checker || !expression)
    return 0;

  // Use type inference to validate the expression
  Type *expr_type = type_checker_infer_type(checker, expression);
  return expr_type != NULL; // Error already reported if NULL
}

// Enhanced binary expression type checking
Type *type_checker_check_binary_expression(TypeChecker *checker,
                                           BinaryExpression *binop,
                                           SourceLocation location) {
  if (!checker || !binop)
    return NULL;

  Type *left_type = type_checker_infer_type(checker, binop->left);
  Type *right_type = type_checker_infer_type(checker, binop->right);

  if (!left_type || !right_type) {
    return NULL; // Error already reported
  }

  const char *op = binop->operator;

  // Arithmetic operators require numeric types
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0 ||
      strcmp(op, "/") == 0 || strcmp(op, "%") == 0) {

    if (!type_checker_is_numeric_type(left_type)) {
      type_checker_report_type_mismatch(checker, binop->left->location,
                                        "numeric type", left_type->name);
      return NULL;
    }

    if (!type_checker_is_numeric_type(right_type)) {
      type_checker_report_type_mismatch(checker, binop->right->location,
                                        "numeric type", right_type->name);
      return NULL;
    }

    // Modulo operator requires integer types
    if (strcmp(op, "%") == 0) {
      if (!type_checker_is_integer_type(left_type)) {
        type_checker_report_type_mismatch(checker, binop->left->location,
                                          "integer type", left_type->name);
        return NULL;
      }

      if (!type_checker_is_integer_type(right_type)) {
        type_checker_report_type_mismatch(checker, binop->right->location,
                                          "integer type", right_type->name);
        return NULL;
      }
    }

    return type_checker_promote_types(checker, left_type, right_type, op);
  }

  // Comparison operators
  if (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0 || strcmp(op, "<") == 0 ||
      strcmp(op, "<=") == 0 || strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
    int is_equality = (strcmp(op, "==") == 0 || strcmp(op, "!=") == 0);
    int left_is_pointer = left_type->kind == TYPE_POINTER;
    int right_is_pointer = right_type->kind == TYPE_POINTER;

    if (left_is_pointer || right_is_pointer) {
      if (!is_equality) {
        type_checker_set_error_at_location(
            checker, location,
            "Pointer ordering comparisons are not supported");
        return NULL;
      }

      int left_is_null = type_checker_is_null_pointer_constant(binop->left);
      int right_is_null = type_checker_is_null_pointer_constant(binop->right);
      int comparable = (left_is_pointer && right_is_pointer &&
                        type_checker_types_equal(left_type, right_type)) ||
                       (left_is_pointer && right_is_null) ||
                       (right_is_pointer && left_is_null);

      if (!comparable) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), "Cannot compare '%s' with '%s'",
                 left_type->name, right_type->name);
        type_checker_set_error_at_location(checker, location, error_msg);
        return NULL;
      }

      return checker->builtin_int32;
    }

    // Both operands should be comparable (same type or compatible)
    if (!type_checker_are_compatible(left_type, right_type)) {
      char error_msg[512];
      snprintf(error_msg, sizeof(error_msg), "Cannot compare '%s' with '%s'",
               left_type->name, right_type->name);
      type_checker_set_error_at_location(checker, location, error_msg);
      return NULL;
    }

    return checker->builtin_int32; // Comparison result is boolean (int32)
  }

  // Logical operators
  if (strcmp(op, "&&") == 0 || strcmp(op, "||") == 0) {
    // Both operands should be numeric (treated as boolean)
    if (!type_checker_is_numeric_type(left_type)) {
      type_checker_report_type_mismatch(checker, binop->left->location,
                                        "numeric type", left_type->name);
      return NULL;
    }

    if (!type_checker_is_numeric_type(right_type)) {
      type_checker_report_type_mismatch(checker, binop->right->location,
                                        "numeric type", right_type->name);
      return NULL;
    }

    return checker->builtin_int32; // Logical result is boolean (int32)
  }

  // Unknown operator
  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Unknown binary operator '%s'", op);
  type_checker_set_error_at_location(checker, location, error_msg);
  return NULL;
}
