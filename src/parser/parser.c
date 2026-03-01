#include "parser.h"
#include "../error/error_reporter.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void parser_report_lexer_token_error(Parser *parser,
                                            const Token *token) {
  if (!parser || !token || token->type != TOKEN_ERROR) {
    return;
  }

  const char *message = (token->value && token->value[0] != '\0')
                            ? token->value
                            : "Invalid token";
  char error_msg[512];
  snprintf(error_msg, sizeof(error_msg), "Lexical error: %s", message);

  parser->has_error = 1;
  parser->had_error = 1;
  parser->error_count++;
  free(parser->error_message);
  parser->error_message = strdup(error_msg);

  if (parser->error_reporter) {
    SourceLocation location =
        source_location_create(token->line, token->column);
    error_reporter_add_error(parser->error_reporter, ERROR_LEXICAL, location,
                             error_msg);
  }
}

Parser *parser_create(Lexer *lexer) {
  return parser_create_with_error_reporter(lexer, NULL);
}

Parser *parser_create_with_error_reporter(Lexer *lexer,
                                          ErrorReporter *error_reporter) {
  if (!lexer) {
    return NULL;
  }

  Parser *parser = malloc(sizeof(Parser));
  if (!parser)
    return NULL;

  parser->lexer = lexer;
  parser->current_token = lexer_next_token(lexer);
  parser->peek_token = lexer_next_token(lexer);
  parser->has_error = 0;
  parser->had_error = 0;
  parser->error_count = 0;
  parser->error_message = NULL;
  parser->error_reporter = error_reporter;
  parser->error_recovery_mode = 0;

  if (parser->current_token.type == TOKEN_ERROR) {
    parser_report_lexer_token_error(parser, &parser->current_token);
  } else if (parser->peek_token.type == TOKEN_ERROR) {
    parser_report_lexer_token_error(parser, &parser->peek_token);
  }

  return parser;
}

void parser_destroy(Parser *parser) {
  if (parser) {
    token_destroy(&parser->current_token);
    token_destroy(&parser->peek_token);
    free(parser->error_message);
    free(parser);
  }
}

void parser_advance(Parser *parser) {
  if (!parser || parser->current_token.type == TOKEN_EOF)
    return;

  token_destroy(&parser->current_token);
  parser->current_token = parser->peek_token;

  // Clear peek_token to avoid double-free
  parser->peek_token.type = TOKEN_EOF;
  parser->peek_token.value = NULL;
  parser->peek_token.line = 0;
  parser->peek_token.column = 0;

  // Get new peek token
  parser->peek_token = lexer_next_token(parser->lexer);

  if (parser->current_token.type == TOKEN_ERROR) {
    parser_report_lexer_token_error(parser, &parser->current_token);
  }
}

int parser_match(Parser *parser, TokenType type) {
  if (!parser)
    return 0;
  return parser->current_token.type == type;
}

static const char *token_type_to_string(TokenType type) {
  switch (type) {
  case TOKEN_EOF:
    return "end of file";
  case TOKEN_IDENTIFIER:
    return "identifier";
  case TOKEN_NUMBER:
    return "number";
  case TOKEN_STRING:
    return "string";
  case TOKEN_IMPORT:
    return "'import'";
  case TOKEN_IMPORT_STR:
    return "'import_str'";
  case TOKEN_EXTERN:
    return "'extern'";
  case TOKEN_EXPORT:
    return "'export'";
  case TOKEN_VAR:
    return "'var'";
  case TOKEN_FUNCTION:
    return "'function'";
  case TOKEN_STRUCT:
    return "'struct'";
  case TOKEN_METHOD:
    return "'method'";
  case TOKEN_RETURN:
    return "'return'";
  case TOKEN_IF:
    return "'if'";
  case TOKEN_ELSE:
    return "'else'";
  case TOKEN_WHILE:
    return "'while'";
  case TOKEN_FOR:
    return "'for'";
  case TOKEN_SWITCH:
    return "'switch'";
  case TOKEN_CASE:
    return "'case'";
  case TOKEN_DEFAULT:
    return "'default'";
  case TOKEN_BREAK:
    return "'break'";
  case TOKEN_CONTINUE:
    return "'continue'";
  case TOKEN_DEFER:
    return "'defer'";
  case TOKEN_ERRDEFER:
    return "'errdefer'";
  case TOKEN_ASM:
    return "'asm'";
  case TOKEN_THIS:
    return "'this'";
  case TOKEN_NEW:
    return "'new'";
  case TOKEN_COLON:
    return "':'";
  case TOKEN_SEMICOLON:
    return "';'";
  case TOKEN_COMMA:
    return "','";
  case TOKEN_EQUALS:
    return "'='";
  case TOKEN_ARROW:
    return "'->'";
  case TOKEN_LPAREN:
    return "'('";
  case TOKEN_RPAREN:
    return "')'";
  case TOKEN_LBRACE:
    return "'{'";
  case TOKEN_RBRACE:
    return "'}'";
  case TOKEN_LBRACKET:
    return "'['";
  case TOKEN_RBRACKET:
    return "']'";
  case TOKEN_PLUS:
    return "'+'";
  case TOKEN_MINUS:
    return "'-'";
  case TOKEN_MULTIPLY:
    return "'*'";
  case TOKEN_AMPERSAND:
    return "'&'";
  case TOKEN_PIPE:
    return "'|'";
  case TOKEN_CARET:
    return "'^'";
  case TOKEN_LSHIFT:
    return "'<<'";
  case TOKEN_RSHIFT:
    return "'>>'";
  case TOKEN_TILDE:
    return "'~'";
  case TOKEN_AND_AND:
    return "'&&'";
  case TOKEN_OR_OR:
    return "'||'";
  case TOKEN_DIVIDE:
    return "'/'";
  case TOKEN_PERCENT:
    return "'%'";
  case TOKEN_DOT:
    return "'.'";
  case TOKEN_NEWLINE:
    return "newline";
  case TOKEN_ERROR:
    return "lexical error";
  default:
    return "unknown token";
  }
}

int parser_expect_statement_end(Parser *parser) {
  if (parser_match(parser, TOKEN_SEMICOLON) ||
      parser_match(parser, TOKEN_NEWLINE)) {
    parser_advance(parser);
    return 1;
  }

  // Skip over any extra newlines
  while (parser_match(parser, TOKEN_NEWLINE)) {
    parser_advance(parser);
  }

  if (parser_match(parser, TOKEN_SEMICOLON)) {
    parser_advance(parser);
    return 1;
  }

  parser_set_error(parser,
                   "Expected ';' or newline at the end of the statement");
  return 0;
}

int parser_expect(Parser *parser, TokenType type) {
  if (!parser)
    return 0;

  if (parser->current_token.type == type) {
    parser_advance(parser);
    return 1;
  }

  // Create a helpful error message
  char error_msg[512];
  const char *expected_str = token_type_to_string(type);
  const char *actual_str = token_type_to_string(parser->current_token.type);

  snprintf(error_msg, sizeof(error_msg), "Expected %s, found %s", expected_str,
           actual_str);

  // Generate context-specific suggestions
  const char *suggestion = NULL;
  if (type == TOKEN_SEMICOLON) {
    suggestion = "add a semicolon ';' to end the statement";
  } else if (type == TOKEN_RPAREN &&
             parser->current_token.type == TOKEN_COMMA) {
    suggestion = "check if you have an extra comma in the parameter list";
  } else if (type == TOKEN_RBRACE && parser->current_token.type == TOKEN_EOF) {
    suggestion = "add a closing brace '}' to match the opening brace";
  } else if (type == TOKEN_COLON &&
             parser->current_token.type == TOKEN_EQUALS) {
    suggestion = "use ':' for type annotations, '=' for assignments";
  }

  parser_set_error_with_suggestion(parser, error_msg, suggestion);
  return 0;
}

void parser_set_error(Parser *parser, const char *message) {
  parser_set_error_with_suggestion(parser, message, NULL);
}

void parser_set_error_with_suggestion(Parser *parser, const char *message,
                                      const char *suggestion) {
  if (!parser || !message)
    return;

  parser->has_error = 1;
  parser->had_error = 1;
  parser->error_count++;
  free(parser->error_message);
  parser->error_message = strdup(message);

  // If we have an error reporter, add the error to it
  if (parser->error_reporter) {
    SourceLocation location = source_location_create(
        parser->current_token.line, parser->current_token.column);

    size_t span_len = 1;
    if (parser->current_token.value && parser->current_token.value[0] != '\0') {
      span_len = strlen(parser->current_token.value);
    }
    SourceSpan span = source_span_from_location(location, span_len);

    if (suggestion) {
      error_reporter_add_error_with_span_and_suggestion(
          parser->error_reporter, ERROR_SYNTAX, span, message, suggestion);
    } else {
      // Try to generate a helpful suggestion
      const char *auto_suggestion = NULL;
      if (parser->current_token.value) {
        auto_suggestion =
            error_reporter_suggest_for_token(parser->current_token.value);
      }

      if (auto_suggestion) {
        error_reporter_add_error_with_span_and_suggestion(
            parser->error_reporter, ERROR_SYNTAX, span, message,
            auto_suggestion);
      } else {
        error_reporter_add_error_with_span(parser->error_reporter, ERROR_SYNTAX,
                                           span, message);
      }
    }
  }
}

void parser_recover_from_error(Parser *parser) {
  if (!parser)
    return;

  parser->error_recovery_mode = 1;
  parser_synchronize(parser);

  // Clear error state to continue parsing
  parser->has_error = 0;
  free(parser->error_message);
  parser->error_message = NULL;
  parser->error_recovery_mode = 0;
}

void parser_synchronize(Parser *parser) {
  if (!parser)
    return;

  while (parser->current_token.type != TOKEN_EOF) {
    if (parser->current_token.type == TOKEN_SEMICOLON ||
        parser->current_token.type == TOKEN_NEWLINE) {
      parser_advance(parser);
      return;
    }

    switch (parser->peek_token.type) {
    case TOKEN_FUNCTION:
    case TOKEN_VAR:
    case TOKEN_STRUCT:
    case TOKEN_RETURN:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_FOR:
    case TOKEN_SWITCH:
    case TOKEN_BREAK:
    case TOKEN_CONTINUE:
    case TOKEN_RBRACE:
      return;
    default:
      break;
    }

    parser_advance(parser);
  }
}

int parser_get_operator_precedence(TokenType type) {
  switch (type) {
  case TOKEN_DOT:
    return 13; // Member access (highest precedence)
  case TOKEN_MULTIPLY:
  case TOKEN_DIVIDE:
  case TOKEN_PERCENT:
    return 11; // Multiplicative
  case TOKEN_PLUS:
  case TOKEN_MINUS:
    return 10; // Additive
  case TOKEN_LSHIFT:
  case TOKEN_RSHIFT:
    return 9; // Shift
  case TOKEN_LESS_THAN:
  case TOKEN_LESS_EQUALS:
  case TOKEN_GREATER_THAN:
  case TOKEN_GREATER_EQUALS:
    return 8; // Relational
  case TOKEN_EQUALS_EQUALS:
  case TOKEN_NOT_EQUALS:
    return 7; // Equality
  case TOKEN_AMPERSAND:
    return 6; // Bitwise AND
  case TOKEN_CARET:
    return 5; // Bitwise XOR
  case TOKEN_PIPE:
    return 4; // Bitwise OR
  case TOKEN_AND_AND:
    return 3; // Logical AND
  case TOKEN_OR_OR:
    return 2; // Logical OR
  default:
    return 0; // Not a binary operator
  }
}

int parser_is_binary_operator(TokenType type) {
  switch (type) {
  case TOKEN_PLUS:
  case TOKEN_MINUS:
  case TOKEN_MULTIPLY:
  case TOKEN_DIVIDE:
  case TOKEN_PERCENT:
  case TOKEN_EQUALS_EQUALS:
  case TOKEN_NOT_EQUALS:
  case TOKEN_LESS_THAN:
  case TOKEN_LESS_EQUALS:
  case TOKEN_GREATER_THAN:
  case TOKEN_GREATER_EQUALS:
  case TOKEN_AND_AND:
  case TOKEN_OR_OR:
  case TOKEN_AMPERSAND:
  case TOKEN_PIPE:
  case TOKEN_CARET:
  case TOKEN_LSHIFT:
  case TOKEN_RSHIFT:
  case TOKEN_DOT:
    return 1;
  default:
    return 0;
  }
}

int parser_is_unary_operator(TokenType type) {
  switch (type) {
  case TOKEN_MINUS:
  case TOKEN_PLUS:
  case TOKEN_MULTIPLY:
  case TOKEN_AMPERSAND:
  case TOKEN_TILDE:
  case TOKEN_NOT:
    return 1;
  default:
    return 0;
  }
}

ASTNode *parser_parse_program(Parser *parser) {
  if (!parser)
    return NULL;

  ASTNode *program = ast_create_program();
  if (!program)
    return NULL;

  Program *prog_data = (Program *)program->data;

  while (parser->current_token.type != TOKEN_EOF) {
    // Skip empty statements/newlines at the top level
    if (parser->current_token.type == TOKEN_NEWLINE ||
        parser->current_token.type == TOKEN_SEMICOLON) {
      parser_advance(parser);
      continue;
    }

    ASTNode *declaration = parser_parse_declaration(parser);

    if (declaration) {
      // Add to program's declarations array
      prog_data->declarations =
          realloc(prog_data->declarations,
                  (prog_data->declaration_count + 1) * sizeof(ASTNode *));
      if (prog_data->declarations) {
        prog_data->declarations[prog_data->declaration_count] = declaration;
        prog_data->declaration_count++;
        ast_add_child(program, declaration);
      }
    } else if (!parser->has_error) {
      parser_set_error(parser, "Failed to parse declaration");
      parser_advance(parser);
    }

    if (parser->has_error) {
      parser_recover_from_error(parser);
    }
  }

  return program;
}

static ASTNode *parser_parse_extern_var_declaration(Parser *parser);

ASTNode *parser_parse_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  switch (parser->current_token.type) {
  case TOKEN_IMPORT:
    return parser_parse_import_declaration(parser);
  case TOKEN_EXTERN: {
    parser_advance(parser); // consume 'extern'
    if (parser->current_token.type == TOKEN_FUNCTION) {
      ASTNode *decl = parser_parse_function_declaration(parser);
      if (decl && decl->data) {
        FunctionDeclaration *func_data = (FunctionDeclaration *)decl->data;
        if (func_data->body != NULL) {
          parser_set_error(parser, "Extern functions must not have a body");
          ast_destroy_node(decl);
          return NULL;
        }
        func_data->is_extern = 1;
      }
      return decl;
    }
    if (parser->current_token.type == TOKEN_VAR) {
      return parser_parse_extern_var_declaration(parser);
    }
    parser_set_error(parser, "Expected 'function' or 'var' after 'extern'");
    return NULL;
  }
  case TOKEN_EXPORT: {
    parser_advance(parser); // consume 'export'
    ASTNode *decl = NULL;
    if (parser->current_token.type == TOKEN_FUNCTION) {
      decl = parser_parse_function_declaration(parser);
      if (decl && decl->data) {
        ((FunctionDeclaration *)decl->data)->is_exported = 1;
      }
    } else if (parser->current_token.type == TOKEN_STRUCT) {
      decl = parser_parse_struct_declaration(parser);
      if (decl && decl->data) {
        ((StructDeclaration *)decl->data)->is_exported = 1;
      }
    } else if (parser->current_token.type == TOKEN_ENUM) {
      decl = parser_parse_enum_declaration(parser);
      if (decl && decl->data) {
        ((EnumDeclaration *)decl->data)->is_exported = 1;
      }
    } else if (parser->current_token.type == TOKEN_VAR) {
      decl = parser_parse_var_declaration(parser);
      if (decl && decl->data) {
        ((VarDeclaration *)decl->data)->is_exported = 1;
      }
    } else if (parser->current_token.type == TOKEN_EXTERN) {
      parser_advance(parser); // consume 'extern'
      if (parser->current_token.type == TOKEN_FUNCTION) {
        decl = parser_parse_function_declaration(parser);
        if (decl && decl->data) {
          FunctionDeclaration *func_data = (FunctionDeclaration *)decl->data;
          if (func_data->body != NULL) {
            parser_set_error(parser, "Extern functions must not have a body");
            ast_destroy_node(decl);
            return NULL;
          }
          func_data->is_extern = 1;
          func_data->is_exported = 1;
        }
      } else if (parser->current_token.type == TOKEN_VAR) {
        decl = parser_parse_extern_var_declaration(parser);
        if (decl && decl->data) {
          ((VarDeclaration *)decl->data)->is_exported = 1;
        }
      } else {
        parser_set_error(parser,
                         "Expected 'function' or 'var' after 'export extern'");
        return NULL;
      }
    } else {
      parser_set_error(parser, "Expected 'function', 'var', 'struct', 'enum', "
                               "or 'extern' after 'export'");
      return NULL;
    }
    return decl;
  }
  case TOKEN_VAR:
    return parser_parse_var_declaration(parser);
  case TOKEN_DEFER:
    return parser_parse_defer_statement(parser);
  case TOKEN_ERRDEFER:
    return parser_parse_errdefer_statement(parser);
  case TOKEN_FUNCTION:
    return parser_parse_function_declaration(parser);
  case TOKEN_STRUCT:
    return parser_parse_struct_declaration(parser);
  case TOKEN_ENUM:
    return parser_parse_enum_declaration(parser);
  default:
    // Try to parse as a statement instead
    return parser_parse_statement(parser);
  }
}

static int parser_is_assignment_target(ASTNode *target) {
  if (!target) {
    return 0;
  }

  if (target->type == AST_IDENTIFIER || target->type == AST_MEMBER_ACCESS ||
      target->type == AST_INDEX_EXPRESSION) {
    return 1;
  }

  if (target->type == AST_UNARY_EXPRESSION) {
    UnaryExpression *unary = (UnaryExpression *)target->data;
    return unary && unary->operator && strcmp(unary->operator, "*") == 0;
  }

  return 0;
}

static ASTNode *parser_parse_assignment_from_target(Parser *parser,
                                                    ASTNode *target) {
  if (!parser || !target) {
    return NULL;
  }

  if (!parser_is_assignment_target(target)) {
    parser_set_error(parser, "Invalid assignment target");
    ast_destroy_node(target);
    return NULL;
  }

  parser_advance(parser); // consume '='
  ASTNode *value = parser_parse_expression(parser);
  if (!value) {
    ast_destroy_node(target);
    return NULL;
  }

  if (target->type == AST_IDENTIFIER) {
    Identifier *id = (Identifier *)target->data;
    if (!id || !id->name) {
      ast_destroy_node(target);
      ast_destroy_node(value);
      parser_set_error(parser, "Invalid assignment target");
      return NULL;
    }

    ASTNode *assign = ast_create_assignment(id->name, value, target->location);
    ast_destroy_node(target);
    return assign;
  }

  return ast_create_field_assignment(target, value, target->location);
}

ASTNode *parser_parse_statement(Parser *parser) {
  if (!parser)
    return NULL;

  switch (parser->current_token.type) {
  case TOKEN_EXTERN:
    return parser_parse_declaration(parser);
  case TOKEN_VAR:
    return parser_parse_var_declaration(parser);
  case TOKEN_RETURN:
    return parser_parse_return_statement(parser);
  case TOKEN_IF:
    return parser_parse_if_statement(parser);
  case TOKEN_WHILE:
    return parser_parse_while_statement(parser);
  case TOKEN_FOR:
    return parser_parse_for_statement(parser);
  case TOKEN_SWITCH:
    return parser_parse_switch_statement(parser);
  case TOKEN_BREAK:
    return parser_parse_break_statement(parser);
  case TOKEN_CONTINUE:
    return parser_parse_continue_statement(parser);
  case TOKEN_DEFER:
    return parser_parse_defer_statement(parser);
  case TOKEN_ERRDEFER:
    return parser_parse_errdefer_statement(parser);
  case TOKEN_ASM:
    return parser_parse_inline_asm(parser);
  case TOKEN_LBRACE:
    return parser_parse_block(parser);
  default:
    break;
  }

  ASTNode *expr = parser_parse_expression(parser);
  if (!expr) {
    return NULL;
  }

  if (parser->current_token.type == TOKEN_EQUALS) {
    return parser_parse_assignment_from_target(parser, expr);
  }

  return expr;
}

ASTNode *parser_parse_defer_statement(Parser *parser) {
  if (!parser) {
    return NULL;
  }

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  if (!parser_expect(parser, TOKEN_DEFER)) {
    return NULL;
  }

  if (parser->current_token.type == TOKEN_SEMICOLON ||
      parser->current_token.type == TOKEN_NEWLINE) {
    parser_set_error(parser, "Expected statement after 'defer'");
    return NULL;
  }

  ASTNode *stmt = parser_parse_statement(parser);
  if (!stmt) {
    if (!parser->has_error) {
      parser_set_error(parser, "Expected statement after 'defer'");
    }
    return NULL;
  }

  parser_expect_statement_end(parser);
  return ast_create_defer_statement(stmt, location);
}

ASTNode *parser_parse_errdefer_statement(Parser *parser) {
  if (!parser) {
    return NULL;
  }

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  if (!parser_expect(parser, TOKEN_ERRDEFER)) {
    return NULL;
  }

  if (parser->current_token.type == TOKEN_SEMICOLON ||
      parser->current_token.type == TOKEN_NEWLINE) {
    parser_set_error(parser, "Expected statement after 'errdefer'");
    return NULL;
  }

  ASTNode *stmt = parser_parse_statement(parser);
  if (!stmt) {
    if (!parser->has_error) {
      parser_set_error(parser, "Expected statement after 'errdefer'");
    }
    return NULL;
  }

  parser_expect_statement_end(parser);
  return ast_create_errdefer_statement(stmt, location);
}

ASTNode *parser_parse_expression(Parser *parser) {
  if (!parser)
    return NULL;

  return parser_parse_binary_expression(parser, 0);
}

int parser_is_identifier_like(TokenType type) {
  // Check if token can be used as an identifier in expression context
  return type == TOKEN_IDENTIFIER ||
         // x86 mnemonics can be used as function names
         (type >= TOKEN_MOV && type <= TOKEN_SYSCALL) ||
         // x86 registers can be used as identifiers in high-level context
         (type >= TOKEN_EAX && type <= TOKEN_R15);
}

int parser_is_type_keyword(TokenType type) {
  // Check if token is a built-in type keyword
  return (type >= TOKEN_INT8 && type <= TOKEN_STRING_TYPE);
}

static char **parser_parse_type_param_list(Parser *parser, size_t *out_count) {
  *out_count = 0;
  if (parser->current_token.type != TOKEN_LESS_THAN)
    return NULL;
  parser_advance(parser); // consume '<'

  char **params = NULL;
  size_t count = 0;

  while (parser->current_token.type != TOKEN_GREATER_THAN &&
         parser->current_token.type != TOKEN_EOF) {
    if (!parser_is_identifier_like(parser->current_token.type)) {
      parser_set_error(parser, "Expected type parameter name");
      for (size_t i = 0; i < count; i++)
        free(params[i]);
      free(params);
      return NULL;
    }
    params = realloc(params, (count + 1) * sizeof(char *));
    params[count] = strdup(parser->current_token.value);
    count++;
    parser_advance(parser);

    if (parser->current_token.type == TOKEN_COMMA) {
      parser_advance(parser);
    } else if (parser->current_token.type != TOKEN_GREATER_THAN) {
      parser_set_error(parser, "Expected ',' or '>' in type parameter list");
      for (size_t i = 0; i < count; i++)
        free(params[i]);
      free(params);
      return NULL;
    }
  }

  if (!parser_expect(parser, TOKEN_GREATER_THAN)) {
    for (size_t i = 0; i < count; i++)
      free(params[i]);
    free(params);
    return NULL;
  }

  *out_count = count;
  return params;
}

static char *parser_parse_type_annotation(Parser *parser) {
  if (!parser)
    return NULL;

  char *type_name = NULL;

  /* Function pointer type: fn(param_types) -> return_type */
  if (parser->current_token.type == TOKEN_FN) {
    parser_advance(parser); /* consume 'fn' */
    if (!parser_expect(parser, TOKEN_LPAREN)) {
      parser_set_error(parser,
                       "Expected '(' after 'fn' in function pointer type");
      return NULL;
    }
    char *params_buf = malloc(1024);
    if (!params_buf)
      return NULL;
    size_t params_len = 0;
    size_t params_cap = 1024;
    params_buf[0] = '\0';
    int first = 1;
    while (parser->current_token.type != TOKEN_RPAREN &&
           parser->current_token.type != TOKEN_EOF && !parser->has_error) {
      if (!first) {
        if (parser->current_token.type != TOKEN_COMMA) {
          parser_set_error(
              parser, "Expected ',' or ')' in function pointer parameter list");
          free(params_buf);
          return NULL;
        }
        parser_advance(parser); /* consume ',' */
        if (params_len + 1 >= params_cap) {
          params_cap *= 2;
          params_buf = realloc(params_buf, params_cap);
          if (!params_buf)
            return NULL;
        }
        params_buf[params_len++] = ',';
        params_buf[params_len] = '\0';
      }
      first = 0;
      char *param = parser_parse_type_annotation(parser);
      if (!param) {
        if (!parser->has_error)
          parser_set_error(parser,
                           "Expected type in function pointer parameter list");
        free(params_buf);
        return NULL;
      }
      size_t plen = strlen(param);
      while (params_len + plen + 1 >= params_cap) {
        params_cap *= 2;
        params_buf = realloc(params_buf, params_cap);
        if (!params_buf) {
          free(param);
          free(params_buf);
          return NULL;
        }
      }
      memcpy(params_buf + params_len, param, plen + 1);
      params_len += plen;
      free(param);
    }
    if (!parser_expect(parser, TOKEN_RPAREN)) {
      free(params_buf);
      return NULL;
    }
    if (!parser_expect(parser, TOKEN_ARROW)) {
      parser_set_error(parser,
                       "Expected '->' after ')' in function pointer type");
      free(params_buf);
      return NULL;
    }
    char *ret = parser_parse_type_annotation(parser);
    if (!ret) {
      if (!parser->has_error)
        parser_set_error(
            parser, "Expected return type after '->' in function pointer type");
      free(params_buf);
      return NULL;
    }
    size_t fn_len = 4 + params_len + 3 + strlen(ret) +
                    1; /* "fn(" + params + ")->" + ret + NUL */
    type_name = malloc(fn_len);
    if (!type_name) {
      free(params_buf);
      free(ret);
      return NULL;
    }
    snprintf(type_name, fn_len, "fn(%s)->%s", params_buf, ret);
    free(params_buf);
    free(ret);
    /* Continue to allow * and [] suffixes (e.g. fn()->int32* is nonsensical but
     * we handle it) */
  } else if (!parser_is_type_keyword(parser->current_token.type) &&
             !parser_is_identifier_like(parser->current_token.type)) {
    return NULL;
  } else {
    type_name = strdup(parser->current_token.value);
    parser_advance(parser);
    if (!type_name)
      return NULL;
  }

  if (parser->current_token.type == TOKEN_LESS_THAN) {
    parser_advance(parser); // consume '<'

    char *args_buf = malloc(1024);
    size_t args_len = 0;
    size_t args_cap = 1024;
    args_buf[0] = '\0';

    int first = 1;
    while (parser->current_token.type != TOKEN_GREATER_THAN &&
           parser->current_token.type != TOKEN_EOF && !parser->has_error) {
      if (!first) {
        if (parser->current_token.type != TOKEN_COMMA) {
          parser_set_error(parser, "Expected ',' or '>' in type argument list");
          free(args_buf);
          free(type_name);
          return NULL;
        }
        parser_advance(parser); // consume ','

        if (args_len + 1 >= args_cap) {
          args_cap *= 2;
          args_buf = realloc(args_buf, args_cap);
        }
        args_buf[args_len++] = ',';
        args_buf[args_len] = '\0';
      }
      first = 0;

      char *arg = parser_parse_type_annotation(parser);
      if (!arg) {
        if (!parser->has_error)
          parser_set_error(parser, "Expected type in type argument list");
        free(args_buf);
        free(type_name);
        return NULL;
      }

      size_t arg_len = strlen(arg);
      while (args_len + arg_len + 1 >= args_cap) {
        args_cap *= 2;
        args_buf = realloc(args_buf, args_cap);
      }
      memcpy(args_buf + args_len, arg, arg_len);
      args_len += arg_len;
      args_buf[args_len] = '\0';
      free(arg);
    }

    if (!parser_expect(parser, TOKEN_GREATER_THAN)) {
      free(args_buf);
      free(type_name);
      return NULL;
    }

    size_t full_len = strlen(type_name) + 1 + args_len + 1 + 1;
    char *full_type = malloc(full_len);
    snprintf(full_type, full_len, "%s<%s>", type_name, args_buf);
    free(type_name);
    free(args_buf);
    type_name = full_type;
  }

  while (parser->current_token.type == TOKEN_MULTIPLY) {
    size_t next_len = strlen(type_name) + 2;
    char *next_type = malloc(next_len);
    if (!next_type) {
      free(type_name);
      return NULL;
    }

    snprintf(next_type, next_len, "%s*", type_name);
    free(type_name);
    type_name = next_type;
    parser_advance(parser); // consume '*'
  }

  if (parser->current_token.type != TOKEN_LBRACKET) {
    return type_name;
  }

  parser_advance(parser); // consume '['

  if (parser->current_token.type != TOKEN_NUMBER) {
    free(type_name);
    parser_set_error(parser, "Expected array size after '['");
    return NULL;
  }

  char *size_text = strdup(parser->current_token.value);
  parser_advance(parser);

  if (!size_text) {
    free(type_name);
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_RBRACKET)) {
    free(type_name);
    free(size_text);
    return NULL;
  }

  size_t full_len = strlen(type_name) + strlen(size_text) + 3;
  char *full_type = malloc(full_len);
  if (!full_type) {
    free(type_name);
    free(size_text);
    return NULL;
  }

  snprintf(full_type, full_len, "%s[%s]", type_name, size_text);
  free(type_name);
  free(size_text);
  return full_type;
}

static ASTNode *parser_parse_for_initializer(Parser *parser) {
  if (!parser)
    return NULL;

  if (parser->current_token.type == TOKEN_SEMICOLON) {
    return NULL;
  }

  if (parser->current_token.type == TOKEN_VAR) {
    return parser_parse_var_declaration(parser);
  }

  ASTNode *expr = parser_parse_expression(parser);
  if (!expr)
    return NULL;

  if (parser->current_token.type == TOKEN_EQUALS) {
    return parser_parse_assignment_from_target(parser, expr);
  }

  return expr;
}

ASTNode *parser_parse_primary_expression(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  if (parser_is_identifier_like(parser->current_token.type)) {
    char *name = strdup(parser->current_token.value);
    parser_advance(parser);
    return ast_create_identifier(name, location);
  }

  switch (parser->current_token.type) {
  case TOKEN_NUMBER: {
    // Check if it's a float or integer
    char *value = parser->current_token.value;
    ASTNode *result;

    if (strchr(value, '.')) {
      double float_val = atof(value);
      result = ast_create_float_literal(float_val, location);
    } else {
      long long int_val = atoll(value);
      result = ast_create_number_literal(int_val, location);
    }

    parser_advance(parser);
    return result;
  }
  case TOKEN_STRING: {
    char *value = strdup(parser->current_token.value);
    parser_advance(parser);
    return ast_create_string_literal(value, location);
  }
  case TOKEN_IMPORT_STR: {
    parser_advance(parser); // consume 'import_str'
    if (parser->current_token.type != TOKEN_STRING) {
      parser_set_error(parser, "Expected string literal after 'import_str'");
      return NULL;
    }
    char *file_path = strdup(parser->current_token.value);
    parser_advance(parser); // consume the string
    ASTNode *node = ast_create_import_str(file_path, location);
    free(file_path);
    return node;
  }
  case TOKEN_LPAREN: {
    parser_advance(parser); // consume '('
    ASTNode *expr = parser_parse_expression(parser);
    if (!parser_expect(parser, TOKEN_RPAREN)) {
      ast_destroy_node(expr);
      return NULL;
    }
    return expr;
  }
  case TOKEN_THIS: {
    parser_advance(parser);
    return ast_create_identifier("this", location);
  }
  case TOKEN_NEW: {
    parser_advance(parser); // Built-in memory alloc handling
    if (!parser_is_identifier_like(parser->current_token.type) &&
        !parser_is_type_keyword(parser->current_token.type)) {
      parser_set_error(parser, "Expected type name after 'new'");
      return NULL;
    }
    char *type_name = strdup(parser->current_token.value);
    parser_advance(parser); // consume type name

    ASTNode *new_expr = ast_create_new_expression(type_name, location);
    free(type_name);
    return new_expr;
  }
  default:
    parser_set_error(parser, "Expected primary expression");
    return NULL;
  }
}

ASTNode *parser_parse_cast_expression(Parser *parser) {
  if (!parser || parser->current_token.type != TOKEN_LPAREN)
    return NULL;

  // Only try if the next token could start a type name
  if (!parser_is_type_keyword(parser->peek_token.type) &&
      !parser_is_identifier_like(parser->peek_token.type) &&
      parser->peek_token.type != TOKEN_FN) {
    return NULL;
  }

  size_t saved_pos = parser->lexer->position;
  size_t saved_line = parser->lexer->line;
  size_t saved_col = parser->lexer->column;
  Token saved_current = {
      parser->current_token.type,
      parser->current_token.value ? strdup(parser->current_token.value) : NULL,
      parser->current_token.line, parser->current_token.column};
  Token saved_peek = {
      parser->peek_token.type,
      parser->peek_token.value ? strdup(parser->peek_token.value) : NULL,
      parser->peek_token.line, parser->peek_token.column};
  int saved_has_error = parser->has_error;
  int saved_had_error = parser->had_error;
  size_t saved_error_count = parser->error_count;
  char *saved_error_msg =
      parser->error_message ? strdup(parser->error_message) : NULL;
  ErrorReporter *saved_error_reporter = parser->error_reporter;
  parser->error_message = NULL;
  parser->error_reporter =
      NULL; // Disable error reporting during speculative parsing

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  parser_advance(parser); // consume '('

  char *type_name = parser_parse_type_annotation(parser);
  if (!type_name || parser->has_error ||
      parser->current_token.type != TOKEN_RPAREN) {
    if (type_name)
      free(type_name);

    // Restore state
    parser->lexer->position = saved_pos;
    parser->lexer->line = saved_line;
    parser->lexer->column = saved_col;

    free(parser->current_token.value);
    parser->current_token = saved_current;
    free(parser->peek_token.value);
    parser->peek_token = saved_peek;

    parser->has_error = saved_has_error;
    parser->had_error = saved_had_error;
    parser->error_count = saved_error_count;
    parser->error_reporter = saved_error_reporter;
    if (parser->error_message)
      free(parser->error_message);
    parser->error_message = saved_error_msg;

    return NULL;
  }

  parser->error_reporter = saved_error_reporter; // Restore error reporter

  parser_advance(parser); // consume ')'

  // Check if it's a grouped expression instead
  int is_binary = parser_is_binary_operator(parser->current_token.type);
  if (parser->current_token.type == TOKEN_RPAREN ||
      parser->current_token.type == TOKEN_COMMA ||
      parser->current_token.type == TOKEN_SEMICOLON ||
      parser->current_token.type == TOKEN_EOF ||
      (is_binary && !parser_is_unary_operator(parser->current_token.type))) {

    free(type_name);

    parser->lexer->position = saved_pos;
    parser->lexer->line = saved_line;
    parser->lexer->column = saved_col;

    free(parser->current_token.value);
    parser->current_token = saved_current;
    free(parser->peek_token.value);
    parser->peek_token = saved_peek;

    parser->has_error = saved_has_error;
    parser->had_error = saved_had_error;
    parser->error_count = saved_error_count;
    parser->error_reporter = saved_error_reporter;
    if (parser->error_message)
      free(parser->error_message);
    parser->error_message = saved_error_msg;

    return NULL;
  }

  // Valid cast found, discard saved state
  free(saved_current.value);
  free(saved_peek.value);
  free(saved_error_msg);
  parser->has_error = 0;
  if (parser->error_message)
    free(parser->error_message);
  parser->error_message = NULL;

  ASTNode *operand = parser_parse_unary_expression(parser);
  if (!operand) {
    free(type_name);
    return NULL;
  }

  ASTNode *cast_expr = ast_create_cast_expression(type_name, operand, location);
  free(type_name);
  return cast_expr;
}

ASTNode *parser_parse_unary_expression(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  if (parser->current_token.type == TOKEN_LPAREN) {
    ASTNode *cast = parser_parse_cast_expression(parser);
    if (cast) {
      return cast;
    }
  }

  if (parser_is_unary_operator(parser->current_token.type)) {
    char *operator = strdup(parser->current_token.value);
    parser_advance(parser);

    ASTNode *operand = parser_parse_unary_expression(parser);
    if (!operand) {
      free(operator);
      return NULL;
    }

    return ast_create_unary_expression(operator, operand, location);
  }

  return parser_parse_postfix_expression(parser);
}

static int parser_try_parse_generic_call_type_args(Parser *parser,
                                                   char ***out_type_args,
                                                   size_t *out_type_arg_count) {
  *out_type_args = NULL;
  *out_type_arg_count = 0;

  if (parser->current_token.type != TOKEN_LESS_THAN)
    return 0;

  size_t saved_pos = parser->lexer->position;
  size_t saved_line = parser->lexer->line;
  size_t saved_col = parser->lexer->column;
  Token saved_current = {
      parser->current_token.type,
      parser->current_token.value ? strdup(parser->current_token.value) : NULL,
      parser->current_token.line, parser->current_token.column};
  Token saved_peek = {
      parser->peek_token.type,
      parser->peek_token.value ? strdup(parser->peek_token.value) : NULL,
      parser->peek_token.line, parser->peek_token.column};
  int saved_has_error = parser->has_error;
  int saved_had_error = parser->had_error;
  size_t saved_error_count = parser->error_count;
  char *saved_error_msg = parser->error_message;
  parser->error_message = NULL;

  parser_advance(parser); // consume '<'

  char **type_args = NULL;
  size_t type_arg_count = 0;
  int success = 1;

  while (parser->current_token.type != TOKEN_GREATER_THAN &&
         parser->current_token.type != TOKEN_EOF) {
    if (type_arg_count > 0) {
      if (parser->current_token.type != TOKEN_COMMA) {
        success = 0;
        break;
      }
      parser_advance(parser);
    }

    char *arg = parser_parse_type_annotation(parser);
    if (!arg || parser->has_error) {
      free(arg);
      success = 0;
      break;
    }

    type_args = realloc(type_args, (type_arg_count + 1) * sizeof(char *));
    type_args[type_arg_count++] = arg;
  }

  if (success && parser->current_token.type == TOKEN_GREATER_THAN) {
    parser_advance(parser); // consume '>'
    if (parser->current_token.type == TOKEN_LPAREN) {
      *out_type_args = type_args;
      *out_type_arg_count = type_arg_count;
      free(saved_current.value);
      free(saved_peek.value);
      free(saved_error_msg);
      parser->has_error = 0;
      free(parser->error_message);
      parser->error_message = NULL;
      return 1;
    }
  }

  for (size_t i = 0; i < type_arg_count; i++)
    free(type_args[i]);
  free(type_args);

  parser->lexer->position = saved_pos;
  parser->lexer->line = saved_line;
  parser->lexer->column = saved_col;

  free(parser->current_token.value);
  parser->current_token = saved_current;
  free(parser->peek_token.value);
  parser->peek_token = saved_peek;

  parser->has_error = saved_has_error;
  parser->had_error = saved_had_error;
  parser->error_count = saved_error_count;
  free(parser->error_message);
  parser->error_message = saved_error_msg;

  return 0;
}

ASTNode *parser_parse_postfix_expression(Parser *parser) {
  if (!parser)
    return NULL;

  ASTNode *expr = parser_parse_primary_expression(parser);
  if (!expr)
    return NULL;

  while (1) {
    SourceLocation location = {parser->current_token.line,
                               parser->current_token.column};

    if (expr->type == AST_IDENTIFIER &&
        parser->current_token.type == TOKEN_LESS_THAN) {
      char **call_type_args = NULL;
      size_t call_type_arg_count = 0;
      if (parser_try_parse_generic_call_type_args(parser, &call_type_args,
                                                  &call_type_arg_count)) {
        parser_advance(parser); // consume '('

        ASTNode **arguments = NULL;
        size_t arg_count = 0;

        if (parser->current_token.type != TOKEN_RPAREN) {
          do {
            ASTNode *arg = parser_parse_expression(parser);
            if (!arg)
              break;
            arguments = realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
            arguments[arg_count++] = arg;
            if (parser->current_token.type == TOKEN_COMMA) {
              parser_advance(parser);
            } else if (parser->current_token.type == TOKEN_RPAREN) {
              break;
            } else {
              parser_set_error(parser, "Expected ',' or ')' in argument list");
              break;
            }
          } while (1);
        }

        if (!parser_expect(parser, TOKEN_RPAREN)) {
          for (size_t i = 0; i < arg_count; i++)
            ast_destroy_node(arguments[i]);
          free(arguments);
          for (size_t i = 0; i < call_type_arg_count; i++)
            free(call_type_args[i]);
          free(call_type_args);
          ast_destroy_node(expr);
          return NULL;
        }

        Identifier *id_data = (Identifier *)expr->data;
        char *func_name = strdup(id_data->name);
        ast_destroy_node(expr);

        expr = ast_create_call_expression(func_name, arguments, arg_count,
                                          location);
        if (expr && expr->data) {
          CallExpression *ce = (CallExpression *)expr->data;
          ce->type_args = call_type_args;
          ce->type_arg_count = call_type_arg_count;
        } else {
          for (size_t i = 0; i < call_type_arg_count; i++)
            free(call_type_args[i]);
          free(call_type_args);
        }
        free(func_name);
        continue;
      }
    }

    if (parser->current_token.type == TOKEN_LPAREN) {
      // Function call or method call
      parser_advance(parser); // consume '('

      // Parse arguments first (common to both cases)
      ASTNode **arguments = NULL;
      size_t arg_count = 0;

      if (parser->current_token.type != TOKEN_RPAREN) {
        do {
          ASTNode *arg = parser_parse_expression(parser);
          if (!arg)
            break;

          arguments = realloc(arguments, (arg_count + 1) * sizeof(ASTNode *));
          arguments[arg_count] = arg;
          arg_count++;

          if (parser->current_token.type == TOKEN_COMMA) {
            parser_advance(parser);
          } else if (parser->current_token.type == TOKEN_RPAREN) {
            break;
          } else {
            parser_set_error(parser, "Expected ',' or ')' in argument list");
            break;
          }
        } while (1);
      }

      if (!parser_expect(parser, TOKEN_RPAREN)) {
        for (size_t i = 0; i < arg_count; i++) {
          ast_destroy_node(arguments[i]);
        }
        free(arguments);
        ast_destroy_node(expr);
        return NULL;
      }

      if (expr->type == AST_MEMBER_ACCESS) {
        // Method call: obj.method(args)
        MemberAccess *access = (MemberAccess *)expr->data;
        char *method_name = strdup(access->member);
        ASTNode *object = access->object;
        // Detach the object from the member access so it's not double-freed
        access->object = NULL;
        ast_destroy_node(expr);

        expr = ast_create_method_call(object, method_name, arguments, arg_count,
                                      location);
        free(method_name);
      } else if (expr->type == AST_IDENTIFIER) {
        // Regular function call (or function pointer variable - type checker
        // validates)
        Identifier *id_data = (Identifier *)expr->data;
        char *func_name = strdup(id_data->name);
        ast_destroy_node(expr);

        expr = ast_create_call_expression(func_name, arguments, arg_count,
                                          location);
        free(func_name);
      } else {
        // Expression like (*fp)(args) or (expr)(args) - function pointer call
        expr = ast_create_func_ptr_call(expr, arguments, arg_count, location);
        for (size_t i = 0; i < arg_count; i++) {
          ast_destroy_node(arguments[i]);
        }
        free(arguments);
        ast_destroy_node(expr);
        return NULL;
      }

    } else if (parser->current_token.type == TOKEN_DOT) {
      // Member access
      parser_advance(parser); // consume '.'

      if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected member name after '.'");
        ast_destroy_node(expr);
        return NULL;
      }

      char *member = strdup(parser->current_token.value);
      parser_advance(parser);

      expr = ast_create_member_access(expr, member, location);
      free(member);

    } else if (parser->current_token.type == TOKEN_ARROW) {
      // Pointer member access: p->field == (*p).field
      parser_advance(parser); // consume '->'

      if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected member name after '->'");
        ast_destroy_node(expr);
        return NULL;
      }

      char *member = strdup(parser->current_token.value);
      parser_advance(parser);

      ASTNode *deref = ast_create_unary_expression("*", expr, location);
      if (!deref) {
        free(member);
        ast_destroy_node(expr);
        return NULL;
      }

      expr = ast_create_member_access(deref, member, location);
      free(member);

    } else if (parser->current_token.type == TOKEN_LBRACKET) {
      parser_advance(parser); // consume '['

      ASTNode *index_expr = parser_parse_expression(parser);
      if (!index_expr) {
        ast_destroy_node(expr);
        return NULL;
      }

      if (!parser_expect(parser, TOKEN_RBRACKET)) {
        ast_destroy_node(index_expr);
        ast_destroy_node(expr);
        return NULL;
      }

      expr = ast_create_array_index_expression(expr, index_expr, location);

    } else {
      break;
    }
  }

  return expr;
}

ASTNode *parser_parse_binary_expression(Parser *parser, int min_precedence) {
  if (!parser)
    return NULL;

  ASTNode *left = parser_parse_unary_expression(parser);
  if (!left)
    return NULL;

  while (parser_is_binary_operator(parser->current_token.type)) {
    SourceLocation location = {parser->current_token.line,
                               parser->current_token.column};
    int precedence = parser_get_operator_precedence(parser->current_token.type);
    if (precedence < min_precedence)
      break;

    char *operator = strdup(parser->current_token.value);
    parser_advance(parser);

    ASTNode *right = parser_parse_binary_expression(parser, precedence + 1);
    if (!right) {
      free(operator);
      ast_destroy_node(left);
      return NULL;
    }

    left = ast_create_binary_expression(left, operator, right, location);
    free(operator);
  }

  return left;
}

// Placeholder implementations for specific parsing functions
// These will be implemented in subsequent tasks

ASTNode *parser_parse_import_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  parser_advance(parser); // consume 'import'

  if (parser->current_token.type != TOKEN_STRING) {
    parser_set_error(parser, "Expected string literal after 'import'");
    return NULL;
  }

  char *module_name = strdup(parser->current_token.value);
  parser_advance(parser); // consume string

  if (!parser_expect_statement_end(parser)) {
    free(module_name);
    return NULL;
  }

  ASTNode *node = ast_create_import_declaration(module_name, location);
  free(module_name);

  return node;
}

static ASTNode *parser_parse_extern_var_declaration(Parser *parser) {
  if (!parser) {
    return NULL;
  }

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (!parser_expect(parser, TOKEN_VAR)) {
    return NULL;
  }

  if (!parser_is_identifier_like(parser->current_token.type)) {
    parser_set_error(parser, "Expected variable name after 'extern var'");
    return NULL;
  }

  char *var_name = strdup(parser->current_token.value);
  parser_advance(parser);
  if (!var_name) {
    parser_set_error(parser, "Memory allocation failed");
    return NULL;
  }

  if (parser->current_token.type != TOKEN_COLON) {
    parser_set_error(parser,
                     "Extern variable declarations require an explicit type");
    free(var_name);
    return NULL;
  }
  parser_advance(parser); // consume ':'

  char *type_name = parser_parse_type_annotation(parser);
  if (!type_name) {
    free(var_name);
    parser_set_error(parser, "Extern variable declarations require a type");
    return NULL;
  }

  char *link_name = NULL;
  if (parser->current_token.type == TOKEN_EQUALS) {
    parser_advance(parser); // consume '='
    if (parser->current_token.type != TOKEN_STRING) {
      parser_set_error(
          parser,
          "Extern variable declarations cannot have an initializer; expected "
          "string literal link name after '='");
      free(var_name);
      free(type_name);
      return NULL;
    }
    link_name = strdup(parser->current_token.value);
    parser_advance(parser);
    if (!link_name) {
      parser_set_error(parser, "Memory allocation failed for link name");
      free(var_name);
      free(type_name);
      return NULL;
    }
  }

  if (!parser_expect_statement_end(parser)) {
    free(var_name);
    free(type_name);
    free(link_name);
    return NULL;
  }

  ASTNode *var_decl =
      ast_create_var_declaration(var_name, type_name, NULL, location);
  free(var_name);
  free(type_name);
  if (!var_decl || !var_decl->data) {
    free(link_name);
    return var_decl;
  }

  VarDeclaration *var_data = (VarDeclaration *)var_decl->data;
  var_data->is_extern = 1;
  if (link_name) {
    var_data->link_name = strdup(link_name);
    if (!var_data->link_name) {
      free(link_name);
      ast_destroy_node(var_decl);
      parser_set_error(parser, "Memory allocation failed for link name");
      return NULL;
    }
  }
  free(link_name);

  return var_decl;
}

ASTNode *parser_parse_var_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  // Expect 'var' keyword
  if (!parser_expect(parser, TOKEN_VAR)) {
    return NULL;
  }

  // Expect identifier
  if (!parser_is_identifier_like(parser->current_token.type)) {
    parser_set_error(parser, "Expected identifier after 'var'");
    return NULL;
  }

  char *var_name = strdup(parser->current_token.value);
  parser_advance(parser);

  char *type_name = NULL;
  ASTNode *initializer = NULL;

  // Optional type annotation: ': type'
  if (parser->current_token.type == TOKEN_COLON) {
    parser_advance(parser); // consume ':'

    type_name = parser_parse_type_annotation(parser);
    if (!type_name) {
      if (!parser->has_error) {
        parser_set_error(parser, "Expected type after ':'");
      }
      free(var_name);
      return NULL;
    }
  }

  // Optional initializer: '= expression'
  if (parser->current_token.type == TOKEN_EQUALS) {
    parser_advance(parser); // consume '='

    initializer = parser_parse_expression(parser);
    if (!initializer) {
      free(var_name);
      free(type_name);
      return NULL;
    }
  }

  // For type inference, if no type is specified but there's an initializer,
  // we'll leave type_name as NULL and let the semantic analyzer infer it
  if (!type_name && !initializer) {
    parser_set_error(parser, "Variable declaration must have either a type "
                             "annotation or an initializer");
    free(var_name);
    return NULL;
  }

  // Expect a newline or semicolon to end the declaration
  parser_expect_statement_end(parser);

  ASTNode *var_decl =
      ast_create_var_declaration(var_name, type_name, initializer, location);

  free(var_name);
  free(type_name);

  return var_decl;
}

ASTNode *parser_parse_function_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  // Expect 'function' keyword
  if (!parser_expect(parser, TOKEN_FUNCTION)) {
    return NULL;
  }

  // Expect function name
  if (!parser_is_identifier_like(parser->current_token.type)) {
    parser_set_error(parser, "Expected function name after 'function'");
    return NULL;
  }

  char *func_name = strdup(parser->current_token.value);
  parser_advance(parser);

  char **func_type_params = NULL;
  size_t func_type_param_count = 0;
  if (parser->current_token.type == TOKEN_LESS_THAN) {
    func_type_params =
        parser_parse_type_param_list(parser, &func_type_param_count);
    if (!func_type_params && parser->has_error) {
      free(func_name);
      return NULL;
    }
  }

  // Expect '('
  if (!parser_expect(parser, TOKEN_LPAREN)) {
    for (size_t i = 0; i < func_type_param_count; i++)
      free(func_type_params[i]);
    free(func_type_params);
    free(func_name);
    return NULL;
  }

  // Parse parameter list
  char **param_names = NULL;
  char **param_types = NULL;
  size_t param_count = 0;

  if (parser->current_token.type != TOKEN_RPAREN) {
    do {

      // Parse parameter: name : type
      if (!parser_is_identifier_like(parser->current_token.type)) {
        parser_set_error(parser, "Expected parameter name");
        // Clean up
        for (size_t i = 0; i < param_count; i++) {
          free(param_names[i]);
          free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(func_name);
        return NULL;
      }

      // Reallocate arrays
      param_names = realloc(param_names, (param_count + 1) * sizeof(char *));
      param_types = realloc(param_types, (param_count + 1) * sizeof(char *));

      param_names[param_count] = strdup(parser->current_token.value);
      parser_advance(parser);

      // Expect ':'
      if (!parser_expect(parser, TOKEN_COLON)) {
        // Clean up
        for (size_t i = 0; i <= param_count; i++) {
          free(param_names[i]);
          if (i < param_count)
            free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(func_name);
        return NULL;
      }

      // Parse parameter type
      param_types[param_count] = parser_parse_type_annotation(parser);
      if (!param_types[param_count]) {
        if (!parser->has_error) {
          parser_set_error(parser, "Expected parameter type");
        }
        // Clean up
        for (size_t i = 0; i <= param_count; i++) {
          free(param_names[i]);
          if (i < param_count)
            free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(func_name);
        return NULL;
      }
      param_count++;

      // Check for comma (more parameters) or end of parameter list
      if (parser->current_token.type == TOKEN_COMMA) {
        parser_advance(parser);
      } else if (parser->current_token.type == TOKEN_RPAREN) {
        break;
      } else {
        parser_set_error(parser, "Expected ',' or ')' in parameter list");
        // Clean up
        for (size_t i = 0; i < param_count; i++) {
          free(param_names[i]);
          free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(func_name);
        return NULL;
      }
    } while (1);
  }

  // Expect ')'
  if (!parser_expect(parser, TOKEN_RPAREN)) {
    // Clean up
    for (size_t i = 0; i < param_count; i++) {
      free(param_names[i]);
      free(param_types[i]);
    }
    free(param_names);
    free(param_types);
    free(func_name);
    return NULL;
  }

  // Optional return type: '-> type' (or ': type' for compatibility)
  char *return_type = NULL;
  char *link_name = NULL;
  if (parser->current_token.type == TOKEN_ARROW ||
      parser->current_token.type == TOKEN_COLON) {
    parser_advance(parser); // consume return separator

    return_type = parser_parse_type_annotation(parser);
    if (!return_type) {
      if (!parser->has_error) {
        parser_set_error(parser, "Expected return type after return separator");
      }
      // Clean up
      for (size_t i = 0; i < param_count; i++) {
        free(param_names[i]);
        free(param_types[i]);
      }
      free(param_names);
      free(param_types);
      free(func_name);
      free(link_name);
      return NULL;
    }
  }

  if (parser->current_token.type == TOKEN_EQUALS) {
    parser_advance(parser); // consume '='
    if (parser->current_token.type != TOKEN_STRING) {
      parser_set_error(parser, "Expected string literal link name after '='");
      for (size_t i = 0; i < param_count; i++) {
        free(param_names[i]);
        free(param_types[i]);
      }
      free(param_names);
      free(param_types);
      free(func_name);
      free(return_type);
      return NULL;
    }
    link_name = strdup(parser->current_token.value);
    parser_advance(parser);
    if (!link_name) {
      parser_set_error(parser, "Memory allocation failed for link name");
      for (size_t i = 0; i < param_count; i++) {
        free(param_names[i]);
        free(param_types[i]);
      }
      free(param_names);
      free(param_types);
      free(func_name);
      free(return_type);
      return NULL;
    }
  }

  // Parse function body (block) or allow forward declaration terminator
  ASTNode *body = NULL;
  if (parser->current_token.type == TOKEN_LBRACE) {
    body = parser_parse_block(parser);
    if (!body && parser->has_error) {
      // Clean up
      for (size_t i = 0; i < param_count; i++) {
        free(param_names[i]);
        free(param_types[i]);
      }
      free(param_names);
      free(param_types);
      free(func_name);
      free(return_type);
      free(link_name);
      return NULL;
    }
  } else if (parser->current_token.type == TOKEN_SEMICOLON ||
             parser->current_token.type == TOKEN_NEWLINE) {
    parser_expect_statement_end(parser);
  } else {
    parser_set_error(parser,
                     "Expected function body ('{') or declaration terminator");
    // Clean up
    for (size_t i = 0; i < param_count; i++) {
      free(param_names[i]);
      free(param_types[i]);
    }
    free(param_names);
    free(param_types);
    free(func_name);
    free(return_type);
    free(link_name);
    return NULL;
  }

  ASTNode *func_decl =
      ast_create_function_declaration(func_name, param_names, param_types,
                                      param_count, return_type, body, location);
  if (func_decl && func_decl->data && link_name) {
    FunctionDeclaration *func_data = (FunctionDeclaration *)func_decl->data;
    func_data->link_name = strdup(link_name);
    if (!func_data->link_name) {
      ast_destroy_node(func_decl);
      func_decl = NULL;
      parser_set_error(parser, "Memory allocation failed for link name");
    }
  }
  if (func_decl && func_decl->data && func_type_param_count > 0) {
    FunctionDeclaration *fd = (FunctionDeclaration *)func_decl->data;
    fd->type_params = malloc(func_type_param_count * sizeof(char *));
    fd->type_param_count = func_type_param_count;
    for (size_t i = 0; i < func_type_param_count; i++) {
      fd->type_params[i] = strdup(func_type_params[i]);
    }
  }
  for (size_t i = 0; i < func_type_param_count; i++)
    free(func_type_params[i]);
  free(func_type_params);

  // Clean up temporary strings
  free(func_name);
  free(return_type);
  free(link_name);
  for (size_t i = 0; i < param_count; i++) {
    free(param_names[i]);
    free(param_types[i]);
  }
  free(param_names);
  free(param_types);

  return func_decl;
}

ASTNode *parser_parse_enum_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (!parser_expect(parser, TOKEN_ENUM))
    return NULL;

  if (!parser_is_identifier_like(parser->current_token.type)) {
    parser_set_error(parser, "Expected enum name after 'enum'");
    return NULL;
  }
  char *enum_name = strdup(parser->current_token.value);
  parser_advance(parser);

  if (!parser_expect(parser, TOKEN_LBRACE)) {
    free(enum_name);
    return NULL;
  }

  EnumVariant *variants = NULL;
  size_t variant_count = 0;

  while (parser->current_token.type != TOKEN_RBRACE &&
         parser->current_token.type != TOKEN_EOF && !parser->has_error) {
    if (parser->current_token.type == TOKEN_NEWLINE ||
        parser->current_token.type == TOKEN_COMMA) {
      parser_advance(parser);
      continue;
    }

    if (!parser_is_identifier_like(parser->current_token.type)) {
      parser_set_error(parser, "Expected enum variant name");
      break;
    }
    char *variant_name = strdup(parser->current_token.value);
    parser_advance(parser);

    ASTNode *value = NULL;
    if (parser->current_token.type == TOKEN_EQUALS) {
      parser_advance(parser);
      value = parser_parse_expression(parser);
      if (!value) {
        if (!parser->has_error)
          parser_set_error(parser, "Expected expression after '='");
        free(variant_name);
        break;
      }
    }

    variants = realloc(variants, (variant_count + 1) * sizeof(EnumVariant));
    variants[variant_count].name = variant_name;
    variants[variant_count].value = value;
    variant_count++;

    if (parser->current_token.type == TOKEN_COMMA ||
        parser->current_token.type == TOKEN_NEWLINE) {
      parser_advance(parser);
    }
  }

  if (parser->has_error) {
    for (size_t i = 0; i < variant_count; i++) {
      free(variants[i].name);
      ast_destroy_node(variants[i].value);
    }
    free(variants);
    free(enum_name);
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_RBRACE)) {
    for (size_t i = 0; i < variant_count; i++) {
      free(variants[i].name);
      ast_destroy_node(variants[i].value);
    }
    free(variants);
    free(enum_name);
    return NULL;
  }

  ASTNode *node =
      ast_create_enum_declaration(enum_name, variants, variant_count, location);

  free(enum_name);
  for (size_t i = 0; i < variant_count; i++) {
    free(variants[i].name);
  }
  free(variants);

  return node;
}

ASTNode *parser_parse_struct_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  // Expect 'struct' keyword
  if (!parser_expect(parser, TOKEN_STRUCT)) {
    return NULL;
  }

  // Expect struct name
  if (!parser_is_identifier_like(parser->current_token.type)) {
    parser_set_error(parser, "Expected struct name after 'struct'");
    return NULL;
  }

  char *struct_name = strdup(parser->current_token.value);
  parser_advance(parser);

  char **type_params = NULL;
  size_t type_param_count = 0;
  if (parser->current_token.type == TOKEN_LESS_THAN) {
    type_params = parser_parse_type_param_list(parser, &type_param_count);
    if (!type_params && parser->has_error) {
      free(struct_name);
      return NULL;
    }
  }

  // Expect '{'
  if (!parser_expect(parser, TOKEN_LBRACE)) {
    for (size_t i = 0; i < type_param_count; i++)
      free(type_params[i]);
    free(type_params);
    free(struct_name);
    return NULL;
  }

  // Parse fields and methods
  char **field_names = NULL;
  char **field_types = NULL;
  size_t field_count = 0;
  ASTNode **methods = NULL;
  size_t method_count = 0;

  while (parser->current_token.type != TOKEN_RBRACE &&
         parser->current_token.type != TOKEN_EOF && !parser->has_error) {

    // Allow blank lines and redundant separators inside struct bodies.
    if (parser->current_token.type == TOKEN_NEWLINE ||
        parser->current_token.type == TOKEN_SEMICOLON) {
      parser_advance(parser);
      continue;
    }

    if (parser->current_token.type == TOKEN_METHOD) {
      // Parse method declaration
      ASTNode *method = parser_parse_method_declaration(parser);
      if (method) {
        methods = realloc(methods, (method_count + 1) * sizeof(ASTNode *));
        methods[method_count] = method;
        method_count++;
      } else if (parser->has_error) {
        // Clean up and return
        for (size_t i = 0; i < field_count; i++) {
          free(field_names[i]);
          free(field_types[i]);
        }
        free(field_names);
        free(field_types);
        for (size_t i = 0; i < method_count; i++) {
          ast_destroy_node(methods[i]);
        }
        free(methods);
        free(struct_name);
        return NULL;
      }
    } else {
      // Parse field declaration: name: type;
      if (!parser_is_identifier_like(parser->current_token.type)) {
        parser_set_error(parser, "Expected field name or method declaration");
        // Clean up
        for (size_t i = 0; i < field_count; i++) {
          free(field_names[i]);
          free(field_types[i]);
        }
        free(field_names);
        free(field_types);
        for (size_t i = 0; i < method_count; i++) {
          ast_destroy_node(methods[i]);
        }
        free(methods);
        free(struct_name);
        return NULL;
      }

      // Reallocate field arrays
      field_names = realloc(field_names, (field_count + 1) * sizeof(char *));
      field_types = realloc(field_types, (field_count + 1) * sizeof(char *));

      field_names[field_count] = strdup(parser->current_token.value);
      parser_advance(parser);

      // Expect ':'
      if (!parser_expect(parser, TOKEN_COLON)) {
        // Clean up
        for (size_t i = 0; i <= field_count; i++) {
          free(field_names[i]);
          if (i < field_count)
            free(field_types[i]);
        }
        free(field_names);
        free(field_types);
        for (size_t i = 0; i < method_count; i++) {
          ast_destroy_node(methods[i]);
        }
        free(methods);
        free(struct_name);
        return NULL;
      }

      // Parse field type
      field_types[field_count] = parser_parse_type_annotation(parser);
      if (!field_types[field_count]) {
        if (!parser->has_error) {
          parser_set_error(parser, "Expected field type");
        }
        // Clean up
        for (size_t i = 0; i <= field_count; i++) {
          free(field_names[i]);
          if (i < field_count)
            free(field_types[i]);
        }
        free(field_names);
        free(field_types);
        for (size_t i = 0; i < method_count; i++) {
          ast_destroy_node(methods[i]);
        }
        free(methods);
        free(struct_name);
        return NULL;
      }
      field_count++;

      // Expect a newline or semicolon to end the declaration
      parser_expect_statement_end(parser);
    }
  }

  // Expect '}'
  if (!parser_expect(parser, TOKEN_RBRACE)) {
    // Clean up
    for (size_t i = 0; i < field_count; i++) {
      free(field_names[i]);
      free(field_types[i]);
    }
    free(field_names);
    free(field_types);
    for (size_t i = 0; i < method_count; i++) {
      ast_destroy_node(methods[i]);
    }
    free(methods);
    free(struct_name);
    return NULL;
  }

  ASTNode *struct_decl = ast_create_struct_declaration(
      struct_name, field_names, field_types, field_count, methods, method_count,
      location);

  if (struct_decl && struct_decl->data && type_param_count > 0) {
    StructDeclaration *sd = (StructDeclaration *)struct_decl->data;
    sd->type_params = malloc(type_param_count * sizeof(char *));
    sd->type_param_count = type_param_count;
    for (size_t i = 0; i < type_param_count; i++) {
      sd->type_params[i] = strdup(type_params[i]);
    }
  }
  for (size_t i = 0; i < type_param_count; i++)
    free(type_params[i]);
  free(type_params);

  // Keep a stable copy for debug output before freeing temporary buffers.
  char *debug_struct_name = strdup(struct_name);
  // Clean up temporary strings
  free(struct_name);
  for (size_t i = 0; i < field_count; i++) {
    free(field_names[i]);
    free(field_types[i]);
  }
  free(field_names);
  free(field_types);
  // Note: methods array is now owned by the AST node, don't free it

  free(debug_struct_name);
  return struct_decl;
}

ASTNode *parser_parse_inline_asm(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  // Expect 'asm' keyword
  if (!parser_expect(parser, TOKEN_ASM)) {
    return NULL;
  }

  // Expect '{'
  if (!parser_expect(parser, TOKEN_LBRACE)) {
    return NULL;
  }

  // Collect all tokens until we hit the closing '}'
  // We need to preserve the exact assembly code as a string
  char *assembly_code = malloc(1024); // Start with 1KB buffer
  size_t buffer_size = 1024;
  size_t code_length = 0;

  if (!assembly_code) {
    parser_set_error(parser, "Memory allocation failed");
    return NULL;
  }

  assembly_code[0] = '\0';

  size_t last_token_line = 0;

  while (parser->current_token.type != TOKEN_RBRACE &&
         parser->current_token.type != TOKEN_EOF && !parser->has_error) {

    // Add the token value to our assembly code string
    if (parser->current_token.value) {
      size_t token_len = strlen(parser->current_token.value);

      // Check if we need to expand the buffer
      if (code_length + token_len + 2 >=
          buffer_size) { // +2 for separator and null terminator
        buffer_size *= 2;
        assembly_code = realloc(assembly_code, buffer_size);
        if (!assembly_code) {
          parser_set_error(parser, "Memory allocation failed");
          return NULL;
        }
      }

      // Add a separator before the token if we already have content
      if (code_length > 0) {
        if (last_token_line != 0 &&
            parser->current_token.line > last_token_line) {
          assembly_code[code_length] = '\n';
        } else {
          assembly_code[code_length] = ' ';
        }
        code_length++;
      }

      // Copy the token value
      strcpy(assembly_code + code_length, parser->current_token.value);
      code_length += token_len;
    }

    last_token_line = parser->current_token.line;
    parser_advance(parser);
  }

  // Expect '}'
  if (!parser_expect(parser, TOKEN_RBRACE)) {
    free(assembly_code);
    return NULL;
  }

  // Create inline assembly node
  ASTNode *inline_asm = ast_create_inline_asm(assembly_code, location);

  free(assembly_code);
  return inline_asm;
}

ASTNode *parser_parse_assignment(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (parser->current_token.type != TOKEN_IDENTIFIER) {
    parser_set_error(parser, "Expected identifier in assignment");
    return NULL;
  }

  char *var_name = strdup(parser->current_token.value);
  parser_advance(parser);

  if (!parser_expect(parser, TOKEN_EQUALS)) {
    free(var_name);
    return NULL;
  }

  ASTNode *value = parser_parse_expression(parser);
  if (!value) {
    free(var_name);
    return NULL;
  }

  ASTNode *assignment = ast_create_assignment(var_name, value, location);
  free(var_name);
  return assignment;
}

ASTNode *parser_parse_return_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  parser_advance(parser); // consume 'return'

  ASTNode *value = NULL;
  if (parser->current_token.type != TOKEN_SEMICOLON &&
      parser->current_token.type != TOKEN_NEWLINE) {
    value = parser_parse_expression(parser);
  }

  ASTNode *return_stmt = ast_create_node(AST_RETURN_STATEMENT, location);
  if (return_stmt && value) {
    ReturnStatement *ret_data = malloc(sizeof(ReturnStatement));
    ret_data->value = value;
    return_stmt->data = ret_data;
    ast_add_child(return_stmt, value);
  }

  return return_stmt;
}

ASTNode *parser_parse_if_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  if (!parser_expect(parser, TOKEN_IF)) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_LPAREN)) {
    parser_set_error(parser, "Expected '(' after 'if'");
    return NULL;
  }

  ASTNode *condition = parser_parse_expression(parser);
  if (!condition) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_RPAREN)) {
    parser_set_error(parser, "Expected ')' after if condition");
    ast_destroy_node(condition);
    return NULL;
  }

  ASTNode *then_branch = (parser->current_token.type == TOKEN_LBRACE)
                             ? parser_parse_block(parser)
                             : parser_parse_statement(parser);
  if (!then_branch) {
    ast_destroy_node(condition);
    return NULL;
  }

  ElseIfClause *else_ifs = NULL;
  size_t else_if_count = 0;
  ASTNode *else_branch = NULL;

  while (parser->current_token.type == TOKEN_ELSE) {
    if (parser->peek_token.type == TOKEN_IF) {
      parser_advance(parser); // consume 'else'
      parser_advance(parser); // consume 'if'

      if (!parser_expect(parser, TOKEN_LPAREN)) {
        parser_set_error(parser, "Expected '(' after 'else if'");
        goto cleanup;
      }

      ASTNode *elif_cond = parser_parse_expression(parser);
      if (!elif_cond)
        goto cleanup;

      if (!parser_expect(parser, TOKEN_RPAREN)) {
        parser_set_error(parser, "Expected ')' after else if condition");
        ast_destroy_node(elif_cond);
        goto cleanup;
      }

      ASTNode *elif_body = (parser->current_token.type == TOKEN_LBRACE)
                               ? parser_parse_block(parser)
                               : parser_parse_statement(parser);
      if (!elif_body) {
        ast_destroy_node(elif_cond);
        goto cleanup;
      }

      else_ifs = realloc(else_ifs, (else_if_count + 1) * sizeof(ElseIfClause));
      else_ifs[else_if_count].condition = elif_cond;
      else_ifs[else_if_count].body = elif_body;
      else_if_count++;
    } else {
      parser_advance(parser); // consume 'else'
      else_branch = (parser->current_token.type == TOKEN_LBRACE)
                        ? parser_parse_block(parser)
                        : parser_parse_statement(parser);
      if (!else_branch)
        goto cleanup;
      break;
    }
  }

  ASTNode *if_node = ast_create_node(AST_IF_STATEMENT, location);
  if (!if_node)
    goto cleanup;

  IfStatement *if_data = malloc(sizeof(IfStatement));
  if (!if_data) {
    ast_destroy_node(if_node);
    goto cleanup;
  }

  if_data->condition = condition;
  if_data->then_branch = then_branch;
  if_data->else_ifs = else_ifs;
  if_data->else_if_count = else_if_count;
  if_data->else_branch = else_branch;
  if_node->data = if_data;

  ast_add_child(if_node, condition);
  ast_add_child(if_node, then_branch);
  for (size_t i = 0; i < else_if_count; i++) {
    ast_add_child(if_node, else_ifs[i].condition);
    ast_add_child(if_node, else_ifs[i].body);
  }
  if (else_branch)
    ast_add_child(if_node, else_branch);

  return if_node;

cleanup:
  ast_destroy_node(condition);
  ast_destroy_node(then_branch);
  for (size_t i = 0; i < else_if_count; i++) {
    ast_destroy_node(else_ifs[i].condition);
    ast_destroy_node(else_ifs[i].body);
  }
  free(else_ifs);
  if (else_branch)
    ast_destroy_node(else_branch);
  return NULL;
}

ASTNode *parser_parse_while_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};

  if (!parser_expect(parser, TOKEN_WHILE)) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_LPAREN)) {
    parser_set_error(parser, "Expected '(' after 'while'");
    return NULL;
  }

  ASTNode *condition = parser_parse_expression(parser);
  if (!condition) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_RPAREN)) {
    parser_set_error(parser, "Expected ')' after while condition");
    ast_destroy_node(condition);
    return NULL;
  }

  ASTNode *body = (parser->current_token.type == TOKEN_LBRACE)
                      ? parser_parse_block(parser)
                      : parser_parse_statement(parser);
  if (!body) {
    ast_destroy_node(condition);
    return NULL;
  }

  ASTNode *while_node = ast_create_node(AST_WHILE_STATEMENT, location);
  if (!while_node) {
    ast_destroy_node(condition);
    ast_destroy_node(body);
    return NULL;
  }

  WhileStatement *while_data = malloc(sizeof(WhileStatement));
  if (!while_data) {
    ast_destroy_node(while_node);
    ast_destroy_node(condition);
    ast_destroy_node(body);
    return NULL;
  }

  while_data->condition = condition;
  while_data->body = body;
  while_node->data = while_data;

  ast_add_child(while_node, condition);
  ast_add_child(while_node, body);

  return while_node;
}

ASTNode *parser_parse_break_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (!parser_expect(parser, TOKEN_BREAK)) {
    return NULL;
  }

  parser_expect_statement_end(parser);
  return ast_create_break_statement(location);
}

ASTNode *parser_parse_continue_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (!parser_expect(parser, TOKEN_CONTINUE)) {
    return NULL;
  }

  parser_expect_statement_end(parser);
  return ast_create_continue_statement(location);
}

ASTNode *parser_parse_for_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (!parser_expect(parser, TOKEN_FOR)) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_LPAREN)) {
    parser_set_error(parser, "Expected '(' after 'for'");
    return NULL;
  }

  ASTNode *initializer = parser_parse_for_initializer(parser);
  if (parser->has_error) {
    if (initializer)
      ast_destroy_node(initializer);
    return NULL;
  }

  if (parser->current_token.type == TOKEN_SEMICOLON) {
    parser_advance(parser);
  } else if (!initializer || initializer->type != AST_VAR_DECLARATION) {
    parser_set_error(parser, "Expected ';' after for-loop initializer");
    if (initializer)
      ast_destroy_node(initializer);
    return NULL;
  }

  ASTNode *condition = NULL;
  if (parser->current_token.type != TOKEN_SEMICOLON) {
    condition = parser_parse_expression(parser);
    if (!condition) {
      if (initializer)
        ast_destroy_node(initializer);
      return NULL;
    }
  }

  if (!parser_expect(parser, TOKEN_SEMICOLON)) {
    if (initializer)
      ast_destroy_node(initializer);
    if (condition)
      ast_destroy_node(condition);
    return NULL;
  }

  ASTNode *increment = NULL;
  if (parser->current_token.type != TOKEN_RPAREN) {
    ASTNode *expr = parser_parse_expression(parser);
    if (!expr) {
      if (initializer)
        ast_destroy_node(initializer);
      if (condition)
        ast_destroy_node(condition);
      return NULL;
    }

    if (parser->current_token.type == TOKEN_EQUALS) {
      increment = parser_parse_assignment_from_target(parser, expr);
      if (!increment) {
        if (initializer)
          ast_destroy_node(initializer);
        if (condition)
          ast_destroy_node(condition);
        return NULL;
      }
    } else {
      increment = expr;
    }
  }

  if (!parser_expect(parser, TOKEN_RPAREN)) {
    if (initializer)
      ast_destroy_node(initializer);
    if (condition)
      ast_destroy_node(condition);
    if (increment)
      ast_destroy_node(increment);
    return NULL;
  }

  ASTNode *body = (parser->current_token.type == TOKEN_LBRACE)
                      ? parser_parse_block(parser)
                      : parser_parse_statement(parser);
  if (!body) {
    if (initializer)
      ast_destroy_node(initializer);
    if (condition)
      ast_destroy_node(condition);
    if (increment)
      ast_destroy_node(increment);
    return NULL;
  }

  return ast_create_for_statement(initializer, condition, increment, body,
                                  location);
}

ASTNode *parser_parse_switch_statement(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  if (!parser_expect(parser, TOKEN_SWITCH)) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_LPAREN)) {
    parser_set_error(parser, "Expected '(' after 'switch'");
    return NULL;
  }

  ASTNode *expression = parser_parse_expression(parser);
  if (!expression) {
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_RPAREN)) {
    ast_destroy_node(expression);
    return NULL;
  }

  if (!parser_expect(parser, TOKEN_LBRACE)) {
    ast_destroy_node(expression);
    return NULL;
  }

  ASTNode **cases = NULL;
  size_t case_count = 0;
  int seen_default = 0;

  while (parser->current_token.type != TOKEN_RBRACE &&
         parser->current_token.type != TOKEN_EOF) {
    while (parser->current_token.type == TOKEN_NEWLINE ||
           parser->current_token.type == TOKEN_SEMICOLON) {
      parser_advance(parser);
    }

    if (parser->current_token.type == TOKEN_RBRACE) {
      break;
    }

    int is_default = 0;
    ASTNode *case_value = NULL;
    SourceLocation case_loc = {parser->current_token.line,
                               parser->current_token.column};

    if (parser->current_token.type == TOKEN_CASE) {
      parser_advance(parser);
      case_value = parser_parse_expression(parser);
      if (!case_value) {
        parser_set_error(parser, "Expected constant expression after 'case'");
        break;
      }
    } else if (parser->current_token.type == TOKEN_DEFAULT) {
      if (seen_default) {
        parser_set_error(parser, "Only one default case is allowed");
        break;
      }
      seen_default = 1;
      is_default = 1;
      parser_advance(parser);
    } else {
      parser_set_error(parser, "Expected 'case' or 'default' in switch");
      break;
    }

    if (!parser_expect(parser, TOKEN_COLON)) {
      if (case_value)
        ast_destroy_node(case_value);
      break;
    }

    ASTNode *case_body = ast_create_program();
    if (!case_body) {
      if (case_value)
        ast_destroy_node(case_value);
      parser_set_error(parser, "Memory allocation failed for switch case");
      break;
    }

    Program *body_prog = (Program *)case_body->data;
    while (parser->current_token.type != TOKEN_EOF &&
           parser->current_token.type != TOKEN_CASE &&
           parser->current_token.type != TOKEN_DEFAULT &&
           parser->current_token.type != TOKEN_RBRACE) {
      if (parser->current_token.type == TOKEN_NEWLINE ||
          parser->current_token.type == TOKEN_SEMICOLON) {
        parser_advance(parser);
        continue;
      }

      ASTNode *stmt = parser_parse_statement(parser);
      if (!stmt) {
        ast_destroy_node(case_body);
        if (case_value)
          ast_destroy_node(case_value);
        for (size_t i = 0; i < case_count; i++)
          ast_destroy_node(cases[i]);
        free(cases);
        ast_destroy_node(expression);
        return NULL;
      }

      body_prog->declarations =
          realloc(body_prog->declarations,
                  (body_prog->declaration_count + 1) * sizeof(ASTNode *));
      body_prog->declarations[body_prog->declaration_count++] = stmt;
      ast_add_child(case_body, stmt);
    }

    ASTNode *case_node =
        ast_create_case_clause(case_value, case_body, is_default, case_loc);
    if (!case_node) {
      ast_destroy_node(case_body);
      if (case_value)
        ast_destroy_node(case_value);
      parser_set_error(parser, "Failed to create switch case clause");
      break;
    }

    cases = realloc(cases, (case_count + 1) * sizeof(ASTNode *));
    cases[case_count++] = case_node;
  }

  if (!parser_expect(parser, TOKEN_RBRACE)) {
    for (size_t i = 0; i < case_count; i++)
      ast_destroy_node(cases[i]);
    free(cases);
    ast_destroy_node(expression);
    return NULL;
  }

  ASTNode *switch_node =
      ast_create_switch_statement(expression, cases, case_count, location);
  free(cases);
  return switch_node;
}

ASTNode *parser_parse_block(Parser *parser) {
  if (!parser)
    return NULL;

  // Expect '{'
  if (!parser_expect(parser, TOKEN_LBRACE)) {
    return NULL;
  }

  // Create a block node (we'll use a program node to hold statements)
  ASTNode *block = ast_create_program();
  if (!block)
    return NULL;

  Program *block_data = (Program *)block->data;

  // Parse statements until we hit '}'
  while (parser->current_token.type != TOKEN_RBRACE &&
         parser->current_token.type != TOKEN_EOF && !parser->has_error) {

    // Skip empty statements/newlines
    if (parser->current_token.type == TOKEN_NEWLINE ||
        parser->current_token.type == TOKEN_SEMICOLON) {
      parser_advance(parser);
      continue;
    }

    ASTNode *stmt = parser_parse_statement(parser);
    if (stmt) {
      // Add to block's statements array
      block_data->declarations =
          realloc(block_data->declarations,
                  (block_data->declaration_count + 1) * sizeof(ASTNode *));
      if (block_data->declarations) {
        block_data->declarations[block_data->declaration_count] = stmt;
        block_data->declaration_count++;
        ast_add_child(block, stmt);
      }

      // Attempt to consume optional statement end (semicolon or newline)
      // for statements that didn't already consume it.
      if (stmt->type != AST_DEFER_STATEMENT &&
          (parser->current_token.type == TOKEN_SEMICOLON ||
           parser->current_token.type == TOKEN_NEWLINE)) {
        parser_expect_statement_end(parser);
      }
    } else if (parser->has_error) {
      // Try to recover from error
      parser_recover_from_error(parser);
    } else {
      parser_set_error(parser, "Failed to parse statement in block");
    }
  }

  // Expect '}'
  if (!parser_expect(parser, TOKEN_RBRACE)) {
    ast_destroy_node(block);
    return NULL;
  }

  return block;
}

ASTNode *parser_parse_method_declaration(Parser *parser) {
  if (!parser)
    return NULL;

  SourceLocation location = {parser->current_token.line,
                             parser->current_token.column};
  // Expect 'method' keyword
  if (!parser_expect(parser, TOKEN_METHOD)) {
    return NULL;
  }

  // Expect method name
  if (!parser_is_identifier_like(parser->current_token.type)) {
    parser_set_error(parser, "Expected method name after 'method'");
    return NULL;
  }

  char *method_name = strdup(parser->current_token.value);
  parser_advance(parser);

  // Expect '('
  if (!parser_expect(parser, TOKEN_LPAREN)) {
    free(method_name);
    return NULL;
  }

  // Parse parameter list (similar to function declaration)
  char **param_names = NULL;
  char **param_types = NULL;
  size_t param_count = 0;

  if (parser->current_token.type != TOKEN_RPAREN) {
    do {
      // Parse parameter: name : type
      if (!parser_is_identifier_like(parser->current_token.type)) {
        parser_set_error(parser, "Expected parameter name");
        // Clean up
        for (size_t i = 0; i < param_count; i++) {
          free(param_names[i]);
          free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(method_name);
        return NULL;
      }

      // Reallocate arrays
      param_names = realloc(param_names, (param_count + 1) * sizeof(char *));
      param_types = realloc(param_types, (param_count + 1) * sizeof(char *));

      param_names[param_count] = strdup(parser->current_token.value);
      parser_advance(parser);

      // Expect ':'
      if (!parser_expect(parser, TOKEN_COLON)) {
        // Clean up
        for (size_t i = 0; i <= param_count; i++) {
          free(param_names[i]);
          if (i < param_count)
            free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(method_name);
        return NULL;
      }

      // Parse parameter type
      param_types[param_count] = parser_parse_type_annotation(parser);
      if (!param_types[param_count]) {
        if (!parser->has_error) {
          parser_set_error(parser, "Expected parameter type");
        }
        // Clean up
        for (size_t i = 0; i <= param_count; i++) {
          free(param_names[i]);
          if (i < param_count)
            free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(method_name);
        return NULL;
      }
      param_count++;

      // Check for comma (more parameters) or end of parameter list
      if (parser->current_token.type == TOKEN_COMMA) {
        parser_advance(parser);
      } else if (parser->current_token.type == TOKEN_RPAREN) {
        break;
      } else {
        parser_set_error(parser, "Expected ',' or ')' in parameter list");
        // Clean up
        for (size_t i = 0; i < param_count; i++) {
          free(param_names[i]);
          free(param_types[i]);
        }
        free(param_names);
        free(param_types);
        free(method_name);
        return NULL;
      }
    } while (1);
  }

  // Expect ')'
  if (!parser_expect(parser, TOKEN_RPAREN)) {
    // Clean up
    for (size_t i = 0; i < param_count; i++) {
      free(param_names[i]);
      free(param_types[i]);
    }
    free(param_names);
    free(param_types);
    free(method_name);
    return NULL;
  }

  // Optional return type: '-> type' (or ': type' for compatibility)
  char *return_type = NULL;
  if (parser->current_token.type == TOKEN_ARROW ||
      parser->current_token.type == TOKEN_COLON) {
    parser_advance(parser); // consume return separator

    return_type = parser_parse_type_annotation(parser);
    if (!return_type) {
      if (!parser->has_error) {
        parser_set_error(parser, "Expected return type after return separator");
      }
      // Clean up
      for (size_t i = 0; i < param_count; i++) {
        free(param_names[i]);
        free(param_types[i]);
      }
      free(param_names);
      free(param_types);
      free(method_name);
      return NULL;
    }
  }

  // Parse method body (block)
  ASTNode *body = NULL;
  if (parser->current_token.type == TOKEN_LBRACE) {
    body = parser_parse_block(parser);
    if (!body && parser->has_error) {
      // Clean up
      for (size_t i = 0; i < param_count; i++) {
        free(param_names[i]);
        free(param_types[i]);
      }
      free(param_names);
      free(param_types);
      free(method_name);
      free(return_type);
      return NULL;
    }
  } else {
    parser_set_error(parser, "Expected method body ('{')");
    // Clean up
    for (size_t i = 0; i < param_count; i++) {
      free(param_names[i]);
      free(param_types[i]);
    }
    free(param_names);
    free(param_types);
    free(method_name);
    free(return_type);
    return NULL;
  }

  // Create method declaration node (we'll use AST_METHOD_DECLARATION type)
  ASTNode *method_decl = ast_create_node(AST_METHOD_DECLARATION, location);
  if (!method_decl) {
    // Clean up
    for (size_t i = 0; i < param_count; i++) {
      free(param_names[i]);
      free(param_types[i]);
    }
    free(param_names);
    free(param_types);
    free(method_name);
    free(return_type);
    if (body)
      ast_destroy_node(body);
    return NULL;
  }

  // Create method declaration data (reuse FunctionDeclaration structure)
  FunctionDeclaration *method_data = malloc(sizeof(FunctionDeclaration));
  if (!method_data) {
    // Clean up
    for (size_t i = 0; i < param_count; i++) {
      free(param_names[i]);
      free(param_types[i]);
    }
    free(param_names);
    free(param_types);
    free(method_name);
    free(return_type);
    if (body)
      ast_destroy_node(body);
    ast_destroy_node(method_decl);
    return NULL;
  }

  method_data->name = strdup(method_name);
  method_data->return_type = return_type ? strdup(return_type) : NULL;
  method_data->parameter_count = param_count;
  method_data->body = body;

  if (param_count > 0) {
    method_data->parameter_names = malloc(param_count * sizeof(char *));
    method_data->parameter_types = malloc(param_count * sizeof(char *));

    for (size_t i = 0; i < param_count; i++) {
      method_data->parameter_names[i] = strdup(param_names[i]);
      method_data->parameter_types[i] = strdup(param_types[i]);
    }
  } else {
    method_data->parameter_names = NULL;
    method_data->parameter_types = NULL;
  }

  method_decl->data = method_data;

  if (body) {
    ast_add_child(method_decl, body);
  }

  // Clean up temporary strings
  free(method_name);
  free(return_type);
  for (size_t i = 0; i < param_count; i++) {
    free(param_names[i]);
    free(param_types[i]);
  }
  free(param_names);
  free(param_types);

  return method_decl;
}
