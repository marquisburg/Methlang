#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include <stddef.h>

typedef enum {
  ERROR_LEXICAL,
  ERROR_SYNTAX,
  ERROR_SEMANTIC,
  ERROR_TYPE,
  ERROR_SCOPE,
  ERROR_IO,
  ERROR_INTERNAL
} ErrorType;

typedef enum {
  ERROR_SEVERITY_ERROR,
  ERROR_SEVERITY_WARNING,
  ERROR_SEVERITY_NOTE
} ErrorSeverity;

#include "../parser/ast.h"

typedef struct {
  ErrorType type;
  ErrorSeverity severity;
  SourceLocation location;
  char *message;
  char *suggestion;
  char *code_snippet;
} ErrorReport;

typedef struct {
  ErrorReport *errors;
  size_t count;
  size_t capacity;
  size_t max_errors;
  const char *source_code;
  const char *filename;
} ErrorReporter;

// Function declarations
ErrorReporter *error_reporter_create(const char *filename,
                                     const char *source_code);
void error_reporter_destroy(ErrorReporter *reporter);

// Error reporting functions
void error_reporter_add_error(ErrorReporter *reporter, ErrorType type,
                              SourceLocation location, const char *message);
void error_reporter_add_error_with_suggestion(ErrorReporter *reporter,
                                              ErrorType type,
                                              SourceLocation location,
                                              const char *message,
                                              const char *suggestion);
void error_reporter_add_warning(ErrorReporter *reporter, ErrorType type,
                                SourceLocation location, const char *message);

// Error display functions
void error_reporter_print_errors(ErrorReporter *reporter);
void error_reporter_print_error(ErrorReporter *reporter,
                                const ErrorReport *error);
int error_reporter_has_errors(ErrorReporter *reporter);
int error_reporter_get_error_count(ErrorReporter *reporter);

// Utility functions
SourceLocation source_location_create(size_t line, size_t column);
char *error_reporter_get_line_from_source(const char *source,
                                          size_t line_number);
char *error_reporter_create_caret_line(size_t column, size_t length);

// Common error suggestions
const char *error_reporter_suggest_for_token(const char *token);
const char *error_reporter_suggest_for_type_mismatch(const char *expected,
                                                     const char *actual);

#endif // ERROR_REPORTER_H