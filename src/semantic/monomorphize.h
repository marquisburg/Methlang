#ifndef MONOMORPHIZE_H
#define MONOMORPHIZE_H

#include "../error/error_reporter.h"
#include "../parser/ast.h"

int monomorphize_program(ASTNode *program, ErrorReporter *reporter);

#endif // MONOMORPHIZE_H
