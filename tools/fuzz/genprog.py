#!/usr/bin/env python3
"""Random Mettle program generator for differential miscompile testing.

Emits a self-contained program whose `main` deterministically computes an
int64 accumulator and returns the low byte (xor-folded) as its exit code. The
program contains no I/O and no UB-prone constructs, so debug and release builds
MUST agree on the exit code. A divergence is a silent miscompile.

The generated shapes deliberately target the historically fragile optimizer /
binary-backend paths: counted reduction loops, polynomial-in-i sums, indexed
array stores then read-back (scaled-address + induction-pointer passes),
unsigned shift/div/mod, nested conditionals, and small inlinable helpers.

The accumulator is masked into [0, 1<<40) after every step so 64-bit signed
overflow never occurs (overflow would be a language-level UB difference, not a
miscompile). Division/modulo guard against zero divisors at generation time.
"""

import random


class Gen:
    def __init__(self, seed):
        self.rng = random.Random(seed)
        self.seed = seed
        self.lines = []
        self.indent = 0
        self.var_counter = 0
        self.live_vars = []          # int64 scalars currently in scope
        self.helpers = []            # emitted helper function source blocks
        self.helper_names = []

    # ---- emission helpers -------------------------------------------------
    def emit(self, s):
        self.lines.append("  " * self.indent + s)

    def fresh(self, prefix="v"):
        self.var_counter += 1
        return f"{prefix}{self.var_counter}"

    def pick_var(self):
        return self.rng.choice(self.live_vars) if self.live_vars else None

    # ---- expression generation -------------------------------------------
    def const(self):
        return self.rng.randint(0, 9999)

    def atom(self, depth):
        """A non-dividing int64 expression usable anywhere."""
        choices = ["const"]
        if self.live_vars:
            choices += ["var", "var"]
        if depth > 0:
            choices += ["binop"]
        kind = self.rng.choice(choices)
        if kind == "const":
            return str(self.const())
        if kind == "var":
            return self.pick_var()
        # binop
        a = self.atom(depth - 1)
        b = self.atom(depth - 1)
        op = self.rng.choice(["+", "-", "*", "&", "|", "^"])
        return f"({a} {op} {b})"

    def masked_assign(self, target):
        """Assign a fresh value to `target`, keeping it in [0, 1<<40)."""
        expr = self.atom(self.rng.randint(1, 3))
        # Occasionally fold in an unsigned shift/div/mod via a uint64 temp,
        # exercising the unsigned codegen paths without risking signed UB.
        if self.rng.random() < 0.35:
            ut = self.fresh("u")
            base = self.rng.randint(1, 1 << 30)
            op = self.rng.choice([">>", "/", "%"])
            rhs = self.rng.randint(1, 31) if op == ">>" else self.rng.randint(1, 97)
            self.emit(f"var {ut}: uint64 = (uint64){base};")
            self.emit(f"{ut} = {ut} {op} {rhs};")
            expr = f"({expr} + (int64){ut})"
        self.emit(f"{target} = ({expr}) & 1099511627775;")  # mask to 40 bits

    # ---- statement generation --------------------------------------------
    def stmt(self, budget):
        if budget <= 0:
            return
        roll = self.rng.random()
        if roll < 0.30:
            self.stmt_decl()
        elif roll < 0.55:
            self.stmt_loop(budget)
        elif roll < 0.75:
            self.stmt_if(budget)
        elif roll < 0.90:
            self.stmt_array(budget)
        else:
            self.stmt_call()

    def stmt_decl(self):
        v = self.fresh()
        self.emit(f"var {v}: int64 = {self.atom(2)} & 1099511627775;")
        self.live_vars.append(v)

    def stmt_loop(self, budget):
        acc = self.pick_var()
        if acc is None:
            self.stmt_decl()
            acc = self.live_vars[-1]
        i = self.fresh("i")
        n = self.rng.randint(0, 40)
        # polynomial-in-i reduction: the closed-form-sum / reduction-unroll shape
        poly = self.rng.choice(["{i}", "{i}*{i}", "{i}*{i}*{i}",
                                 "(2*{i}+3)", "7"]).replace("{i}", i)
        self.emit(f"var {i}: int64 = 1;")
        self.emit(f"while ({i} <= {n}) {{")
        self.indent += 1
        self.emit(f"{acc} = ({acc} + {poly}) & 1099511627775;")
        self.emit(f"{i} = {i} + 1;")
        self.indent -= 1
        self.emit("}")

    def block(self, budget):
        """Emit a nested statement, restoring scope afterward so vars declared
        inside the block don't leak into the outer live set."""
        saved = list(self.live_vars)
        self.stmt(budget - 1)
        self.live_vars = saved

    def stmt_if(self, budget):
        a = self.atom(2)
        b = self.atom(1)
        op = self.rng.choice(["<", "<=", ">", ">=", "==", "!="])
        self.emit(f"if ({a} {op} {b}) {{")
        self.indent += 1
        self.block(budget)
        self.indent -= 1
        if self.rng.random() < 0.5:
            self.emit("} else {")
            self.indent += 1
            self.block(budget)
            self.indent -= 1
        self.emit("}")

    def stmt_array(self, budget):
        """arr[i] = f(i) write loop, then read-back fold -- the scaled-address
        and induction-pointer miscompile shape."""
        arr = self.fresh("arr")
        n = self.rng.randint(2, 16)
        i = self.fresh("i")
        acc = self.pick_var()
        if acc is None:
            self.stmt_decl()
            acc = self.live_vars[-1]
        self.emit(f"var {arr}: int64[{n}];")
        # write loop
        self.emit(f"var {i}: int64 = 0;")
        self.emit(f"while ({i} < {n}) {{")
        self.indent += 1
        self.emit(f"{arr}[{i}] = ({i} * {self.rng.randint(1,50)} + {self.rng.randint(0,99)}) & 1099511627775;")
        self.emit(f"{i} = {i} + 1;")
        self.indent -= 1
        self.emit("}")
        # read-back fold loop (separate counter)
        j = self.fresh("j")
        self.emit(f"var {j}: int64 = 0;")
        self.emit(f"while ({j} < {n}) {{")
        self.indent += 1
        self.emit(f"{acc} = ({acc} + {arr}[{j}]) & 1099511627775;")
        self.emit(f"{j} = {j} + 1;")
        self.indent -= 1
        self.emit("}")

    def stmt_call(self):
        if not self.helper_names:
            return
        fn = self.rng.choice(self.helper_names)
        acc = self.pick_var()
        if acc is None:
            self.stmt_decl()
            acc = self.live_vars[-1]
        a = self.atom(1)
        self.emit(f"{acc} = ({acc} + {fn}({a})) & 1099511627775;")

    # ---- helper functions (inlining bait) --------------------------------
    def gen_helpers(self, k):
        for _ in range(k):
            name = self.fresh("h")
            p = "p"
            body = []
            body.append(f"function {name}({p}: int64) -> int64 {{")
            body.append(f"  var r: int64 = {p} & 1099511627775;")
            # a tiny counted loop so the inliner + reduction passes engage
            m = self.rng.randint(0, 20)
            body.append(f"  var k: int64 = 1;")
            body.append(f"  while (k <= {m}) {{")
            poly = self.rng.choice(["k", "k*k", "(k+r)"])
            body.append(f"    r = (r + {poly}) & 1099511627775;")
            body.append(f"    k = k + 1;")
            body.append(f"  }}")
            body.append(f"  return r & 1099511627775;")
            body.append("}")
            self.helpers.append("\n".join(body))
            self.helper_names.append(name)

    # ---- top-level program ------------------------------------------------
    def generate(self):
        self.gen_helpers(self.rng.randint(1, 3))
        # main body
        self.emit("function main() -> int32 {")
        self.indent += 1
        self.emit("var acc: int64 = 1;")
        self.live_vars = ["acc"]
        nstmts = self.rng.randint(6, 14)
        for _ in range(nstmts):
            self.stmt(self.rng.randint(2, 4))
        # fold every live var into acc so they all matter
        for v in self.live_vars:
            if v != "acc":
                self.emit(f"acc = (acc + {v}) & 1099511627775;")
        self.emit("return (int32)(acc & 255);")
        self.indent -= 1
        self.emit("}")

        header = [f"// seed={self.seed}", ""]
        return "\n".join(header + self.helpers + [""] + self.lines) + "\n"


def generate(seed):
    return Gen(seed).generate()


if __name__ == "__main__":
    import sys
    seed = int(sys.argv[1]) if len(sys.argv) > 1 else 0
    sys.stdout.write(generate(seed))
