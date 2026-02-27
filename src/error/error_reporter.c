#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "error_reporter.h"
#include "../parser/ast.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#else
#include <unistd.h>
#endif

#define INITIAL_ERROR_CAPACITY 16
#define MAX_ERRORS_DEFAULT 100
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define ANSI_BOLD "\x1b[1m"

static int error_reporter_should_use_color(void) {
  const char *no_color = getenv("NO_COLOR");
  if (no_color && no_color[0] != '\0') {
    return 0;
  }

  const char *term = getenv("TERM");
  if (term && strcmp(term, "dumb") == 0) {
    return 0;
  }

  int fd = fileno(stdout);
  if (fd < 0) {
    return 0;
  }

  return isatty(fd) ? 1 : 0;
}

static size_t error_reporter_count_digits(size_t n) {
  size_t digits = 1;
  while (n >= 10) {
    n /= 10;
    digits++;
  }
  return digits;
}

ErrorReporter *error_reporter_create(const char *filename,
                                     const char *source_code) {
  ErrorReporter *reporter = malloc(sizeof(ErrorReporter));
  if (!reporter)
    return NULL;

  reporter->errors = malloc(sizeof(ErrorReport) * INITIAL_ERROR_CAPACITY);
  if (!reporter->errors) {
    free(reporter);
    return NULL;
  }

  reporter->count = 0;
  reporter->capacity = INITIAL_ERROR_CAPACITY;
  reporter->max_errors = MAX_ERRORS_DEFAULT;
  reporter->source_code = source_code;
  reporter->filename = filename ? strdup(filename) : NULL;

  return reporter;
}

SourceSpan source_span_create(size_t line, size_t column, size_t length) {
  SourceSpan span;
  span.line = line;
  span.column = column;
  span.length = length;
  return span;
}

SourceSpan source_span_from_location(SourceLocation location, size_t length) {
  return source_span_create(location.line, location.column, length);
}

void error_reporter_destroy(ErrorReporter *reporter) {
  if (!reporter)
    return;

  for (size_t i = 0; i < reporter->count; i++) {
    free(reporter->errors[i].message);
    free(reporter->errors[i].suggestion);
    free(reporter->errors[i].code_snippet);
  }

  free(reporter->errors);
  free((char *)reporter->filename);
  free(reporter);
}

static int error_reporter_expand_capacity(ErrorReporter *reporter) {
  if (reporter->count >= reporter->capacity) {
    size_t new_capacity = reporter->capacity * 2;
    ErrorReport *new_errors =
        realloc(reporter->errors, sizeof(ErrorReport) * new_capacity);
    if (!new_errors)
      return 0;

    reporter->errors = new_errors;
    reporter->capacity = new_capacity;
  }
  return 1;
}

void error_reporter_add_error(ErrorReporter *reporter, ErrorType type,
                              SourceLocation location, const char *message) {
  error_reporter_add_error_with_suggestion(reporter, type, location, message,
                                           NULL);
}

void error_reporter_add_error_with_suggestion(ErrorReporter *reporter,
                                              ErrorType type,
                                              SourceLocation location,
                                              const char *message,
                                              const char *suggestion) {
  SourceSpan span = source_span_from_location(location, 1);
  error_reporter_add_error_with_span_and_suggestion(reporter, type, span,
                                                    message, suggestion);
}

void error_reporter_add_error_with_span(ErrorReporter *reporter, ErrorType type,
                                        SourceSpan span, const char *message) {
  error_reporter_add_error_with_span_and_suggestion(reporter, type, span,
                                                    message, NULL);
}

void error_reporter_add_error_with_span_and_suggestion(
    ErrorReporter *reporter, ErrorType type, SourceSpan span, const char *message,
    const char *suggestion) {
  if (!reporter || reporter->count >= reporter->max_errors)
    return;

  if (!error_reporter_expand_capacity(reporter))
    return;

  if (span.length == 0)
    span.length = 1;

  ErrorReport *error = &reporter->errors[reporter->count];
  error->type = type;
  error->severity = ERROR_SEVERITY_ERROR;
  error->location = source_location_create(span.line, span.column);
  error->span = span;
  error->message = strdup(message);
  error->suggestion = suggestion ? strdup(suggestion) : NULL;
  error->code_snippet =
      error_reporter_get_line_from_source(reporter->source_code, span.line);

  reporter->count++;
}

void error_reporter_add_warning(ErrorReporter *reporter, ErrorType type,
                                SourceLocation location, const char *message) {
  SourceSpan span = source_span_from_location(location, 1);
  error_reporter_add_warning_with_span(reporter, type, span, message);
}

void error_reporter_add_warning_with_span(ErrorReporter *reporter, ErrorType type,
                                          SourceSpan span, const char *message) {
  if (!reporter || reporter->count >= reporter->max_errors)
    return;

  if (!error_reporter_expand_capacity(reporter))
    return;

  if (span.length == 0)
    span.length = 1;

  ErrorReport *error = &reporter->errors[reporter->count];
  error->type = type;
  error->severity = ERROR_SEVERITY_WARNING;
  error->location = source_location_create(span.line, span.column);
  error->span = span;
  error->message = strdup(message);
  error->suggestion = NULL;
  error->code_snippet =
      error_reporter_get_line_from_source(reporter->source_code, span.line);

  reporter->count++;
}

void error_reporter_print_errors(ErrorReporter *reporter) {
  if (!reporter)
    return;

  for (size_t i = 0; i < reporter->count; i++) {
    error_reporter_print_error(reporter, &reporter->errors[i]);
    if (i < reporter->count - 1) {
      printf("\n");
    }
  }
}

void error_reporter_print_error(ErrorReporter *reporter,
                                const ErrorReport *error) {
  if (!reporter || !error)
    return;

  const int use_color = error_reporter_should_use_color();
  const char *severity_color = "";
  const char *severity_text = "";
  const char *reset = "";
  const char *help_color = "";

  switch (error->severity) {
  case ERROR_SEVERITY_ERROR:
    severity_color = use_color ? ANSI_COLOR_RED : "";
    severity_text = "error";
    break;
  case ERROR_SEVERITY_WARNING:
    severity_color = use_color ? ANSI_COLOR_YELLOW : "";
    severity_text = "warning";
    break;
  case ERROR_SEVERITY_NOTE:
    severity_color = use_color ? ANSI_COLOR_BLUE : "";
    severity_text = "note";
    break;
  }

  reset = use_color ? ANSI_COLOR_RESET : "";
  help_color = use_color ? ANSI_COLOR_BLUE : "";

  // Print error header
  printf("%s%s%s: %s\n", severity_color, severity_text, reset, error->message);

  // Print location
  if (reporter->filename) {
    printf("  --> %s:%zu:%zu\n", reporter->filename, error->location.line,
           error->location.column);
  } else {
    printf("  --> line %zu, column %zu\n", error->location.line,
           error->location.column);
  }

  // Print code snippet with caret (+ context)
  if (error->code_snippet) {
    const size_t line = error->location.line;
    const size_t prev_line = (line > 1) ? (line - 1) : 0;
    const size_t next_line = line + 1;

    char *prev = prev_line ? error_reporter_get_line_from_source(
                                reporter->source_code, prev_line)
                          : NULL;
    char *next = error_reporter_get_line_from_source(reporter->source_code,
                                                     next_line);

    size_t max_line_num = line;
    if (next)
      max_line_num = next_line;
    if (prev)
      max_line_num = line;
    size_t gutter_width = error_reporter_count_digits(max_line_num);

    printf("%*s |\n", (int)gutter_width, "");

    if (prev) {
      printf("%*zu | %s\n", (int)gutter_width, prev_line, prev);
    }

    printf("%*zu | %s\n", (int)gutter_width, line, error->code_snippet);

    size_t caret_len = (error->span.length > 0) ? error->span.length : 1;
    char *caret_line =
        error_reporter_create_caret_line(error->location.column, caret_len);
    if (caret_line) {
      printf("%*s | %s\n", (int)gutter_width, "", caret_line);
      free(caret_line);
    }

    if (next) {
      printf("%*zu | %s\n", (int)gutter_width, next_line, next);
    }

    free(prev);
    free(next);
  }

  // Print suggestion if available
  if (error->suggestion) {
    printf("   = %shelp%s: %s\n", help_color, reset, error->suggestion);
  }
}

int error_reporter_has_errors(ErrorReporter *reporter) {
  if (!reporter)
    return 0;

  for (size_t i = 0; i < reporter->count; i++) {
    if (reporter->errors[i].severity == ERROR_SEVERITY_ERROR) {
      return 1;
    }
  }
  return 0;
}

int error_reporter_get_error_count(ErrorReporter *reporter) {
  if (!reporter)
    return 0;

  int count = 0;
  for (size_t i = 0; i < reporter->count; i++) {
    if (reporter->errors[i].severity == ERROR_SEVERITY_ERROR) {
      count++;
    }
  }
  return count;
}

SourceLocation source_location_create(size_t line, size_t column) {
  SourceLocation location;
  location.line = line;
  location.column = column;
  return location;
}

char *error_reporter_get_line_from_source(const char *source,
                                          size_t line_number) {
  if (!source || line_number == 0)
    return NULL;

  size_t current_line = 1;
  const char *line_start = source;
  const char *line_end = source;

  // Find the start of the target line
  while (*line_end && current_line < line_number) {
    if (*line_end == '\n') {
      current_line++;
      line_start = line_end + 1;
    }
    line_end++;
  }

  if (current_line != line_number)
    return NULL;

  // Find the end of the line
  while (*line_end && *line_end != '\n') {
    line_end++;
  }

  // Copy the line
  size_t line_length = line_end - line_start;
  char *line = malloc(line_length + 1);
  if (!line)
    return NULL;

  strncpy(line, line_start, line_length);
  line[line_length] = '\0';

  return line;
}

char *error_reporter_create_caret_line(size_t column, size_t length) {
  if (column == 0)
    return NULL;

  size_t caret_length = column + length;
  char *caret_line = malloc(caret_length + 1);
  if (!caret_line)
    return NULL;

  // Fill with spaces up to the error column
  for (size_t i = 0; i < column - 1; i++) {
    caret_line[i] = ' ';
  }

  // Add carets for the error length
  for (size_t i = column - 1; i < column - 1 + (length > 0 ? length : 1); i++) {
    caret_line[i] = '^';
  }

  caret_line[caret_length] = '\0';
  return caret_line;
}

const char *error_reporter_suggest_for_token(const char *token) {
  if (!token)
    return NULL;

  // Common typos and suggestions
  if (strcmp(token, "fucntion") == 0)
    return "did you mean 'function'?";
  if (strcmp(token, "retrun") == 0)
    return "did you mean 'return'?";
  if (strcmp(token, "strcut") == 0)
    return "did you mean 'struct'?";
  if (strcmp(token, "methdo") == 0)
    return "did you mean 'method'?";
  if (strcmp(token, "whiel") == 0)
    return "did you mean 'while'?";
  if (strcmp(token, "fi") == 0)
    return "did you mean 'if'?";
  if (strcmp(token, "itn32") == 0)
    return "did you mean 'int32'?";
  if (strcmp(token, "int23") == 0)
    return "did you mean 'int32'?";
  if (strcmp(token, "stirng") == 0)
    return "did you mean 'string'?";
  if (strcmp(token, "flaot32") == 0)
    return "did you mean 'float32'?";
  if (strcmp(token, "flaot64") == 0)
    return "did you mean 'float64'?";

  return NULL;
}

const char *error_reporter_suggest_for_type_mismatch(const char *expected,
                                                     const char *actual) {
  if (!expected || !actual)
    return NULL;

  static char suggestion[256];

  // Suggest explicit casting
  if ((strcmp(expected, "int32") == 0 && strcmp(actual, "int64") == 0) ||
      (strcmp(expected, "int64") == 0 && strcmp(actual, "int32") == 0)) {
    snprintf(suggestion, sizeof(suggestion),
             "consider explicit casting: (%s)value", expected);
    return suggestion;
  }

  // Suggest string literal for string type
  if (strcmp(expected, "string") == 0 && strcmp(actual, "int32") == 0) {
    return "use string literal with double quotes: \"value\"";
  }

  // Suggest numeric literal for numeric types
  if (strstr(expected, "int") && strcmp(actual, "string") == 0) {
    return "use numeric literal without quotes: 42";
  }

  return NULL;
}