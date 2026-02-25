#ifndef IR_OPTIMIZE_H
#define IR_OPTIMIZE_H

#include "ir.h"

// Runs optimization passes on the generated IR program.
// Currently implements:
// - Simple Copy Propagation
// Returns 1 on success, 0 on error.
int ir_optimize_program(IRProgram *program);

#endif // IR_OPTIMIZE_H
