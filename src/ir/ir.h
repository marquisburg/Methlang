#ifndef IR_H
#define IR_H

#include "../parser/ast.h"
#include "../semantic/symbol_table.h"
#include "../semantic/type_checker.h"
#include <stddef.h>
#include <stdio.h>

typedef enum {
  IR_OPERAND_NONE,
  IR_OPERAND_TEMP,
  IR_OPERAND_SYMBOL,
  IR_OPERAND_INT,
  IR_OPERAND_FLOAT,
  IR_OPERAND_STRING,
  IR_OPERAND_LABEL
} IROperandKind;

typedef struct {
  IROperandKind kind;
  char *name;
  long long int_value;
  double float_value;
  /* IEEE-754 width of a floating operand: 32 or 64. 0 means "not a float /
   * unspecified" and callers must treat it as the default double width (64).
   * Carried so backends never have to re-derive single vs double precision
   * from scattered symbol/type lookups. */
  int float_bits;
} IROperand;

typedef enum {
  IR_OP_NOP,
  IR_OP_LABEL,
  IR_OP_JUMP,
  IR_OP_BRANCH_ZERO,
  IR_OP_BRANCH_EQ,
  IR_OP_DECLARE_LOCAL,
  IR_OP_ASSIGN,
  IR_OP_ADDRESS_OF,
  IR_OP_LOAD,
  IR_OP_STORE,
  IR_OP_BINARY,
  IR_OP_UNARY,
  /* Fibonacci-style rotate: dest=next, lhs=a, rhs=b => next=a+b; a=b; b=next */
  IR_OP_ROTATE_ADD,
  IR_OP_CALL,
  IR_OP_CALL_INDIRECT,
  IR_OP_NEW,
  IR_OP_RETURN,
  IR_OP_INLINE_ASM,
  IR_OP_CAST,
  // Threading opcodes
  IR_OP_THREAD_SPAWN,   // dest = spawn fn(args...)  — lhs=fn name (text), args=arguments
  IR_OP_THREAD_JOIN,    // dest = join(thread_handle) — lhs=handle operand
  IR_OP_MUTEX_NEW,      // dest = Mutex.new()
  IR_OP_MUTEX_LOCK,     // dest = mutex.lock()  — returns Guard, lhs=mutex operand
  IR_OP_MUTEX_UNLOCK,   // mutex.unlock(guard) — lhs=guard operand
  IR_OP_ATOMIC_LOAD,    // dest = atomic.load() — lhs=atomic operand
  IR_OP_ATOMIC_STORE,   // atomic.store(val)    — lhs=atomic operand, rhs=value
  IR_OP_ATOMIC_FETCH_ADD, // dest = atomic.fetch_add(val) — lhs=atomic, rhs=delta
  IR_OP_ATOMIC_FETCH_SUB,
  IR_OP_ATOMIC_CAS,     // dest=old, lhs=atomic, rhs=expected, arguments[0]=desired
  IR_OP_CHAN_NEW,        // dest = channel(cap)  — lhs=capacity (INT, 0=unbounded)
  IR_OP_CHAN_SEND,       // chan.send(val)        — lhs=chan operand, rhs=value
  IR_OP_CHAN_RECV        // dest = chan.recv()    — lhs=chan operand
} IROpcode;

typedef struct {
  IROpcode op;
  SourceLocation location;
  IROperand dest;
  IROperand lhs;
  IROperand rhs;
  char *text;
  IROperand *arguments;
  size_t argument_count;
  int is_float;
  /* Width of the floating result when is_float is set: 32 or 64. 0 means
   * unspecified and is treated as 64 (double) for backward compatibility with
   * code paths that only ever produced float64. */
  int float_bits;
  ASTNode *ast_ref;
} IRInstruction;

typedef struct {
  char *name;
  char **parameter_names;
  char **parameter_types;
  size_t parameter_count;
  IRInstruction *instructions;
  size_t instruction_count;
  size_t instruction_capacity;
} IRFunction;

typedef struct {
  IRFunction **functions;
  size_t function_count;
  size_t function_capacity;
} IRProgram;

IROperand ir_operand_none(void);
IROperand ir_operand_temp(const char *name);
IROperand ir_operand_symbol(const char *name);
IROperand ir_operand_int(long long value);
/* Defaults float_bits to 64 (double) for backward compatibility. */
IROperand ir_operand_float(double value);
/* Like ir_operand_float but tags the IEEE-754 width (32 or 64). Any other
 * value is normalized to 64. */
IROperand ir_operand_float_sized(double value, int float_bits);
IROperand ir_operand_string(const char *value);
IROperand ir_operand_label(const char *name);
void ir_operand_destroy(IROperand *operand);

IRFunction *ir_function_create(const char *name);
int ir_function_set_parameters(IRFunction *function, const char **parameter_names,
                               const char **parameter_types,
                               size_t parameter_count);
void ir_function_destroy(IRFunction *function);
int ir_function_append_instruction(IRFunction *function,
                                   const IRInstruction *instruction);

IRProgram *ir_program_create(void);
void ir_program_destroy(IRProgram *program);
int ir_program_add_function(IRProgram *program, IRFunction *function);

IRProgram *ir_lower_program(ASTNode *program, TypeChecker *type_checker,
                            SymbolTable *symbol_table, char **error_message,
                            int emit_runtime_checks);
int ir_program_dump(IRProgram *program, FILE *output);

#endif // IR_H
