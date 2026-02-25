#ifndef CODE_GENERATOR_INTERNAL_H
#define CODE_GENERATOR_INTERNAL_H

#include "code_generator.h"

void code_generator_set_error(CodeGenerator *generator, const char *format, ...);
void code_generator_emit_to_global_buffer(CodeGenerator *generator,
                                          const char *format, ...);
void code_generator_register_function_parameters(
    CodeGenerator *generator, FunctionDeclaration *func_data,
    int parameter_home_size);
int code_generator_generate_function_from_ir(CodeGenerator *generator,
                                             ASTNode *function_declaration,
                                             IRFunction *ir_function);

#endif // CODE_GENERATOR_INTERNAL_H
