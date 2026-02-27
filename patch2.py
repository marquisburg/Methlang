import os

filepath = r"g:\Projects\MethASM\src\ir\ir_lowering.c"
with open(filepath, "r") as f:
    content = f.read()

target = """    IROperand left = ir_operand_none();
    IROperand right = ir_operand_none();
    if (!ir_lower_expression(context, function, binary->left, &left) ||
        !ir_lower_expression(context, function, binary->right, &right)) {"""

repl = """    Type *expr_type = ir_infer_expression_type(context, expression);
    if (expr_type && expr_type->kind == TYPE_STRING && strcmp(binary->operator, "+") == 0) {
      IROperand destination = ir_operand_none();
      if (!ir_make_temp_operand(context, &destination)) {
        return 0;
      }
      IRInstruction instruction = {0};
      instruction.op = IR_OP_BINARY;
      instruction.location = expression->location;
      instruction.dest = destination;
      instruction.text = binary->operator;
      instruction.ast_ref = expression;
      
      if (!ir_emit(context, function, &instruction)) {
        ir_operand_destroy(&destination);
        return 0;
      }
      *out_value = destination;
      return 1;
    }

    IROperand left = ir_operand_none();
    IROperand right = ir_operand_none();
    if (!ir_lower_expression(context, function, binary->left, &left) ||
        !ir_lower_expression(context, function, binary->right, &right)) {"""

if target in content:
    content = content.replace(target, repl)
    with open(filepath, "w") as f:
        f.write(content)
    print("Patched ir_lowering.c successfully!")
else:
    print("Target not found.")
