#ifndef IR_OPTIMIZE_H
#define IR_OPTIMIZE_H

#include "ir.h"

typedef struct {
  /* Reserved for future IR optimization controls. */
  int preserve_function_boundaries;
} IROptimizeOptions;

// Runs optimization passes on the generated IR program.
// Currently implements:
// - Small-function inlining (including control flow, no calls in callee)
// - Copy/constant propagation for temporaries and stack locals
// - Fibonacci-style rotate_add fusion and loop-body rotate fusion
// - Small constant-bound counted-loop unrolling (<= 64 trips)
// - Integer constant/algebraic folding and strength reduction
// - CSE, dead temp elimination, branch/jump CFG cleanup
// Returns 1 on success, 0 on error.
int ir_optimize_program(IRProgram *program,
                        const IROptimizeOptions *options);

#endif // IR_OPTIMIZE_H
