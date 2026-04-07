#ifndef ASYNC_REWRITE_H
#define ASYNC_REWRITE_H

#include "../error/error_reporter.h"
#include "../parser/ast.h"

typedef enum {
  ASYNC_REWRITE_MODEL_POOL = 0,
  ASYNC_REWRITE_MODEL_COROUTINE = 1,
} AsyncRewriteModel;

int async_rewrite_program(ASTNode *program, ErrorReporter *reporter,
                          AsyncRewriteModel model);

#endif // ASYNC_REWRITE_H
