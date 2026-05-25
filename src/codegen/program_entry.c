#include "program_entry.h"

#include <string.h>

int program_main_wants_argc_argv(const ASTNode *program) {
  Program *program_data;

  if (!program || program->type != AST_PROGRAM) {
    return 0;
  }

  program_data = (Program *)program->data;
  if (!program_data) {
    return 0;
  }

  for (size_t i = 0; i < program_data->declaration_count; i++) {
    ASTNode *declaration = program_data->declarations[i];
    FunctionDeclaration *func_data;

    if (!declaration || declaration->type != AST_FUNCTION_DECLARATION) {
      continue;
    }

    func_data = (FunctionDeclaration *)declaration->data;
    if (!func_data || !func_data->name ||
        strcmp(func_data->name, "main") != 0) {
      continue;
    }

    if (func_data->parameter_count == 2 && func_data->parameter_types &&
        func_data->parameter_types[0] && func_data->parameter_types[1]) {
      const char *p0 = func_data->parameter_types[0];
      const char *p1 = func_data->parameter_types[1];
      int p0_ok = (strcmp(p0, "int32") == 0 || strcmp(p0, "int64") == 0);
      int p1_ok = (strcmp(p1, "cstring*") == 0 ||
                   (p1[0] && strstr(p1, "*") != NULL));
      if (p0_ok && p1_ok) {
        return 1;
      }
    }
    return 0;
  }

  return 0;
}
