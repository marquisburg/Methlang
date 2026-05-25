#ifndef REGISTER_ALLOCATOR_H
#define REGISTER_ALLOCATOR_H

#include "symbol_table.h"

typedef enum {
    REG_RAX, REG_RBX, REG_RCX, REG_RDX,
    REG_RSI, REG_RDI, REG_RSP, REG_RBP,
    REG_R8,  REG_R9,  REG_R10, REG_R11,
    REG_R12, REG_R13, REG_R14, REG_R15,
    REG_XMM0, REG_XMM1, REG_XMM2, REG_XMM3,
    REG_XMM4, REG_XMM5, REG_XMM6, REG_XMM7,
    REG_XMM8, REG_XMM9, REG_XMM10, REG_XMM11,
    REG_XMM12, REG_XMM13, REG_XMM14, REG_XMM15,
    REG_NONE
} x86Register;

typedef enum {
    CALLING_CONV_SYSV,
    CALLING_CONV_MS_X64
} CallingConvention;

typedef enum {
    REG_CLASS_CALLER_SAVED,
    REG_CLASS_CALLEE_SAVED,
    REG_CLASS_PARAMETER,
    REG_CLASS_RETURN,
    REG_CLASS_RESERVED
} RegisterClass;

typedef struct {
    CallingConvention convention;

    x86Register* int_param_registers;
    size_t int_param_count;
    x86Register* float_param_registers;
    size_t float_param_count;

    x86Register int_return_register;
    x86Register float_return_register;

    x86Register* caller_saved_registers;
    size_t caller_saved_count;

    x86Register* callee_saved_registers;
    size_t callee_saved_count;

    int stack_alignment;
    int shadow_space_size;
} CallingConventionSpec;

typedef struct {
    CallingConventionSpec* calling_convention;
} RegisterAllocator;

RegisterAllocator* register_allocator_create(void);
void register_allocator_destroy(RegisterAllocator* allocator);

void register_allocator_set_calling_convention(RegisterAllocator* allocator,
                                               CallingConvention convention);
CallingConventionSpec* register_allocator_get_calling_convention_spec(
    CallingConvention convention);
void register_allocator_destroy_calling_convention_spec(
    CallingConventionSpec* spec);

RegisterClass register_allocator_get_register_class(CallingConventionSpec* spec,
                                                    x86Register reg);
int register_allocator_is_caller_saved(CallingConventionSpec* spec,
                                       x86Register reg);
int register_allocator_is_callee_saved(CallingConventionSpec* spec,
                                       x86Register reg);
int register_allocator_is_floating_point_type(Type* type);
const char* register_allocator_register_name(x86Register reg);

#endif // REGISTER_ALLOCATOR_H
