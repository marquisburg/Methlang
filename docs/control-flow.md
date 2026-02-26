# Control Flow

MethASM provides structured control flow: conditionals, loops, and switches. All control structures use braces for the body.

## Assignment

Assignment uses `=`. The left side must be an lvalue (variable, struct field, array element, or dereferenced pointer). Assignment is a statement; it does not produce a value for use in larger expressions.

```masm
x = 42;
ptr->field = value;
arr[i] = x;
```

**Compound assignment** (`+=`, `-=`, `*=`, `/=`) is not supported. Use explicit forms: `x = x + 1` instead of `x += 1`.

**Type mismatches** produce a compile error. Assigning a value of incompatible type (e.g. `x = 3.14` where `x` is `int32`) is rejected; the compiler does not silently truncate.

## If and Else

The `if` statement evaluates a condition. If true, the then branch runs. The optional `else` branch runs when the condition is false. The condition must be a **numeric type** (integer or floating-point); zero is false, non-zero is true. Pointers are not valid as conditions—use an explicit comparison: `if (ptr != 0)` to check for non-null, not `if (ptr)`.

```masm
if (x > 0) {
  // ...
} else if (x < 0) {
  // `else if` is parsed as part of the if statement
} else {
  // ...
}
```

`else if` chaining is fully supported as a contiguous sequence of conditions, avoiding deep AST nesting. There is no separate `elseif` keyword.

## While

The `while` loop evaluates the condition. If true, the body runs and the condition is evaluated again. The loop exits when the condition is false.

```masm
while (condition) {
  // ...
}
```

Common patterns:

```masm
// Iterate over an array
var i: int32 = 0;
while (i < len) {
  arr[i] = arr[i] * 2;
  i = i + 1;
}

// Infinite loop (e.g. accept loop in a server)
while (1) {
  // ...
}
```

An infinite loop is written `while (1)`; the condition is always true.

## For

The `for` loop has an initializer, condition, and increment. The initializer runs once. The condition is evaluated before each iteration; if false, the loop exits. The increment runs after each iteration. The initializer can declare a variable. Condition and increment are optional—`for (;;)` is a valid infinite loop.

```masm
for (var i: int32 = 0; i < 10; i = i + 1) {
  // ...
}
```

**Scope:** A variable declared in the initializer (e.g. `var i`) is scoped to the loop. It is not accessible after the loop exits.

**Infinite loop:** Use `for (;;)` when all three parts are omitted. This is idiomatic in systems code.

## Switch

The `switch` statement evaluates an expression and compares it to each `case` value. Case values must be compile-time constant integer expressions (including enum variants). When a case matches, its body runs. Use `break` to exit the switch. Use `continue` inside a loop that contains the switch to continue the loop. Only one `default` clause is allowed.

**Fall-through:** Unlike some languages, MethASM does not enforce `break`. If you omit it, execution falls through to the next case (C-style behavior). To avoid accidental bugs, always end each case with `break` explicitly unless you intend fall-through.

**No case matches, no default:** If no case matches and there is no `default` clause, execution continues silently after the switch. No error is raised.

```masm
switch (expr) {
  case 1:
    // ...
    break;
  case 2:
    // ...
    break;
  default:
    // ...
}
```

## Break and Continue

`break` exits the innermost loop or switch. `continue` skips to the next iteration of the innermost loop. Both are context-checked; they are valid only inside loops or switches. Using them elsewhere is a compile error.

**Important:** `break` and `continue` always target the **innermost** enclosing loop or switch. Inside nested loops, `break` exits only the inner loop. Inside a `switch` that is inside a loop, `break` exits the switch, not the loop—use `continue` to skip to the next loop iteration.

```masm
while (1) {
  switch (cmd) {
    case 0:
      break;      // exits switch only, loop continues
    case 1:
      continue;   // skips to next loop iteration (exits switch and continues loop)
    case 2:
      break;      // exits switch
  }
  // ...
}
```

**Labeled break/continue** (e.g. `break outer` to exit an outer loop) is not supported. To break out of nested loops, use a flag or restructure the code.

## Return

`return` exits the current function. A function with a return type must provide a value: `return value`. A void function uses `return` with no value.

```masm
return;
return value;
```

## Short-Circuit Evaluation

Logical operators `&&` and `||` support short-circuit evaluation. For pointer checks like `ptr != 0 && ptr->field > 0`, a single condition is safe:

```masm
if (ptr != 0 && ptr->field > 0) {
  // ...
}
```

## Unreachable Code

The compiler emits a warning for unreachable statements that appear after an unconditional `return`, `break`, or `continue` in the same block.
