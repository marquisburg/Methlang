# Run Lua 5.5 stress benchmarks (requires lua55 on PATH).
param(
    [string]$Suite = "fib-stress",
    [string]$Lua = "lua55"
)

$ErrorActionPreference = "Stop"
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = Split-Path -Parent (Split-Path -Parent $Here)
Set-Location $Root

$luaCmd = Get-Command $Lua -CommandType Application -ErrorAction SilentlyContinue
if (-not $luaCmd) {
    Write-Error "$Lua not found on PATH. Install Lua 5.5 or pass -Lua <path-to-lua55.exe>"
    exit 1
}

Write-Host "Using $($luaCmd.Source)"
& $luaCmd.Source "examples\stress\stress.lua" $Suite
