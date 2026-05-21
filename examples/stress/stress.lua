--[[
  Lua 5.5 stress / benchmark suite (interpreted baseline vs Mettle/C).

  Matches workloads from examples/fib, collatz, sum_squares:
    - fib:        fib(35) x 10M (looped, same as fib.c)
    - fib-stress: fully unrolled fib(35) body x 10M (same as fib.mettle bench_unrolled)
    - collatz:    sum steps 1..100000 x 10 passes
    - sum_squares: sum 1..100000² x 500 passes

  Usage:
    lua55 stress.lua
    lua55 stress.lua fib-stress
    lua55 stress.lua all

  Harness parsers accept:  Time: <N> us
]]

local function script_dir()
  local src = debug.getinfo(1, "S").source
  if src:sub(1, 1) == "@" then
    src = src:sub(2)
  end
  local dir = src:match("(.*[/\\])")
  if not dir then
    error("could not resolve stress.lua directory")
  end
  return dir
end

local bench = dofile(script_dir() .. "../bench_time.lua")

local bench_time_us = bench.bench_time_us

local function fib(n)
  if n <= 1 then
    return n
  end
  local a, b = 0, 1
  for i = 2, n do
    local next = a + b
    a = b
    b = next
  end
  return b
end

-- One fib(35) iteration with the loop unrolled in source (Mettle bench_unrolled).
local function fib35_unrolled()
  local a, b, next = 0, 1, 0
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  next = a + b; a = b; b = next
  return b
end

local function bench_fib_looped()
  local iter = 10000000
  local check = fib(35)
  if fib(35) ~= check then
    error("fib self-check failed")
  end

  local t0 = bench_time_us()
  local bench_sum = 0
  for j = 1, iter do
    bench_sum = bench_sum + fib(35)
  end
  local elapsed_us = bench_time_us() - t0

  print("Benchmark: fib(35) x 10,000,000 (looped, vs fib.c)")
  print(string.format("fib(35) = %d", check))
  print(string.format("Bench sum mod check = %d", bench_sum % check))
  print(string.format("Time: %d us", elapsed_us))
  print(string.format("Per call: ~%d ns", math.floor(elapsed_us * 1000 / iter)))
end

local function bench_fib_stress()
  local iter = 10000000
  local check = fib35_unrolled()
  if fib(35) ~= check then
    error("fib-stress self-check failed")
  end

  local t0 = bench_time_us()
  local bench_sum = 0
  for j = 1, iter do
    bench_sum = bench_sum + fib35_unrolled()
  end
  local elapsed_us = bench_time_us() - t0

  print("Benchmark: fib(35) x 10,000,000 (unrolled stress, vs fib.mettle bench_unrolled)")
  print(string.format("fib(35) = %d", check))
  print(string.format("Bench sum mod check = %d", bench_sum % check))
  print(string.format("Time: %d us", elapsed_us))
  print(string.format("Per call: ~%d ns", math.floor(elapsed_us * 1000 / iter)))
end

local function collatz_steps(n)
  local count = 0
  local x = n
  while x > 1 do
    if x % 2 == 0 then
      x = x // 2
    else
      x = 3 * x + 1
    end
    count = count + 1
  end
  return count
end

local function bench_collatz()
  local passes = 10
  local t0 = bench_time_us()
  local bench_sum = 0
  for p = 1, passes do
    for i = 1, 100000 do
      bench_sum = bench_sum + collatz_steps(i)
    end
  end
  local elapsed_us = bench_time_us() - t0

  print("Benchmark: 10 passes (Collatz steps 1..100000)")
  print(string.format("Bench sum = %d", bench_sum))
  print(string.format("Time: %d us", elapsed_us))
  print(string.format("Per pass: ~%d us", math.floor(elapsed_us / passes)))
end

local function sum_squares(n)
  local sum = 0
  for i = 1, n do
    sum = sum + i * i
  end
  return sum
end

local function bench_sum_squares()
  local passes = 500
  local n = 100000
  local t0 = bench_time_us()
  local bench_sum = 0
  for p = 1, passes do
    bench_sum = bench_sum + sum_squares(n)
  end
  local elapsed_us = bench_time_us() - t0

  print("Benchmark: 500 passes (sum_squares 100000)")
  print(string.format("Bench sum = %d", bench_sum))
  print(string.format("Time: %d us", elapsed_us))
  print(string.format("Per pass: ~%d us", math.floor(elapsed_us / passes)))
end

local suites = {
  fib = bench_fib_looped,
  ["fib-stress"] = bench_fib_stress,
  collatz = bench_collatz,
  ["sum_squares"] = bench_sum_squares,
  ["sum-squares"] = bench_sum_squares,
}

local order = { "fib-stress", "fib", "collatz", "sum_squares" }

local function run_one(name)
  local fn = suites[name]
  if not fn then
    error("unknown suite: " .. name)
  end
  print("")
  print("===== " .. name .. " (Lua 5.5) =====")
  fn()
end

local target = arg[1]
if not target or target == "" or target == "all" then
  for _, name in ipairs(order) do
    run_one(name)
  end
else
  run_one(target)
end
