#ifndef PROGRAM_ENTRY_H
#define PROGRAM_ENTRY_H

#include "../parser/ast.h"

/* True when the program defines main(int32|int64, cstring*|T*). */
int program_main_wants_argc_argv(const ASTNode *program);

#endif /* PROGRAM_ENTRY_H */
