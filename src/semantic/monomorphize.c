#include "monomorphize.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *generic_name;
  char **type_args;
  size_t type_arg_count;
  char *mangled_name;
} Instantiation;

typedef struct {
  char *name;
  ASTNode *node;
  char **type_params;
  size_t type_param_count;
  int is_struct; // 1 = struct, 0 = function
} GenericDef;

typedef struct {
  GenericDef *defs;
  size_t def_count;
  Instantiation *instances;
  size_t instance_count;
} MonoContext;

static char *mangle_name(const char *base, char **type_args,
                          size_t type_arg_count) {
  size_t len = strlen(base);
  for (size_t i = 0; i < type_arg_count; i++) {
    len += 2 + strlen(type_args[i]);
  }
  len += 1;

  char *result = malloc(len + 64);
  if (!result)
    return NULL;

  strcpy(result, base);
  for (size_t i = 0; i < type_arg_count; i++) {
    strcat(result, "__");
    for (const char *p = type_args[i]; *p; p++) {
      size_t cur = strlen(result);
      if (*p == '*') {
        strcat(result, "_ptr");
      } else if (*p == '<' || *p == '>' || *p == ',') {
        result[cur] = '_';
        result[cur + 1] = '\0';
      } else {
        result[cur] = *p;
        result[cur + 1] = '\0';
      }
    }
  }

  return result;
}

static int parse_generic_type_name(const char *type_str, char **out_base,
                                   char ***out_args, size_t *out_arg_count) {
  const char *lt = strchr(type_str, '<');
  if (!lt)
    return 0;

  size_t base_len = (size_t)(lt - type_str);
  *out_base = malloc(base_len + 1);
  memcpy(*out_base, type_str, base_len);
  (*out_base)[base_len] = '\0';

  const char *start = lt + 1;
  const char *end = type_str + strlen(type_str);

  // Find matching '>'
  const char *gt = NULL;
  int depth = 1;
  for (const char *p = start; *p; p++) {
    if (*p == '<')
      depth++;
    else if (*p == '>') {
      depth--;
      if (depth == 0) {
        gt = p;
        break;
      }
    }
  }

  if (!gt) {
    free(*out_base);
    *out_base = NULL;
    return 0;
  }

  // Parse comma-separated args between start and gt
  char **args = NULL;
  size_t count = 0;
  const char *p = start;

  while (p < gt) {
    while (p < gt && *p == ' ')
      p++;

    const char *arg_start = p;
    int inner_depth = 0;
    while (p < gt) {
      if (*p == '<')
        inner_depth++;
      else if (*p == '>')
        inner_depth--;
      else if (*p == ',' && inner_depth == 0)
        break;
      p++;
    }

    const char *arg_end = p;
    while (arg_end > arg_start && *(arg_end - 1) == ' ')
      arg_end--;

    size_t arg_len = (size_t)(arg_end - arg_start);
    if (arg_len > 0) {
      args = realloc(args, (count + 1) * sizeof(char *));
      args[count] = malloc(arg_len + 1);
      memcpy(args[count], arg_start, arg_len);
      args[count][arg_len] = '\0';
      count++;
    }

    if (p < gt && *p == ',')
      p++;
  }

  // Check for pointer suffix after '>' (e.g., "List<int32>*")
  const char *suffix = gt + 1;
  if (suffix < end && *suffix != '\0') {
    // There's a suffix like "*" — we need to incorporate it
    // The base stays the same, but we need the caller to handle the suffix
    // For simplicity, append the suffix to the last arg? No — the suffix
    // applies to the whole generic type. We'll handle this by including
    // the suffix in the base name replacement later.
  }

  *out_args = args;
  *out_arg_count = count;
  return 1;
}

static char *substitute_type_string(const char *type_str, char **param_names,
                                    char **arg_names, size_t count,
                                    MonoContext *ctx);

static char *substitute_type_string(const char *type_str, char **param_names,
                                    char **arg_names, size_t count,
                                    MonoContext *ctx) {
  if (!type_str)
    return NULL;

  size_t ts_len = strlen(type_str);

  // Strip trailing pointer stars and array suffixes to get the core type
  size_t core_len = ts_len;
  size_t ptr_count = 0;
  char *array_suffix = NULL;

  // Check for array suffix first: "type[N]"
  const char *lbr = NULL;
  int depth = 0;
  for (size_t i = 0; i < ts_len; i++) {
    if (type_str[i] == '<')
      depth++;
    else if (type_str[i] == '>')
      depth--;
    else if (type_str[i] == '[' && depth == 0) {
      lbr = type_str + i;
      break;
    }
  }

  if (lbr) {
    array_suffix = strdup(lbr);
    core_len = (size_t)(lbr - type_str);
  }

  // Strip trailing '*' from core
  while (core_len > 0 && type_str[core_len - 1] == '*') {
    ptr_count++;
    core_len--;
  }

  char *core = malloc(core_len + 1);
  memcpy(core, type_str, core_len);
  core[core_len] = '\0';

  // Check if core is a type parameter
  for (size_t i = 0; i < count; i++) {
    if (strcmp(core, param_names[i]) == 0) {
      free(core);
      size_t result_len = strlen(arg_names[i]) + ptr_count +
                          (array_suffix ? strlen(array_suffix) : 0) + 1;
      char *result = malloc(result_len);
      strcpy(result, arg_names[i]);
      for (size_t j = 0; j < ptr_count; j++)
        strcat(result, "*");
      if (array_suffix) {
        strcat(result, array_suffix);
        free(array_suffix);
      }
      return result;
    }
  }

  // Check if core is a generic type instantiation like "List<T>"
  char *gen_base = NULL;
  char **gen_args = NULL;
  size_t gen_arg_count = 0;
  if (parse_generic_type_name(core, &gen_base, &gen_args, &gen_arg_count)) {
    // Substitute type params within the generic args
    char **subst_args = malloc(gen_arg_count * sizeof(char *));
    for (size_t i = 0; i < gen_arg_count; i++) {
      subst_args[i] =
          substitute_type_string(gen_args[i], param_names, arg_names, count, ctx);
    }

    // Generate mangled name for this instantiation
    char *mangled = mangle_name(gen_base, subst_args, gen_arg_count);

    // Record this as an instantiation if not already recorded
    if (ctx) {
      int found = 0;
      for (size_t i = 0; i < ctx->instance_count; i++) {
        if (strcmp(ctx->instances[i].mangled_name, mangled) == 0) {
          found = 1;
          break;
        }
      }
      if (!found) {
        ctx->instances =
            realloc(ctx->instances,
                    (ctx->instance_count + 1) * sizeof(Instantiation));
        Instantiation *inst = &ctx->instances[ctx->instance_count];
        inst->generic_name = strdup(gen_base);
        inst->type_arg_count = gen_arg_count;
        inst->type_args = malloc(gen_arg_count * sizeof(char *));
        for (size_t i = 0; i < gen_arg_count; i++) {
          inst->type_args[i] = strdup(subst_args[i]);
        }
        inst->mangled_name = strdup(mangled);
        ctx->instance_count++;
      }
    }

    size_t result_len =
        strlen(mangled) + ptr_count +
        (array_suffix ? strlen(array_suffix) : 0) + 1;
    char *result = malloc(result_len);
    strcpy(result, mangled);
    for (size_t j = 0; j < ptr_count; j++)
      strcat(result, "*");
    if (array_suffix) {
      strcat(result, array_suffix);
      free(array_suffix);
    }

    free(mangled);
    free(gen_base);
    for (size_t i = 0; i < gen_arg_count; i++) {
      free(gen_args[i]);
      free(subst_args[i]);
    }
    free(gen_args);
    free(subst_args);
    free(core);
    return result;
  }

  free(core);
  free(array_suffix);

  return strdup(type_str);
}

static void substitute_types_in_ast(ASTNode *node, char **param_names,
                                    char **arg_names, size_t count,
                                    MonoContext *ctx) {
  if (!node)
    return;

  switch (node->type) {
  case AST_VAR_DECLARATION: {
    VarDeclaration *vd = (VarDeclaration *)node->data;
    if (vd && vd->type_name) {
      char *new_type =
          substitute_type_string(vd->type_name, param_names, arg_names, count, ctx);
      if (new_type) {
        free(vd->type_name);
        vd->type_name = new_type;
      }
    }
    if (vd && vd->initializer) {
      substitute_types_in_ast(vd->initializer, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_NEW_EXPRESSION: {
    NewExpression *ne = (NewExpression *)node->data;
    if (ne && ne->type_name) {
      char *new_type =
          substitute_type_string(ne->type_name, param_names, arg_names, count, ctx);
      if (new_type) {
        free(ne->type_name);
        ne->type_name = new_type;
      }
    }
    break;
  }
  case AST_FUNCTION_CALL: {
    CallExpression *ce = (CallExpression *)node->data;
    if (ce) {
      // Substitute type args in generic function calls
      if (ce->type_arg_count > 0 && ce->type_args) {
        for (size_t i = 0; i < ce->type_arg_count; i++) {
          char *new_arg = substitute_type_string(ce->type_args[i], param_names,
                                                 arg_names, count, ctx);
          if (new_arg) {
            free(ce->type_args[i]);
            ce->type_args[i] = new_arg;
          }
        }
      }
      for (size_t i = 0; i < ce->argument_count; i++) {
        substitute_types_in_ast(ce->arguments[i], param_names, arg_names, count,
                                ctx);
      }
      if (ce->object) {
        substitute_types_in_ast(ce->object, param_names, arg_names, count, ctx);
      }
    }
    break;
  }
  case AST_PROGRAM: {
    Program *prog = (Program *)node->data;
    if (prog) {
      for (size_t i = 0; i < prog->declaration_count; i++) {
        substitute_types_in_ast(prog->declarations[i], param_names, arg_names,
                                count, ctx);
      }
    }
    break;
  }
  case AST_RETURN_STATEMENT: {
    ReturnStatement *rs = (ReturnStatement *)node->data;
    if (rs && rs->value) {
      substitute_types_in_ast(rs->value, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_ASSIGNMENT: {
    Assignment *as = (Assignment *)node->data;
    if (as) {
      if (as->value)
        substitute_types_in_ast(as->value, param_names, arg_names, count, ctx);
      if (as->target)
        substitute_types_in_ast(as->target, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_IF_STATEMENT: {
    IfStatement *is = (IfStatement *)node->data;
    if (is) {
      substitute_types_in_ast(is->condition, param_names, arg_names, count, ctx);
      substitute_types_in_ast(is->then_branch, param_names, arg_names, count, ctx);
      for (size_t i = 0; i < is->else_if_count; i++) {
        substitute_types_in_ast(is->else_ifs[i].condition, param_names,
                                arg_names, count, ctx);
        substitute_types_in_ast(is->else_ifs[i].body, param_names, arg_names,
                                count, ctx);
      }
      if (is->else_branch)
        substitute_types_in_ast(is->else_branch, param_names, arg_names, count,
                                ctx);
    }
    break;
  }
  case AST_WHILE_STATEMENT: {
    WhileStatement *ws = (WhileStatement *)node->data;
    if (ws) {
      substitute_types_in_ast(ws->condition, param_names, arg_names, count, ctx);
      substitute_types_in_ast(ws->body, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_FOR_STATEMENT: {
    ForStatement *fs = (ForStatement *)node->data;
    if (fs) {
      if (fs->initializer)
        substitute_types_in_ast(fs->initializer, param_names, arg_names, count,
                                ctx);
      if (fs->condition)
        substitute_types_in_ast(fs->condition, param_names, arg_names, count,
                                ctx);
      if (fs->increment)
        substitute_types_in_ast(fs->increment, param_names, arg_names, count,
                                ctx);
      if (fs->body)
        substitute_types_in_ast(fs->body, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_SWITCH_STATEMENT: {
    SwitchStatement *ss = (SwitchStatement *)node->data;
    if (ss) {
      substitute_types_in_ast(ss->expression, param_names, arg_names, count,
                              ctx);
      for (size_t i = 0; i < ss->case_count; i++) {
        substitute_types_in_ast(ss->cases[i], param_names, arg_names, count,
                                ctx);
      }
    }
    break;
  }
  case AST_CASE_CLAUSE: {
    CaseClause *cc = (CaseClause *)node->data;
    if (cc) {
      if (cc->value)
        substitute_types_in_ast(cc->value, param_names, arg_names, count, ctx);
      if (cc->body)
        substitute_types_in_ast(cc->body, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_BINARY_EXPRESSION: {
    BinaryExpression *be = (BinaryExpression *)node->data;
    if (be) {
      substitute_types_in_ast(be->left, param_names, arg_names, count, ctx);
      substitute_types_in_ast(be->right, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *ue = (UnaryExpression *)node->data;
    if (ue && ue->operand) {
      substitute_types_in_ast(ue->operand, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_MEMBER_ACCESS: {
    MemberAccess *ma = (MemberAccess *)node->data;
    if (ma && ma->object) {
      substitute_types_in_ast(ma->object, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *ie = (ArrayIndexExpression *)node->data;
    if (ie) {
      if (ie->array)
        substitute_types_in_ast(ie->array, param_names, arg_names, count, ctx);
      if (ie->index)
        substitute_types_in_ast(ie->index, param_names, arg_names, count, ctx);
    }
    break;
  }
  case AST_DEFER_STATEMENT:
  case AST_ERRDEFER_STATEMENT: {
    DeferStatement *ds = (DeferStatement *)node->data;
    if (ds && ds->statement)
      substitute_types_in_ast(ds->statement, param_names, arg_names, count, ctx);
    break;
  }
  default:
    break;
  }
}

static void collect_generic_defs(ASTNode *program, MonoContext *ctx) {
  Program *prog = (Program *)program->data;
  if (!prog)
    return;

  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    if (!decl)
      continue;

    if (decl->type == AST_STRUCT_DECLARATION) {
      StructDeclaration *sd = (StructDeclaration *)decl->data;
      if (sd && sd->type_param_count > 0) {
        ctx->defs =
            realloc(ctx->defs, (ctx->def_count + 1) * sizeof(GenericDef));
        GenericDef *def = &ctx->defs[ctx->def_count];
        def->name = strdup(sd->name);
        def->node = decl;
        def->type_param_count = sd->type_param_count;
        def->type_params = malloc(sd->type_param_count * sizeof(char *));
        for (size_t j = 0; j < sd->type_param_count; j++) {
          def->type_params[j] = strdup(sd->type_params[j]);
        }
        def->is_struct = 1;
        ctx->def_count++;
      }
    } else if (decl->type == AST_FUNCTION_DECLARATION) {
      FunctionDeclaration *fd = (FunctionDeclaration *)decl->data;
      if (fd && fd->type_param_count > 0) {
        ctx->defs =
            realloc(ctx->defs, (ctx->def_count + 1) * sizeof(GenericDef));
        GenericDef *def = &ctx->defs[ctx->def_count];
        def->name = strdup(fd->name);
        def->node = decl;
        def->type_param_count = fd->type_param_count;
        def->type_params = malloc(fd->type_param_count * sizeof(char *));
        for (size_t j = 0; j < fd->type_param_count; j++) {
          def->type_params[j] = strdup(fd->type_params[j]);
        }
        def->is_struct = 0;
        ctx->def_count++;
      }
    }
  }
}

static void collect_type_instantiations(ASTNode *node, MonoContext *ctx) {
  if (!node)
    return;

  switch (node->type) {
  case AST_VAR_DECLARATION: {
    VarDeclaration *vd = (VarDeclaration *)node->data;
    if (vd && vd->type_name) {
      char *base = NULL;
      char **args = NULL;
      size_t arg_count = 0;
      if (parse_generic_type_name(vd->type_name, &base, &args, &arg_count)) {
        int found = 0;
        char *mangled = mangle_name(base, args, arg_count);
        for (size_t i = 0; i < ctx->instance_count; i++) {
          if (strcmp(ctx->instances[i].mangled_name, mangled) == 0) {
            found = 1;
            break;
          }
        }
        if (!found) {
          ctx->instances = realloc(
              ctx->instances,
              (ctx->instance_count + 1) * sizeof(Instantiation));
          Instantiation *inst = &ctx->instances[ctx->instance_count];
          inst->generic_name = strdup(base);
          inst->type_arg_count = arg_count;
          inst->type_args = malloc(arg_count * sizeof(char *));
          for (size_t i = 0; i < arg_count; i++) {
            inst->type_args[i] = strdup(args[i]);
          }
          inst->mangled_name = strdup(mangled);
          ctx->instance_count++;
        }
        free(mangled);
        free(base);
        for (size_t i = 0; i < arg_count; i++)
          free(args[i]);
        free(args);
      }
    }
    if (vd && vd->initializer)
      collect_type_instantiations(vd->initializer, ctx);
    break;
  }
  case AST_NEW_EXPRESSION: {
    NewExpression *ne = (NewExpression *)node->data;
    if (ne && ne->type_name) {
      char *base = NULL;
      char **args = NULL;
      size_t arg_count = 0;
      if (parse_generic_type_name(ne->type_name, &base, &args, &arg_count)) {
        int found = 0;
        char *mangled = mangle_name(base, args, arg_count);
        for (size_t i = 0; i < ctx->instance_count; i++) {
          if (strcmp(ctx->instances[i].mangled_name, mangled) == 0) {
            found = 1;
            break;
          }
        }
        if (!found) {
          ctx->instances = realloc(
              ctx->instances,
              (ctx->instance_count + 1) * sizeof(Instantiation));
          Instantiation *inst = &ctx->instances[ctx->instance_count];
          inst->generic_name = strdup(base);
          inst->type_arg_count = arg_count;
          inst->type_args = malloc(arg_count * sizeof(char *));
          for (size_t i = 0; i < arg_count; i++) {
            inst->type_args[i] = strdup(args[i]);
          }
          inst->mangled_name = strdup(mangled);
          ctx->instance_count++;
        }
        free(mangled);
        free(base);
        for (size_t i = 0; i < arg_count; i++)
          free(args[i]);
        free(args);
      }
    }
    break;
  }
  case AST_FUNCTION_CALL: {
    CallExpression *ce = (CallExpression *)node->data;
    if (ce && ce->type_arg_count > 0 && ce->function_name) {
      int found = 0;
      char *mangled =
          mangle_name(ce->function_name, ce->type_args, ce->type_arg_count);
      for (size_t i = 0; i < ctx->instance_count; i++) {
        if (strcmp(ctx->instances[i].mangled_name, mangled) == 0) {
          found = 1;
          break;
        }
      }
      if (!found) {
        ctx->instances =
            realloc(ctx->instances,
                    (ctx->instance_count + 1) * sizeof(Instantiation));
        Instantiation *inst = &ctx->instances[ctx->instance_count];
        inst->generic_name = strdup(ce->function_name);
        inst->type_arg_count = ce->type_arg_count;
        inst->type_args = malloc(ce->type_arg_count * sizeof(char *));
        for (size_t i = 0; i < ce->type_arg_count; i++) {
          inst->type_args[i] = strdup(ce->type_args[i]);
        }
        inst->mangled_name = strdup(mangled);
        ctx->instance_count++;
      }
      free(mangled);
    }
    if (ce) {
      for (size_t i = 0; i < ce->argument_count; i++)
        collect_type_instantiations(ce->arguments[i], ctx);
      if (ce->object)
        collect_type_instantiations(ce->object, ctx);
    }
    break;
  }
  case AST_FUNCTION_DECLARATION: {
    FunctionDeclaration *fd = (FunctionDeclaration *)node->data;
    if (fd && fd->type_param_count == 0 && fd->body)
      collect_type_instantiations(fd->body, ctx);
    // Also check parameter types for generic type uses
    if (fd && fd->type_param_count == 0) {
      for (size_t i = 0; i < fd->parameter_count; i++) {
        if (fd->parameter_types[i]) {
          char *base = NULL;
          char **args = NULL;
          size_t arg_count = 0;
          if (parse_generic_type_name(fd->parameter_types[i], &base, &args,
                                      &arg_count)) {
            int found = 0;
            char *mangled = mangle_name(base, args, arg_count);
            for (size_t j = 0; j < ctx->instance_count; j++) {
              if (strcmp(ctx->instances[j].mangled_name, mangled) == 0) {
                found = 1;
                break;
              }
            }
            if (!found) {
              ctx->instances = realloc(
                  ctx->instances,
                  (ctx->instance_count + 1) * sizeof(Instantiation));
              Instantiation *inst = &ctx->instances[ctx->instance_count];
              inst->generic_name = strdup(base);
              inst->type_arg_count = arg_count;
              inst->type_args = malloc(arg_count * sizeof(char *));
              for (size_t k = 0; k < arg_count; k++)
                inst->type_args[k] = strdup(args[k]);
              inst->mangled_name = strdup(mangled);
              ctx->instance_count++;
            }
            free(mangled);
            free(base);
            for (size_t k = 0; k < arg_count; k++)
              free(args[k]);
            free(args);
          }
        }
      }
      if (fd->return_type) {
        char *base = NULL;
        char **args = NULL;
        size_t arg_count = 0;
        if (parse_generic_type_name(fd->return_type, &base, &args,
                                    &arg_count)) {
          int found = 0;
          char *mangled = mangle_name(base, args, arg_count);
          for (size_t j = 0; j < ctx->instance_count; j++) {
            if (strcmp(ctx->instances[j].mangled_name, mangled) == 0) {
              found = 1;
              break;
            }
          }
          if (!found) {
            ctx->instances = realloc(
                ctx->instances,
                (ctx->instance_count + 1) * sizeof(Instantiation));
            Instantiation *inst = &ctx->instances[ctx->instance_count];
            inst->generic_name = strdup(base);
            inst->type_arg_count = arg_count;
            inst->type_args = malloc(arg_count * sizeof(char *));
            for (size_t k = 0; k < arg_count; k++)
              inst->type_args[k] = strdup(args[k]);
            inst->mangled_name = strdup(mangled);
            ctx->instance_count++;
          }
          free(mangled);
          free(base);
          for (size_t k = 0; k < arg_count; k++)
            free(args[k]);
          free(args);
        }
      }
    }
    break;
  }
  case AST_PROGRAM: {
    Program *prog = (Program *)node->data;
    if (prog) {
      for (size_t i = 0; i < prog->declaration_count; i++)
        collect_type_instantiations(prog->declarations[i], ctx);
    }
    break;
  }
  case AST_RETURN_STATEMENT: {
    ReturnStatement *rs = (ReturnStatement *)node->data;
    if (rs && rs->value)
      collect_type_instantiations(rs->value, ctx);
    break;
  }
  case AST_ASSIGNMENT: {
    Assignment *as = (Assignment *)node->data;
    if (as) {
      if (as->value)
        collect_type_instantiations(as->value, ctx);
      if (as->target)
        collect_type_instantiations(as->target, ctx);
    }
    break;
  }
  case AST_IF_STATEMENT: {
    IfStatement *is_stmt = (IfStatement *)node->data;
    if (is_stmt) {
      collect_type_instantiations(is_stmt->condition, ctx);
      collect_type_instantiations(is_stmt->then_branch, ctx);
      for (size_t i = 0; i < is_stmt->else_if_count; i++) {
        collect_type_instantiations(is_stmt->else_ifs[i].condition, ctx);
        collect_type_instantiations(is_stmt->else_ifs[i].body, ctx);
      }
      if (is_stmt->else_branch)
        collect_type_instantiations(is_stmt->else_branch, ctx);
    }
    break;
  }
  case AST_WHILE_STATEMENT: {
    WhileStatement *ws = (WhileStatement *)node->data;
    if (ws) {
      collect_type_instantiations(ws->condition, ctx);
      collect_type_instantiations(ws->body, ctx);
    }
    break;
  }
  case AST_FOR_STATEMENT: {
    ForStatement *fs = (ForStatement *)node->data;
    if (fs) {
      if (fs->initializer)
        collect_type_instantiations(fs->initializer, ctx);
      if (fs->condition)
        collect_type_instantiations(fs->condition, ctx);
      if (fs->increment)
        collect_type_instantiations(fs->increment, ctx);
      if (fs->body)
        collect_type_instantiations(fs->body, ctx);
    }
    break;
  }
  case AST_SWITCH_STATEMENT: {
    SwitchStatement *ss = (SwitchStatement *)node->data;
    if (ss) {
      collect_type_instantiations(ss->expression, ctx);
      for (size_t i = 0; i < ss->case_count; i++)
        collect_type_instantiations(ss->cases[i], ctx);
    }
    break;
  }
  case AST_CASE_CLAUSE: {
    CaseClause *cc = (CaseClause *)node->data;
    if (cc) {
      if (cc->value)
        collect_type_instantiations(cc->value, ctx);
      if (cc->body)
        collect_type_instantiations(cc->body, ctx);
    }
    break;
  }
  case AST_BINARY_EXPRESSION: {
    BinaryExpression *be = (BinaryExpression *)node->data;
    if (be) {
      collect_type_instantiations(be->left, ctx);
      collect_type_instantiations(be->right, ctx);
    }
    break;
  }
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *ue = (UnaryExpression *)node->data;
    if (ue && ue->operand)
      collect_type_instantiations(ue->operand, ctx);
    break;
  }
  case AST_MEMBER_ACCESS: {
    MemberAccess *ma = (MemberAccess *)node->data;
    if (ma && ma->object)
      collect_type_instantiations(ma->object, ctx);
    break;
  }
  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *ie = (ArrayIndexExpression *)node->data;
    if (ie) {
      if (ie->array)
        collect_type_instantiations(ie->array, ctx);
      if (ie->index)
        collect_type_instantiations(ie->index, ctx);
    }
    break;
  }
  case AST_DEFER_STATEMENT:
  case AST_ERRDEFER_STATEMENT: {
    DeferStatement *ds = (DeferStatement *)node->data;
    if (ds && ds->statement)
      collect_type_instantiations(ds->statement, ctx);
    break;
  }
  default:
    break;
  }
}

static void rewrite_generic_references(ASTNode *node, MonoContext *ctx) {
  if (!node)
    return;

  switch (node->type) {
  case AST_VAR_DECLARATION: {
    VarDeclaration *vd = (VarDeclaration *)node->data;
    if (vd && vd->type_name) {
      // Replace generic type names with mangled names
      // Need to handle "List<int32>" -> "List__int32" and "List<int32>*" -> "List__int32*"
      char *base = NULL;
      char **args = NULL;
      size_t arg_count = 0;

      // Strip pointer suffix for matching
      char *type_str = vd->type_name;
      size_t len = strlen(type_str);
      size_t ptr_count = 0;
      while (len > 0 && type_str[len - 1] == '*') {
        ptr_count++;
        len--;
      }

      char *core = malloc(len + 1);
      memcpy(core, type_str, len);
      core[len] = '\0';

      if (parse_generic_type_name(core, &base, &args, &arg_count)) {
        char *mangled = mangle_name(base, args, arg_count);
        size_t new_len = strlen(mangled) + ptr_count + 1;
        char *new_type = malloc(new_len);
        strcpy(new_type, mangled);
        for (size_t i = 0; i < ptr_count; i++)
          strcat(new_type, "*");
        free(vd->type_name);
        vd->type_name = new_type;
        free(mangled);
        free(base);
        for (size_t i = 0; i < arg_count; i++)
          free(args[i]);
        free(args);
      }
      free(core);
    }
    if (vd && vd->initializer)
      rewrite_generic_references(vd->initializer, ctx);
    break;
  }
  case AST_NEW_EXPRESSION: {
    NewExpression *ne = (NewExpression *)node->data;
    if (ne && ne->type_name) {
      char *base = NULL;
      char **args = NULL;
      size_t arg_count = 0;
      if (parse_generic_type_name(ne->type_name, &base, &args, &arg_count)) {
        char *mangled = mangle_name(base, args, arg_count);
        free(ne->type_name);
        ne->type_name = mangled;
        free(base);
        for (size_t i = 0; i < arg_count; i++)
          free(args[i]);
        free(args);
      }
    }
    break;
  }
  case AST_FUNCTION_CALL: {
    CallExpression *ce = (CallExpression *)node->data;
    if (ce && ce->type_arg_count > 0 && ce->function_name) {
      char *mangled =
          mangle_name(ce->function_name, ce->type_args, ce->type_arg_count);
      free(ce->function_name);
      ce->function_name = mangled;
      for (size_t i = 0; i < ce->type_arg_count; i++)
        free(ce->type_args[i]);
      free(ce->type_args);
      ce->type_args = NULL;
      ce->type_arg_count = 0;
    }
    if (ce) {
      for (size_t i = 0; i < ce->argument_count; i++)
        rewrite_generic_references(ce->arguments[i], ctx);
      if (ce->object)
        rewrite_generic_references(ce->object, ctx);
    }
    break;
  }
  case AST_FUNCTION_DECLARATION: {
    FunctionDeclaration *fd = (FunctionDeclaration *)node->data;
    if (fd && fd->type_param_count == 0) {
      for (size_t i = 0; i < fd->parameter_count; i++) {
        if (fd->parameter_types[i]) {
          char *base = NULL;
          char **args = NULL;
          size_t arg_count = 0;
          char *type_str = fd->parameter_types[i];
          size_t len = strlen(type_str);
          size_t ptr_count = 0;
          while (len > 0 && type_str[len - 1] == '*') {
            ptr_count++;
            len--;
          }
          char *core = malloc(len + 1);
          memcpy(core, type_str, len);
          core[len] = '\0';
          if (parse_generic_type_name(core, &base, &args, &arg_count)) {
            char *mangled = mangle_name(base, args, arg_count);
            size_t new_len = strlen(mangled) + ptr_count + 1;
            char *new_type = malloc(new_len);
            strcpy(new_type, mangled);
            for (size_t j = 0; j < ptr_count; j++)
              strcat(new_type, "*");
            free(fd->parameter_types[i]);
            fd->parameter_types[i] = new_type;
            free(mangled);
            free(base);
            for (size_t j = 0; j < arg_count; j++)
              free(args[j]);
            free(args);
          }
          free(core);
        }
      }
      if (fd->return_type) {
        char *base = NULL;
        char **args = NULL;
        size_t arg_count = 0;
        if (parse_generic_type_name(fd->return_type, &base, &args,
                                    &arg_count)) {
          char *mangled = mangle_name(base, args, arg_count);
          free(fd->return_type);
          fd->return_type = mangled;
          free(base);
          for (size_t i = 0; i < arg_count; i++)
            free(args[i]);
          free(args);
        }
      }
      if (fd->body)
        rewrite_generic_references(fd->body, ctx);
    }
    break;
  }
  case AST_STRUCT_DECLARATION: {
    StructDeclaration *sd = (StructDeclaration *)node->data;
    if (sd && sd->type_param_count == 0) {
      for (size_t i = 0; i < sd->field_count; i++) {
        if (sd->field_types[i]) {
          char *base = NULL;
          char **args = NULL;
          size_t arg_count = 0;
          char *type_str = sd->field_types[i];
          size_t len = strlen(type_str);
          size_t ptr_count = 0;
          while (len > 0 && type_str[len - 1] == '*') {
            ptr_count++;
            len--;
          }
          char *core = malloc(len + 1);
          memcpy(core, type_str, len);
          core[len] = '\0';
          if (parse_generic_type_name(core, &base, &args, &arg_count)) {
            char *mangled = mangle_name(base, args, arg_count);
            size_t new_len = strlen(mangled) + ptr_count + 1;
            char *new_type = malloc(new_len);
            strcpy(new_type, mangled);
            for (size_t j = 0; j < ptr_count; j++)
              strcat(new_type, "*");
            free(sd->field_types[i]);
            sd->field_types[i] = new_type;
            free(mangled);
            free(base);
            for (size_t j = 0; j < arg_count; j++)
              free(args[j]);
            free(args);
          }
          free(core);
        }
      }
    }
    break;
  }
  case AST_PROGRAM: {
    Program *prog = (Program *)node->data;
    if (prog) {
      for (size_t i = 0; i < prog->declaration_count; i++)
        rewrite_generic_references(prog->declarations[i], ctx);
    }
    break;
  }
  case AST_RETURN_STATEMENT: {
    ReturnStatement *rs = (ReturnStatement *)node->data;
    if (rs && rs->value)
      rewrite_generic_references(rs->value, ctx);
    break;
  }
  case AST_ASSIGNMENT: {
    Assignment *as = (Assignment *)node->data;
    if (as) {
      if (as->value)
        rewrite_generic_references(as->value, ctx);
      if (as->target)
        rewrite_generic_references(as->target, ctx);
    }
    break;
  }
  case AST_IF_STATEMENT: {
    IfStatement *is_stmt = (IfStatement *)node->data;
    if (is_stmt) {
      rewrite_generic_references(is_stmt->condition, ctx);
      rewrite_generic_references(is_stmt->then_branch, ctx);
      for (size_t i = 0; i < is_stmt->else_if_count; i++) {
        rewrite_generic_references(is_stmt->else_ifs[i].condition, ctx);
        rewrite_generic_references(is_stmt->else_ifs[i].body, ctx);
      }
      if (is_stmt->else_branch)
        rewrite_generic_references(is_stmt->else_branch, ctx);
    }
    break;
  }
  case AST_WHILE_STATEMENT: {
    WhileStatement *ws = (WhileStatement *)node->data;
    if (ws) {
      rewrite_generic_references(ws->condition, ctx);
      rewrite_generic_references(ws->body, ctx);
    }
    break;
  }
  case AST_FOR_STATEMENT: {
    ForStatement *fs = (ForStatement *)node->data;
    if (fs) {
      if (fs->initializer)
        rewrite_generic_references(fs->initializer, ctx);
      if (fs->condition)
        rewrite_generic_references(fs->condition, ctx);
      if (fs->increment)
        rewrite_generic_references(fs->increment, ctx);
      if (fs->body)
        rewrite_generic_references(fs->body, ctx);
    }
    break;
  }
  case AST_SWITCH_STATEMENT: {
    SwitchStatement *ss = (SwitchStatement *)node->data;
    if (ss) {
      rewrite_generic_references(ss->expression, ctx);
      for (size_t i = 0; i < ss->case_count; i++)
        rewrite_generic_references(ss->cases[i], ctx);
    }
    break;
  }
  case AST_CASE_CLAUSE: {
    CaseClause *cc = (CaseClause *)node->data;
    if (cc) {
      if (cc->value)
        rewrite_generic_references(cc->value, ctx);
      if (cc->body)
        rewrite_generic_references(cc->body, ctx);
    }
    break;
  }
  case AST_BINARY_EXPRESSION: {
    BinaryExpression *be = (BinaryExpression *)node->data;
    if (be) {
      rewrite_generic_references(be->left, ctx);
      rewrite_generic_references(be->right, ctx);
    }
    break;
  }
  case AST_UNARY_EXPRESSION: {
    UnaryExpression *ue = (UnaryExpression *)node->data;
    if (ue && ue->operand)
      rewrite_generic_references(ue->operand, ctx);
    break;
  }
  case AST_MEMBER_ACCESS: {
    MemberAccess *ma = (MemberAccess *)node->data;
    if (ma && ma->object)
      rewrite_generic_references(ma->object, ctx);
    break;
  }
  case AST_INDEX_EXPRESSION: {
    ArrayIndexExpression *ie = (ArrayIndexExpression *)node->data;
    if (ie) {
      if (ie->array)
        rewrite_generic_references(ie->array, ctx);
      if (ie->index)
        rewrite_generic_references(ie->index, ctx);
    }
    break;
  }
  case AST_DEFER_STATEMENT:
  case AST_ERRDEFER_STATEMENT: {
    DeferStatement *ds = (DeferStatement *)node->data;
    if (ds && ds->statement)
      rewrite_generic_references(ds->statement, ctx);
    break;
  }
  default:
    break;
  }
}

static GenericDef *find_generic_def(MonoContext *ctx, const char *name) {
  for (size_t i = 0; i < ctx->def_count; i++) {
    if (strcmp(ctx->defs[i].name, name) == 0)
      return &ctx->defs[i];
  }
  return NULL;
}

static ASTNode *create_monomorphized_struct(GenericDef *def,
                                            Instantiation *inst,
                                            MonoContext *ctx) {
  ASTNode *clone = ast_clone_node(def->node);
  if (!clone)
    return NULL;

  StructDeclaration *sd = (StructDeclaration *)clone->data;

  // Set the mangled name
  free(sd->name);
  sd->name = strdup(inst->mangled_name);

  // Clear type params (this is now a concrete type)
  for (size_t i = 0; i < sd->type_param_count; i++)
    free(sd->type_params[i]);
  free(sd->type_params);
  sd->type_params = NULL;
  sd->type_param_count = 0;

  // Substitute type params in field types
  for (size_t i = 0; i < sd->field_count; i++) {
    char *new_type = substitute_type_string(
        sd->field_types[i], def->type_params, inst->type_args,
        inst->type_arg_count, ctx);
    if (new_type) {
      free(sd->field_types[i]);
      sd->field_types[i] = new_type;
    }
  }

  return clone;
}

static ASTNode *create_monomorphized_function(GenericDef *def,
                                              Instantiation *inst,
                                              MonoContext *ctx) {
  ASTNode *clone = ast_clone_node(def->node);
  if (!clone)
    return NULL;

  FunctionDeclaration *fd = (FunctionDeclaration *)clone->data;

  // Set the mangled name
  free(fd->name);
  fd->name = strdup(inst->mangled_name);

  // Clear type params
  for (size_t i = 0; i < fd->type_param_count; i++)
    free(fd->type_params[i]);
  free(fd->type_params);
  fd->type_params = NULL;
  fd->type_param_count = 0;

  // Substitute type params in parameter types
  for (size_t i = 0; i < fd->parameter_count; i++) {
    char *new_type = substitute_type_string(
        fd->parameter_types[i], def->type_params, inst->type_args,
        inst->type_arg_count, ctx);
    if (new_type) {
      free(fd->parameter_types[i]);
      fd->parameter_types[i] = new_type;
    }
  }

  // Substitute type params in return type
  if (fd->return_type) {
    char *new_type = substitute_type_string(
        fd->return_type, def->type_params, inst->type_args,
        inst->type_arg_count, ctx);
    if (new_type) {
      free(fd->return_type);
      fd->return_type = new_type;
    }
  }

  // Substitute type params throughout the function body
  if (fd->body) {
    substitute_types_in_ast(fd->body, def->type_params, inst->type_args,
                            inst->type_arg_count, ctx);
  }

  return clone;
}

int monomorphize_program(ASTNode *program) {
  if (!program || program->type != AST_PROGRAM)
    return 1;

  Program *prog = (Program *)program->data;
  if (!prog)
    return 1;

  MonoContext ctx = {0};

  // Step 1: Collect generic definitions
  collect_generic_defs(program, &ctx);
  if (ctx.def_count == 0) {
    return 1; // No generics to process
  }

  // Step 2: Collect all instantiations from non-generic code
  collect_type_instantiations(program, &ctx);

  // Step 3: Generate monomorphized definitions, iterating until no new
  // instantiations are discovered (handles transitive generic usage).
  size_t processed = 0;
  while (processed < ctx.instance_count) {
    size_t current_count = ctx.instance_count;
    for (size_t i = processed; i < current_count; i++) {
      Instantiation *inst = &ctx.instances[i];
      GenericDef *def = find_generic_def(&ctx, inst->generic_name);
      if (!def)
        continue;

      ASTNode *mono_node = NULL;
      if (def->is_struct) {
        mono_node = create_monomorphized_struct(def, inst, &ctx);
      } else {
        mono_node = create_monomorphized_function(def, inst, &ctx);
      }

      if (mono_node) {
        prog->declarations =
            realloc(prog->declarations,
                    (prog->declaration_count + 1) * sizeof(ASTNode *));
        prog->declarations[prog->declaration_count] = mono_node;
        prog->declaration_count++;
        ast_add_child(program, mono_node);

        // Scan the new node for additional generic instantiations
        // (e.g., a monomorphized function calling another generic function)
        collect_type_instantiations(mono_node, &ctx);
      }
    }
    processed = current_count;
  }

  // Step 4: Rewrite all generic references to use mangled names
  rewrite_generic_references(program, &ctx);

  // Step 5: Remove generic (template) definitions from the program
  size_t write_idx = 0;
  for (size_t i = 0; i < prog->declaration_count; i++) {
    ASTNode *decl = prog->declarations[i];
    int is_generic = 0;

    if (decl && decl->type == AST_STRUCT_DECLARATION) {
      StructDeclaration *sd = (StructDeclaration *)decl->data;
      if (sd && sd->type_param_count > 0)
        is_generic = 1;
    } else if (decl && decl->type == AST_FUNCTION_DECLARATION) {
      FunctionDeclaration *fd = (FunctionDeclaration *)decl->data;
      if (fd && fd->type_param_count > 0)
        is_generic = 1;
    }

    if (!is_generic) {
      prog->declarations[write_idx++] = decl;
    }
  }
  prog->declaration_count = write_idx;

  // Clean up context
  for (size_t i = 0; i < ctx.def_count; i++) {
    free(ctx.defs[i].name);
    for (size_t j = 0; j < ctx.defs[i].type_param_count; j++)
      free(ctx.defs[i].type_params[j]);
    free(ctx.defs[i].type_params);
  }
  free(ctx.defs);

  for (size_t i = 0; i < ctx.instance_count; i++) {
    free(ctx.instances[i].generic_name);
    free(ctx.instances[i].mangled_name);
    for (size_t j = 0; j < ctx.instances[i].type_arg_count; j++)
      free(ctx.instances[i].type_args[j]);
    free(ctx.instances[i].type_args);
  }
  free(ctx.instances);

  return 1;
}
