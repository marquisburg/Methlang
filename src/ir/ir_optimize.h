#ifndef IR_OPTIMIZE_H
#define IR_OPTIMIZE_H

#include "ir.h"

// Runs optimization passes on the generated IR program.
// Currently implements:
// - Basic-block copy/constant propagation for temporaries
// - Integer constant/algebraic folding
// - Dead temporary write elimination for side-effect-free instructions
// - Constant branch simplification
// - Redundant jump cleanup
// - Straight-line unreachable code elimination
// Returns 1 on success, 0 on error.
int ir_optimize_program(IRProgram *program);

#endif // IR_OPTIMIZE_H
