#ifndef IMPORT_RESOLVER_H
#define IMPORT_RESOLVER_H

#include "../error/error_reporter.h"
#include "../parser/ast.h"


// Resolves imports by finding AST_IMPORT nodes, lexing/parsing the imported
// files, and merging their declarations into the main program's AST. Returns 1
// on success, 0 on failure.
int resolve_imports(ASTNode *program, const char *base_path,
                    ErrorReporter *reporter);

#endif // IMPORT_RESOLVER_H
