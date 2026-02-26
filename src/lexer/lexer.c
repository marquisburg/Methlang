#include "lexer.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Lexer *lexer_create(const char *source) {
  Lexer *lexer = malloc(sizeof(Lexer));
  if (!lexer)
    return NULL;

  lexer->source = source;
  lexer->position = 0;
  lexer->line = 1;
  lexer->column = 1;
  lexer->length = strlen(source);
  lexer->error_message = NULL;
  lexer->has_error = 0;

  return lexer;
}

void lexer_destroy(Lexer *lexer) {
  if (lexer) {
    if (lexer->error_message) {
      free(lexer->error_message);
    }
    free(lexer);
  }
}

Token lexer_next_token(Lexer *lexer) {
  Token token = {TOKEN_EOF, NULL, lexer->line, lexer->column};

  // Skip whitespace
  while (lexer->position < lexer->length &&
         isspace(lexer->source[lexer->position])) {
    if (lexer->source[lexer->position] == '\n') {
      token.type = TOKEN_NEWLINE;
      token.value = NULL;
      lexer->position++;
      lexer->line++;
      lexer->column = 1;
      return token;
    }
    lexer->column++;
    lexer->position++;
  }

  if (lexer->position >= lexer->length) {
    return token; // EOF
  }

  char current = lexer->source[lexer->position];
  token.line = lexer->line;
  token.column = lexer->column;

  // Handle comments first
  if (current == '/' && lexer->position + 1 < lexer->length &&
      lexer->source[lexer->position + 1] == '/') {
    // Skip line comment
    lexer->position += 2; // Skip '//'
    lexer->column += 2;

    // Skip until end of line
    while (lexer->position < lexer->length &&
           lexer->source[lexer->position] != '\n') {
      lexer->position++;
      lexer->column++;
    }

    // Recursively get next token after comment
    return lexer_next_token(lexer);
  }

  // Single character tokens
  switch (current) {
  case ':':
    token.type = TOKEN_COLON;
    break;
  case ';':
    token.type = TOKEN_SEMICOLON;
    break;
  case ',':
    token.type = TOKEN_COMMA;
    break;
  case '=':
    if (lexer->position + 1 < lexer->length &&
        lexer->source[lexer->position + 1] == '=') {
      token.type = TOKEN_EQUALS_EQUALS;
      token.value = strdup("==");
      lexer->position += 2;
      lexer->column += 2;
      return token;
    }
    token.type = TOKEN_EQUALS;
    break;
  case '!':
    if (lexer->position + 1 < lexer->length &&
        lexer->source[lexer->position + 1] == '=') {
      token.type = TOKEN_NOT_EQUALS;
      token.value = strdup("!=");
      lexer->position += 2;
      lexer->column += 2;
      return token;
    }
    token.type = TOKEN_ERROR;
    break;
  case '<':
    if (lexer->position + 1 < lexer->length &&
        lexer->source[lexer->position + 1] == '=') {
      token.type = TOKEN_LESS_EQUALS;
      token.value = strdup("<=");
      lexer->position += 2;
      lexer->column += 2;
      return token;
    }
    token.type = TOKEN_LESS_THAN;
    break;
  case '>':
    if (lexer->position + 1 < lexer->length &&
        lexer->source[lexer->position + 1] == '=') {
      token.type = TOKEN_GREATER_EQUALS;
      token.value = strdup(">=");
      lexer->position += 2;
      lexer->column += 2;
      return token;
    }
    token.type = TOKEN_GREATER_THAN;
    break;
  case '(':
    token.type = TOKEN_LPAREN;
    break;
  case ')':
    token.type = TOKEN_RPAREN;
    break;
  case '{':
    token.type = TOKEN_LBRACE;
    break;
  case '}':
    token.type = TOKEN_RBRACE;
    break;
  case '[':
    token.type = TOKEN_LBRACKET;
    break;
  case ']':
    token.type = TOKEN_RBRACKET;
    break;
  case '+':
    token.type = TOKEN_PLUS;
    break;
  case '*':
    token.type = TOKEN_MULTIPLY;
    break;
  case '&':
    if (lexer->position + 1 < lexer->length &&
        lexer->source[lexer->position + 1] == '&') {
      token.type = TOKEN_AND_AND;
      token.value = strdup("&&");
      lexer->position += 2;
      lexer->column += 2;
      return token;
    }
    token.type = TOKEN_AMPERSAND;
    break;
  case '|':
    if (lexer->position + 1 < lexer->length &&
        lexer->source[lexer->position + 1] == '|') {
      token.type = TOKEN_OR_OR;
      token.value = strdup("||");
      lexer->position += 2;
      lexer->column += 2;
      return token;
    }
    token.type = TOKEN_ERROR;
    token.value = strdup("Unknown character: |");
    lexer_set_error(lexer, token.value);
    lexer->position++;
    lexer->column++;
    return token;
  case '/':
    // Note: comments (//) are already handled above before this switch
    token.type = TOKEN_DIVIDE;
    break;
  case '.':
    token.type = TOKEN_DOT;
    break;
  default:
    token.type = TOKEN_ERROR;
  }

  if (token.type != TOKEN_ERROR) {
    token.value = malloc(2);
    token.value[0] = current;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  // Handle arrow operator
  if (current == '-' && lexer->position + 1 < lexer->length &&
      lexer->source[lexer->position + 1] == '>') {
    token.type = TOKEN_ARROW;
    token.value = malloc(3);
    strcpy(token.value, "->");
    lexer->position += 2;
    lexer->column += 2;
    return token;
  }

  if (current == '-') {
    token.type = TOKEN_MINUS;
    token.value = malloc(2);
    token.value[0] = current;
    token.value[1] = '\0';
    lexer->position++;
    lexer->column++;
    return token;
  }

  // Character literals
  if (current == '\'') {
    lexer->position++; // skip opening quote
    lexer->column++;
    if (lexer->position >= lexer->length) {
      token.type = TOKEN_ERROR;
      token.value = strdup("Unterminated character literal");
      lexer_set_error(lexer, token.value);
      return token;
    }

    int value = 0;
    char ch = lexer->source[lexer->position];
    if (ch == '\\') {
      lexer->position++;
      lexer->column++;
      if (lexer->position >= lexer->length) {
        token.type = TOKEN_ERROR;
        token.value = strdup("Unterminated character literal");
        lexer_set_error(lexer, token.value);
        return token;
      }

      char esc = lexer->source[lexer->position];
      switch (esc) {
      case 'n':
        value = '\n';
        break;
      case 't':
        value = '\t';
        break;
      case 'r':
        value = '\r';
        break;
      case '\\':
        value = '\\';
        break;
      case '\'':
        value = '\'';
        break;
      case '0':
        value = '\0';
        break;
      default:
        token.type = TOKEN_ERROR;
        token.value = strdup("Invalid character escape sequence");
        lexer_set_error(lexer, token.value);
        return token;
      }
    } else {
      if (ch == '\n' || ch == '\r') {
        token.type = TOKEN_ERROR;
        token.value = strdup("Unterminated character literal");
        lexer_set_error(lexer, token.value);
        return token;
      }
      value = (unsigned char)ch;
    }

    lexer->position++;
    lexer->column++;
    if (lexer->position >= lexer->length || lexer->source[lexer->position] != '\'') {
      token.type = TOKEN_ERROR;
      token.value = strdup("Character literal must contain exactly one character");
      lexer_set_error(lexer, token.value);
      return token;
    }

    lexer->position++; // skip closing quote
    lexer->column++;

    token.type = TOKEN_NUMBER;
    token.value = malloc(16);
    if (!token.value) {
      token.type = TOKEN_ERROR;
      token.value = strdup("Memory allocation failed");
      lexer_set_error(lexer, token.value);
      return token;
    }
    snprintf(token.value, 16, "%d", value);
    return token;
  }

  // Numbers
  if (isdigit(current) ||
      (current == '0' && lexer->position + 1 < lexer->length &&
       (lexer->source[lexer->position + 1] == 'x' ||
        lexer->source[lexer->position + 1] == 'X' ||
        lexer->source[lexer->position + 1] == 'b' ||
        lexer->source[lexer->position + 1] == 'B'))) {
    size_t start = lexer->position;

    // Handle hexadecimal numbers (0x or 0X prefix)
    if (current == '0' && lexer->position + 1 < lexer->length &&
        (lexer->source[lexer->position + 1] == 'x' ||
         lexer->source[lexer->position + 1] == 'X')) {
      lexer->position += 2; // Skip "0x"
      lexer->column += 2;
      size_t digits_start = lexer->position;
      while (lexer->position < lexer->length &&
             isxdigit(lexer->source[lexer->position])) {
        lexer->position++;
        lexer->column++;
      }
      if (lexer->position == digits_start) {
        token.type = TOKEN_ERROR;
        token.value = strdup("Invalid hexadecimal literal");
        lexer_set_error(lexer, token.value);
        return token;
      }
    }
    // Handle binary numbers (0b or 0B prefix)
    else if (current == '0' && lexer->position + 1 < lexer->length &&
             (lexer->source[lexer->position + 1] == 'b' ||
              lexer->source[lexer->position + 1] == 'B')) {
      lexer->position += 2; // Skip "0b"
      lexer->column += 2;
      size_t digits_start = lexer->position;
      while (lexer->position < lexer->length &&
             (lexer->source[lexer->position] == '0' ||
              lexer->source[lexer->position] == '1')) {
        lexer->position++;
        lexer->column++;
      }
      if (lexer->position == digits_start) {
        token.type = TOKEN_ERROR;
        token.value = strdup("Invalid binary literal");
        lexer_set_error(lexer, token.value);
        return token;
      }
    }
    // Handle decimal numbers (including floating point)
    else {
      while (lexer->position < lexer->length &&
             (isdigit(lexer->source[lexer->position]) ||
              lexer->source[lexer->position] == '.')) {
        lexer->position++;
        lexer->column++;
      }
    }

    size_t length = lexer->position - start;
    token.type = TOKEN_NUMBER;
    token.value = malloc(length + 1);
    strncpy(token.value, &lexer->source[start], length);
    token.value[length] = '\0';
    return token;
  }

  // Identifiers and keywords
  if (isalpha(current) || current == '_') {
    size_t start = lexer->position;
    while (lexer->position < lexer->length &&
           (isalnum(lexer->source[lexer->position]) ||
            lexer->source[lexer->position] == '_')) {
      lexer->position++;
      lexer->column++;
    }

    size_t length = lexer->position - start;
    token.value = malloc(length + 1);
    strncpy(token.value, &lexer->source[start], length);
    token.value[length] = '\0';

    // Check for keywords
    if (strcmp(token.value, "import") == 0)
      token.type = TOKEN_IMPORT;
    else if (strcmp(token.value, "import_str") == 0)
      token.type = TOKEN_IMPORT_STR;
    else if (strcmp(token.value, "extern") == 0)
      token.type = TOKEN_EXTERN;
    else if (strcmp(token.value, "export") == 0)
      token.type = TOKEN_EXPORT;
    else if (strcmp(token.value, "var") == 0)
      token.type = TOKEN_VAR;
    else if (strcmp(token.value, "function") == 0)
      token.type = TOKEN_FUNCTION;
    else if (strcmp(token.value, "struct") == 0)
      token.type = TOKEN_STRUCT;
    else if (strcmp(token.value, "enum") == 0)
      token.type = TOKEN_ENUM;
    else if (strcmp(token.value, "method") == 0)
      token.type = TOKEN_METHOD;
    else if (strcmp(token.value, "return") == 0)
      token.type = TOKEN_RETURN;
    else if (strcmp(token.value, "if") == 0)
      token.type = TOKEN_IF;
    else if (strcmp(token.value, "else") == 0)
      token.type = TOKEN_ELSE;
    else if (strcmp(token.value, "while") == 0)
      token.type = TOKEN_WHILE;
    else if (strcmp(token.value, "for") == 0)
      token.type = TOKEN_FOR;
    else if (strcmp(token.value, "switch") == 0)
      token.type = TOKEN_SWITCH;
    else if (strcmp(token.value, "case") == 0)
      token.type = TOKEN_CASE;
    else if (strcmp(token.value, "default") == 0)
      token.type = TOKEN_DEFAULT;
    else if (strcmp(token.value, "break") == 0)
      token.type = TOKEN_BREAK;
    else if (strcmp(token.value, "continue") == 0)
      token.type = TOKEN_CONTINUE;
    else if (strcmp(token.value, "asm") == 0)
      token.type = TOKEN_ASM;
    else if (strcmp(token.value, "this") == 0)
      token.type = TOKEN_THIS;
    else if (strcmp(token.value, "new") == 0)
      token.type = TOKEN_NEW;

    // Type keywords
    else if (strcmp(token.value, "int8") == 0)
      token.type = TOKEN_INT8;
    else if (strcmp(token.value, "int16") == 0)
      token.type = TOKEN_INT16;
    else if (strcmp(token.value, "int32") == 0)
      token.type = TOKEN_INT32;
    else if (strcmp(token.value, "int64") == 0)
      token.type = TOKEN_INT64;
    else if (strcmp(token.value, "uint8") == 0)
      token.type = TOKEN_UINT8;
    else if (strcmp(token.value, "uint16") == 0)
      token.type = TOKEN_UINT16;
    else if (strcmp(token.value, "uint32") == 0)
      token.type = TOKEN_UINT32;
    else if (strcmp(token.value, "uint64") == 0)
      token.type = TOKEN_UINT64;
    else if (strcmp(token.value, "float32") == 0)
      token.type = TOKEN_FLOAT32;
    else if (strcmp(token.value, "float64") == 0)
      token.type = TOKEN_FLOAT64;
    else if (strcmp(token.value, "string") == 0)
      token.type = TOKEN_STRING_TYPE;

    // x86 mnemonics (case insensitive)
    else if (strcasecmp(token.value, "mov") == 0)
      token.type = TOKEN_MOV;
    else if (strcasecmp(token.value, "add") == 0)
      token.type = TOKEN_ADD;
    else if (strcasecmp(token.value, "sub") == 0)
      token.type = TOKEN_SUB;
    else if (strcasecmp(token.value, "mul") == 0)
      token.type = TOKEN_MUL;
    else if (strcasecmp(token.value, "div") == 0)
      token.type = TOKEN_DIV;
    else if (strcasecmp(token.value, "imul") == 0)
      token.type = TOKEN_IMUL;
    else if (strcasecmp(token.value, "idiv") == 0)
      token.type = TOKEN_IDIV;
    else if (strcasecmp(token.value, "inc") == 0)
      token.type = TOKEN_INC;
    else if (strcasecmp(token.value, "dec") == 0)
      token.type = TOKEN_DEC;
    else if (strcasecmp(token.value, "cmp") == 0)
      token.type = TOKEN_CMP;
    else if (strcasecmp(token.value, "jmp") == 0)
      token.type = TOKEN_JMP;
    else if (strcasecmp(token.value, "je") == 0)
      token.type = TOKEN_JE;
    else if (strcasecmp(token.value, "jne") == 0)
      token.type = TOKEN_JNE;
    else if (strcasecmp(token.value, "jl") == 0)
      token.type = TOKEN_JL;
    else if (strcasecmp(token.value, "jle") == 0)
      token.type = TOKEN_JLE;
    else if (strcasecmp(token.value, "jg") == 0)
      token.type = TOKEN_JG;
    else if (strcasecmp(token.value, "jge") == 0)
      token.type = TOKEN_JGE;
    else if (strcasecmp(token.value, "call") == 0)
      token.type = TOKEN_CALL;
    else if (strcasecmp(token.value, "ret") == 0)
      token.type = TOKEN_RET;
    else if (strcasecmp(token.value, "push") == 0)
      token.type = TOKEN_PUSH;
    else if (strcasecmp(token.value, "pop") == 0)
      token.type = TOKEN_POP;
    else if (strcasecmp(token.value, "lea") == 0)
      token.type = TOKEN_LEA;
    else if (strcasecmp(token.value, "nop") == 0)
      token.type = TOKEN_NOP;
    else if (strcasecmp(token.value, "int") == 0)
      token.type = TOKEN_INT;
    else if (strcasecmp(token.value, "syscall") == 0)
      token.type = TOKEN_SYSCALL;

    // x86 registers (case insensitive)
    else if (strcasecmp(token.value, "eax") == 0)
      token.type = TOKEN_EAX;
    else if (strcasecmp(token.value, "ebx") == 0)
      token.type = TOKEN_EBX;
    else if (strcasecmp(token.value, "ecx") == 0)
      token.type = TOKEN_ECX;
    else if (strcasecmp(token.value, "edx") == 0)
      token.type = TOKEN_EDX;
    else if (strcasecmp(token.value, "esi") == 0)
      token.type = TOKEN_ESI;
    else if (strcasecmp(token.value, "edi") == 0)
      token.type = TOKEN_EDI;
    else if (strcasecmp(token.value, "esp") == 0)
      token.type = TOKEN_ESP;
    else if (strcasecmp(token.value, "ebp") == 0)
      token.type = TOKEN_EBP;
    else if (strcasecmp(token.value, "rax") == 0)
      token.type = TOKEN_RAX;
    else if (strcasecmp(token.value, "rbx") == 0)
      token.type = TOKEN_RBX;
    else if (strcasecmp(token.value, "rcx") == 0)
      token.type = TOKEN_RCX;
    else if (strcasecmp(token.value, "rdx") == 0)
      token.type = TOKEN_RDX;
    else if (strcasecmp(token.value, "rsi") == 0)
      token.type = TOKEN_RSI;
    else if (strcasecmp(token.value, "rdi") == 0)
      token.type = TOKEN_RDI;
    else if (strcasecmp(token.value, "rsp") == 0)
      token.type = TOKEN_RSP;
    else if (strcasecmp(token.value, "rbp") == 0)
      token.type = TOKEN_RBP;
    else if (strcasecmp(token.value, "r8") == 0)
      token.type = TOKEN_R8;
    else if (strcasecmp(token.value, "r9") == 0)
      token.type = TOKEN_R9;
    else if (strcasecmp(token.value, "r10") == 0)
      token.type = TOKEN_R10;
    else if (strcasecmp(token.value, "r11") == 0)
      token.type = TOKEN_R11;
    else if (strcasecmp(token.value, "r12") == 0)
      token.type = TOKEN_R12;
    else if (strcasecmp(token.value, "r13") == 0)
      token.type = TOKEN_R13;
    else if (strcasecmp(token.value, "r14") == 0)
      token.type = TOKEN_R14;
    else if (strcasecmp(token.value, "r15") == 0)
      token.type = TOKEN_R15;

    else
      token.type = TOKEN_IDENTIFIER;

    return token;
  }

  // String literals
  if (current == '"') {
    lexer->position++; // Skip opening quote
    lexer->column++;

    // Allocate buffer for processed string (worst case: same length as input)
    size_t buffer_size = lexer->length - lexer->position;
    char *buffer = malloc(buffer_size + 1);
    if (!buffer) {
      token.type = TOKEN_ERROR;
      token.value = strdup("Memory allocation failed");
      lexer_set_error(lexer, token.value);
      return token;
    }

    size_t buffer_pos = 0;

    while (lexer->position < lexer->length &&
           lexer->source[lexer->position] != '"') {
      if (lexer->source[lexer->position] == '\\' &&
          lexer->position + 1 < lexer->length) {
        lexer->position++; // Skip backslash
        lexer->column++;

        // Process escape sequence
        char escape_char = lexer->source[lexer->position];
        switch (escape_char) {
        case 'n':
          buffer[buffer_pos++] = '\n';
          break;
        case 't':
          buffer[buffer_pos++] = '\t';
          break;
        case 'r':
          buffer[buffer_pos++] = '\r';
          break;
        case '\\':
          buffer[buffer_pos++] = '\\';
          break;
        case '"':
          buffer[buffer_pos++] = '"';
          break;
        case '0':
          buffer[buffer_pos++] = '\0';
          break;
        default:
          // Unknown escape sequence, keep both characters
          buffer[buffer_pos++] = '\\';
          buffer[buffer_pos++] = escape_char;
          break;
        }
        lexer->position++;
        lexer->column++;
      } else {
        buffer[buffer_pos++] = lexer->source[lexer->position];
        lexer->position++;
        lexer->column++;
      }
    }

    if (lexer->position >= lexer->length) {
      free(buffer);
      token.type = TOKEN_ERROR;
      token.value = strdup("Unterminated string literal");
      lexer_set_error(lexer, token.value);
      return token;
    }

    buffer[buffer_pos] = '\0';
    token.type = TOKEN_STRING;
    token.value = buffer;

    lexer->position++; // Skip closing quote
    lexer->column++;
    return token;
  }

  // Unknown character
  token.type = TOKEN_ERROR;
  token.value = malloc(32);
  if (token.value) {
    snprintf(token.value, 32, "Unknown character: %c", current);
    lexer_set_error(lexer, token.value);
  } else {
    token.value = strdup("Unknown character");
    lexer_set_error(lexer, "Unknown character");
  }
  lexer->position++;
  lexer->column++;

  return token;
}

Token lexer_peek_token(Lexer *lexer) {
  size_t saved_position = lexer->position;
  size_t saved_line = lexer->line;
  size_t saved_column = lexer->column;

  Token token = lexer_next_token(lexer);

  lexer->position = saved_position;
  lexer->line = saved_line;
  lexer->column = saved_column;

  return token;
}

void token_destroy(Token *token) {
  if (token && token->value) {
    free(token->value);
    token->value = NULL;
  }
}

Token *lexer_tokenize(Lexer *lexer, size_t *token_count) {
  if (!lexer || !token_count) {
    return NULL;
  }

  // Reset lexer position to start
  lexer->position = 0;
  lexer->line = 1;
  lexer->column = 1;

  // First pass: count tokens
  size_t count = 0;
  size_t saved_position = lexer->position;
  size_t saved_line = lexer->line;
  size_t saved_column = lexer->column;

  Token token;
  do {
    token = lexer_next_token(lexer);
    count++;
    if (token.value) {
      free(token.value);
    }
  } while (token.type != TOKEN_EOF);

  // Reset lexer position
  lexer->position = saved_position;
  lexer->line = saved_line;
  lexer->column = saved_column;

  // Allocate token array
  Token *tokens = malloc(count * sizeof(Token));
  if (!tokens) {
    *token_count = 0;
    return NULL;
  }

  // Second pass: collect tokens
  for (size_t i = 0; i < count; i++) {
    tokens[i] = lexer_next_token(lexer);
  }

  *token_count = count;
  return tokens;
}

void tokens_destroy(Token *tokens, size_t count) {
  if (!tokens)
    return;

  for (size_t i = 0; i < count; i++) {
    token_destroy(&tokens[i]);
  }
  free(tokens);
}

// Error reporting functions
void lexer_set_error(Lexer *lexer, const char *message) {
  if (!lexer)
    return;

  if (lexer->error_message) {
    free(lexer->error_message);
  }

  if (message) {
    size_t msg_len = strlen(message);
    lexer->error_message =
        malloc(msg_len + 100); // Extra space for location info
    if (lexer->error_message) {
      snprintf(lexer->error_message, msg_len + 100,
               "Lexer error at line %lu, column %lu: %s",
               (unsigned long)lexer->line, (unsigned long)lexer->column,
               message);
    }
  } else {
    lexer->error_message = NULL;
  }

  lexer->has_error = (message != NULL);
}

const char *lexer_get_error(Lexer *lexer) {
  return lexer ? lexer->error_message : NULL;
}

int lexer_has_error(Lexer *lexer) { return lexer ? lexer->has_error : 0; }

void lexer_clear_error(Lexer *lexer) {
  if (lexer) {
    if (lexer->error_message) {
      free(lexer->error_message);
      lexer->error_message = NULL;
    }
    lexer->has_error = 0;
  }
}
