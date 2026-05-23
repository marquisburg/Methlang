#ifndef IR_PROFILE_H
#define IR_PROFILE_H

#include "ir.h"

/* Insert mettle_profile_enter/exit IR calls at function boundaries. Inlining
 * duplicates these hooks at each call site; inline sites get distinct profile
 * ids via ir_profile_register_inline_site. */
int ir_profile_instrument_program(IRProgram *program);
int ir_profile_instrument_operation_counters(IRProgram *program);

uint32_t ir_profile_registry_add(IRProgram *program, const char *name,
                                 const char *filename, uint64_t line);

uint32_t ir_profile_register_inline_site(IRProgram *program,
                                         const char *callee_name,
                                         size_t inline_site_id,
                                         SourceLocation call_site);

int ir_profile_instruction_is_enter(const IRInstruction *instruction,
                                    uint32_t *profile_id_out);

int ir_profile_build_enter_instruction(IRInstruction *instruction,
                                       uint32_t profile_id,
                                       SourceLocation location);

#endif /* IR_PROFILE_H */
