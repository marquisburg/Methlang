#ifndef IR_H
#define IR_H

#include "../parser/ast.h"
#include "../semantic/symbol_table.h"
#include "../semantic/type_checker.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define IR_PROFILE_ID_NONE UINT32_MAX

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
  /* Vectorized idiom: count word starts in a byte buffer. Produced only by
   * ir_vectorize_simple_loops_pass when it recognizes the exact
   * "while (i<len){c=buf[i]; if(ws(c)) in_word=0; else {if(!in_word)count++;
   * in_word=1;} i++}" shape. Semantics: dest receives the number of maximal
   * non-whitespace runs in lhs[0..rhs-1] (whitespace = 0x20/0x09/0x0A/0x0D),
   * added to dest's prior value (the scalar code initializes count=0, so the
   * pass only matches when that holds). lhs = buffer base symbol, rhs = length
   * symbol/operand, dest = count symbol. Codegen lowers this to an SSE2
   * 16-bytes/iteration scan plus a scalar tail; see code_generator_ir.c. */
  IR_OP_COUNT_WORD_STARTS,
  /* Inline memory copy: dest = dst pointer, lhs = src pointer, rhs = byte count
   * (INT). Produced by ir_memcpy_inline_pass for constant-size memcpy calls. */
  IR_OP_MEMCPY_INLINE,
  /* Horizontal sum of int32 array into int64 accumulator. dest = sum symbol
   * (added to prior value), lhs = base pointer, rhs = element count. */
  IR_OP_SIMD_SUM_I32,
  /* Fixed 32x32 int32 matrix multiply. dest = c, lhs = a, rhs = b (pointers). */
  /* Reserved for an explicit 32x32 int32 SIMD matmul API. Do not introduce
   * this from ordinary source by function name or benchmark-shaped matching. */
  IR_OP_SIMD_MATMUL_N32,
  /* In-place signed int32 insertion sort. dest = base pointer, rhs = len. */
  IR_OP_SIMD_INSERTION_SORT_I32,
  /* Signed int32 dot product into int64. dest = sum/result, lhs = a, rhs = b,
   * arguments[0] = element count. */
  IR_OP_SIMD_DOT_I32,
  /* dst[i] = src[i]*mul+add; dest += sum of outputs. lhs=src, rhs=dst,
   * arguments[0]=len, [1]=mul, [2]=add (int32). */
  IR_OP_SIMD_SCALE_I32,
  /* dst[i] = clamp(src[i], lo, hi); dest += sum of outputs. lhs=src, rhs=dst,
   * arguments[0]=len, [1]=lo, [2]=hi (int32). */
  IR_OP_SIMD_CLAMP_I32,
  /* dst[i] = src[n-1-i]; dest += sum of outputs. lhs=src, rhs=dst,
   * arguments[0]=len. */
  IR_OP_SIMD_REVERSE_COPY_I32,
  /* Lower-bound index search over sorted int32 array:
   * dest=lo index result, lhs=arr, rhs=n, arguments[0]=key(int32). */
  IR_OP_LOWER_BOUND_I32,
  /* Inclusive int32 prefix sum: dst[i]=sum(src[0..i]) in int32, dest holds
   * int64 running sum. lhs=src, rhs=dst, arguments[0]=len. */
  IR_OP_PREFIX_SUM_I32,
  /* Min/max scan over arr[1..n-1] updating dest=minv and arguments[0]=maxv;
   * caller initializes both from arr[0]. lhs=arr, rhs=n. */
  IR_OP_SIMD_MINMAX_I32,
  /* Horizontal sum of a float64/float32 array into the dest float accumulator
   * (added to dest's prior value). lhs = base pointer, rhs = element count. */
  IR_OP_SIMD_SUM_F64,
  IR_OP_SIMD_SUM_F32,
  /* Float64/float32 dot product into the dest float accumulator (added to
   * dest's prior value). lhs = a, rhs = b, arguments[0] = element count. */
  IR_OP_SIMD_DOT_F64,
  IR_OP_SIMD_DOT_F32,
  /* Float affine memory map:
   * rhs[i] = arguments[1] * lhs[i] + arguments[2] * rhs[i] + arguments[3].
   * lhs = src, rhs = dst, arguments[0] = element count. */
  IR_OP_SIMD_AFFINE_MAP_F64,
  IR_OP_SIMD_AFFINE_MAP_F32
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
  const char *label;
  IRInstruction *instructions;
  size_t instruction_count;
  size_t first_instruction;
  size_t *successors;
  size_t successor_count;
  size_t *predecessors;
  size_t predecessor_count;
} IRBasicBlock;

typedef struct {
  char *name;
  uint32_t profile_id;
  char **parameter_names;
  char **parameter_types;
  size_t parameter_count;
  IRInstruction *instructions;
  size_t instruction_count;
  size_t instruction_capacity;
  IRBasicBlock *blocks;
  size_t block_count;
  size_t entry_block;
  int cfg_valid;
} IRFunction;

typedef struct {
  char *name;
  char *filename;
  uint64_t line;
} IRProfileEntry;

typedef struct {
  IRFunction **functions;
  size_t function_count;
  size_t function_capacity;
  IRProfileEntry *profile_entries;
  size_t profile_entry_count;
  size_t profile_entry_capacity;
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
IROperand ir_operand_copy(const IROperand *operand);
void ir_operand_destroy(IROperand *operand);

IRFunction *ir_function_create(const char *name);
int ir_function_set_parameters(IRFunction *function, const char **parameter_names,
                               const char **parameter_types,
                               size_t parameter_count);
void ir_function_destroy(IRFunction *function);
int ir_function_append_instruction(IRFunction *function,
                                   const IRInstruction *instruction);
int ir_function_insert_instruction(IRFunction *function, size_t index,
                                   const IRInstruction *instruction);
void ir_function_clear_cfg(IRFunction *function);
int ir_function_rebuild_cfg(IRFunction *function);
const IRBasicBlock *ir_function_blocks(IRFunction *function,
                                       size_t *block_count);

IRProgram *ir_program_create(void);
void ir_program_destroy(IRProgram *program);
int ir_program_add_function(IRProgram *program, IRFunction *function);

IRProgram *ir_lower_program(ASTNode *program, TypeChecker *type_checker,
                            SymbolTable *symbol_table, char **error_message,
                            int emit_runtime_checks);
int ir_program_dump(IRProgram *program, FILE *output);
int ir_instruction_dump(const IRInstruction *instruction,
                        char *buffer, size_t capacity);

#endif // IR_H
