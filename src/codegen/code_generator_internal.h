#ifndef CODE_GENERATOR_INTERNAL_H
#define CODE_GENERATOR_INTERNAL_H

#include "code_generator.h"

void code_generator_set_error(CodeGenerator *generator, const char *format, ...);
void code_generator_emit_to_global_buffer(CodeGenerator *generator,
                                          const char *format, ...);

#endif // CODE_GENERATOR_INTERNAL_H
