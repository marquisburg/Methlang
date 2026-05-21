--[[
  High-resolution-ish timing for Mettle example benchmarks (Lua 5.5).

  On Windows with PUC-Rio Lua there is no built-in wall-clock microsecond API;
  we use os.clock() (process CPU seconds). Output still matches harness format:
    Time: <N> us

  Run scripts with: lua55 <script.lua>
]]

local M = {}

--- Elapsed CPU time since interpreter start, in microseconds (integer).
function M.bench_time_us()
  return math.floor(os.clock() * 1e6)
end

return M
