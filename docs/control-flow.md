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

## Defer and Errdefer

`defer` schedules a statement to execute when the current scope exits, while `errdefer` schedules a statement to execute when returning a non-zero value from the current function. Both follow **LIFO (Last In, First Out)** ordering - the most recently deferred statement executes first.

### Syntax and Basic Behavior

Defer statements use the `defer` or `errdefer` keyword followed by a statement:

```masm
defer cleanup();          // Always runs on scope exit
errdefer rollback();      // Runs on non-zero return
```

The current compiler accepts function calls, assignments, and blocks:

```masm
defer puts("cleanup");
defer count = count + 1;
defer {
  flush();
  close(handle);
}
errdefer handle_error_recovery();
```

> **Variable capture pitfall:** Deferred statements capture variables **by reference**, not by value. In a loop, `defer print_int(i)` will read `i` when the defer runs—at the end of each iteration—so every deferred call sees the **final** value of `i` after the increment. This causes subtle bugs. Use a temporary: `var current: int32 = i; defer print_int(current);` to capture the value at defer time.

### Implementation Details

**AST Representation:**
- `defer` statements create `AST_DEFER_STATEMENT` nodes
- `errdefer` statements create `AST_ERRDEFER_STATEMENT` nodes
- Both contain a single `statement` field pointing to the deferred statement

**IR Lowering Process:**
1. **Stack Management:** Each scope has an `IRDeferStack` that tracks deferred statements
2. **Scope Hierarchy:** `IRDeferScope` structures form a linked list, allowing nested scopes
3. **Push Operation:** When encountering defer/errdefer, the compiler pushes the AST node onto the current scope's stack with an `is_err` flag
4. **Emission:** At scope exit, the compiler emits deferred statements in reverse order (LIFO)

**Data Structures:**
```c
typedef struct {
    ASTNode *node;    // The defer/errdefer AST node
    int is_err;       // 1 for errdefer, 0 for defer
} IRDeferEntry;

typedef struct {
    IRDeferEntry *entries;
    size_t count;
    size_t capacity;
} IRDeferStack;

typedef struct {
    IRDeferStack stack;
    struct IRDeferScope *parent;  // Link to outer scope
} IRDeferScope;
```

**Return Statement Handling:**
For functions with errdefer statements, the compiler generates two code paths:
1. **Error Path:** Emits both defer and errdefer statements
2. **Success Path:** Emits only defer statements

The return value is checked to determine which path to take, using generated labels like `errdefer_ok_N` and `errdefer_end_N`. This is convention-based: `0` means success and any non-zero return value is treated as an error, so `return 42;` also triggers `errdefer`.

**Control Flow Integration:**
- **Blocks:** Create new `IRDeferScope` with parent link to outer scope
- **If/Else:** Each branch gets its own defer scope; deferred statements run when branch exits
- **Loops:** Each iteration creates a new scope; deferred statements run at iteration end
- **Break/Continue:** Trigger deferred statement emission before jumping

The same success/error split is used for explicit `return` and for implicit fall-through at the end of a function body.

### LIFO Ordering and Execution

Deferred statements execute in reverse order of declaration. This is crucial for resource management where cleanup must happen in reverse of acquisition:

```masm
func example() {
  defer puts("first");    // Executes third
  defer puts("second");   // Executes second  
  defer puts("third");    // Executes first
  
  // Function body...
  // Output: "third", "second", "first"
}
```

**Mixed defer and errdefer:**
```masm
func mixed_example() {
  defer puts("always 1");
  errdefer puts("error only");
  defer puts("always 2");
  
  if (error_condition) {
    return err();  // Output: "always 2", "error only", "always 1"
  }
  
  return ok();     // Output: "always 2", "always 1"
}
```

### Scope-Level vs Function-Level Behavior

**Function scope:** defer/errdefer execute when the function returns via any path (return, break from main loop, etc.)

**Block scope:** defer/errdefer execute when the block exits, including if/else branches, loop bodies, and switch cases:

```masm
func demo() {
  defer puts("function exit");
  
  if (condition) {
    defer puts("if branch exit");  // Runs before function defer
    // ... branch code ...
  } else {
    defer puts("else branch exit");  // Runs before function defer
    // ... else code ...
  }
  
  // Output on condition=true: "if branch exit", "function exit"
  // Output on condition=false: "else branch exit", "function exit"
}
```

### Control Flow Integration

**Loops:** Each iteration gets its own defer scope. Deferred statements run at the end of each iteration. **Beware:** variables used in deferred statements are captured by reference (see pitfall above)—use a temporary if you need the value at defer time.

```masm
func loop_example() {
  defer puts("function cleanup");
  
  var i: int32 = 0;
  while (i < 3) {
    defer puts("iteration cleanup");  // Runs each iteration
    puts("iteration start");
    i = i + 1;
    
    if (i == 2) {
      break;  // Runs iteration defer, then function defer
    }
  }
  
  // Output: "iteration start", "iteration cleanup", 
  //         "iteration start", "iteration cleanup",
  //         "function cleanup"
}
```

**Switch statements:** Each case that creates a block gets its own defer scope:

```masm
func switch_demo(value: int32) {
  defer puts("function cleanup");
  
  switch (value) {
    case 1: {
      defer puts("case 1 cleanup");
      // ... case 1 code ...
    }
    case 2: {
      defer puts("case 2 cleanup");
      // ... case 2 code ...
    }
    default: {
      defer puts("default cleanup");
      // ... default code ...
    }
  }
  
  // Only one case's defer runs, plus function defer
}
```

Because `switch` allows fall-through, cleanup order becomes harder to reason about if execution crosses multiple case bodies. Prefer explicit `break` when a case owns deferred cleanup.

**Break and Continue:** These statements trigger deferred statement emission before jumping:

```masm
func control_flow_demo() {
  defer puts("function cleanup");
  
  while (1) {
    defer puts("iteration cleanup");
    
    if (early_exit) {
      break;  // Runs "iteration cleanup", then "function cleanup"
    }
    
    if (skip_iteration) {
      continue;  // Runs "iteration cleanup", then next iteration
    }
  }
}
```

### Error Handling Patterns

**Resource cleanup with error recovery:**
```masm
func process_file(filename: string) {
  var file: File* = fopen(filename, "r");
  if (file == 0) {
    return err();  // No defer to run yet
  }
  defer fclose(file);  // Always runs if file was opened
  
  var buffer: uint8* = malloc(4096);
  if (buffer == 0) {
    return err();  // Runs defer: fclose(file)
  }
  errdefer free(buffer);  // Only on error
  
  var data: string = read_file_content(file, buffer, 4096);
  if (data.length == 0) {
    return err();  // Runs errdefer: free(buffer), then defer: fclose(file)
  }
  
  // Process successful data...
  return ok();  // Runs only defer: fclose(file)
}
```

**Nested error handling:**
```masm
func nested_operations() {
  defer puts("outer cleanup");
  
  var resource1: Resource* = acquire_resource();
  if (resource1 == 0) {
    return err();
  }
  defer release_resource(resource1);
  
  {
    defer puts("inner cleanup");
    
    var resource2: Resource* = acquire_resource();
    if (resource2 == 0) {
      return err();  // Runs "inner cleanup", "release_resource(resource1)", "outer cleanup"
    }
    defer release_resource(resource2);
    
    if (processing_error) {
      return err();  // Runs "release_resource(resource2)", "inner cleanup", 
                   // "release_resource(resource1)", "outer cleanup"
    }
    
    // Success path...
    return ok();  // Runs "release_resource(resource2)", "inner cleanup", 
                   // "release_resource(resource1)", "outer cleanup"
  }
}
```

### Common Pitfalls and Limitations

**Top-level defer:** defer/errdefer can only be used inside functions:

```masm
// ERROR: defer outside function
defer puts("this fails");

func valid_function() {
  defer puts("this works");  // OK
}
```

**Supported deferred statements:** `defer` and `errdefer` currently support function calls, assignments, and blocks:

```masm
func example() {
  defer close_file(file);    // OK
  errdefer update_value(x);  // OK
  defer x = 1;               // OK
  errdefer {
    x = x + 1;
    update_value(x);
  }
}
```

**Variable capture:** Deferred statements capture variables by reference, not value (see the warning callout above). In a loop, `defer print_int(i)` reads `i` when the defer runs, so you get the value at scope exit—not at defer declaration time. Use a temporary so each iteration has its own variable:

```masm
while (i < 3) {
  var current: int32 = i;
  defer print_int(current);  // current holds the value from start of iteration
  i = i + 1;
}
```

**Performance considerations:** Each defer statement adds runtime overhead for stack management and conditional execution. In performance-critical code, consider manual cleanup for simple cases.

### Resource Management Patterns

**File handling with multiple resources:**
```masm
func copy_file(src: string, dst: string) {
  var src_file: File* = fopen(src, "r");
  if (src_file == 0) {
    return err();
  }
  defer fclose(src_file);
  
  var dst_file: File* = fopen(dst, "w");
  if (dst_file == 0) {
    return err();
  }
  defer fclose(dst_file);  // Runs first (LIFO)
  
  var buffer: uint8* = malloc(4096);
  if (buffer == 0) {
    return err();
  }
  errdefer free(buffer);
  
  // Copy loop...
  while (!feof(src_file)) {
    var bytes: int32 = fread(buffer, 1, 4096, src_file);
    if (bytes <= 0) {
      return err();  // Free buffer, close dst_file, close src_file
    }
    fwrite(buffer, 1, bytes, dst_file);
  }
  
  free(buffer);  // Manual cleanup before success return
  return ok();     // Close dst_file, close src_file
}
```

**Socket management in servers:**
```masm
func handle_client_connection(client_socket: int32) {
  defer close_socket(client_socket);
  
  // Set socket options
  if (set_socket_options(client_socket) != 0) {
    return err();  // Runs defer: close_socket(client_socket)
  }
  
  var buffer: uint8* = malloc(8192);
  if (buffer == 0) {
    return err();
  }
  errdefer free(buffer);
  
  // Read request loop
  while (1) {
    var bytes: int32 = recv(client_socket, buffer, 8192, 0);
    if (bytes <= 0) {
      break;  // Client disconnected or error
    }
    
    if (process_request(buffer, bytes) != 0) {
      return err();  // Free buffer, close socket
    }
  }
  
  return ok();  // Free buffer, close socket
}
```

**Memory allocation chains:**
```masm
func complex_allocation_chain() {
  var resource1: Resource* = allocate_resource();
  if (resource1 == 0) {
    return err();
  }
  defer free_resource(resource1);
  
  var resource2: Resource* = allocate_resource();
  if (resource2 == 0) {
    return err();
  }
  defer free_resource(resource2);
  
  var temp_buffer: uint8* = malloc(1024);
  if (temp_buffer == 0) {
    return err();
  }
  errdefer free(temp_buffer);  // Only on error
  
  if (complex_processing(resource1, resource2, temp_buffer) != 0) {
    return err();  // Free temp_buffer, resource2, resource1
  }
  
  // Success: manually clean up temp_buffer
  free(temp_buffer);
  return ok();     // Free resource2, resource1
}
```

## Unreachable Code
The compiler emits a warning for unreachable statements that appear after an unconditional `return`, `break`, or `continue` in the same block.
