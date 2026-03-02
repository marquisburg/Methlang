@echo off
setlocal enabledelayedexpansion

REM meth-build.bat - A wrapper script for Methlang
REM Automatically invokes methlang.exe, nasm, and gcc

if "%~1"=="" (
    echo Usage: meth-build ^<input.meth^> [methlang options...]
    exit /b 1
)

set "METH_INSTALL_DIR=%~dp0.."
set "METH_CC=%~dp0methlang.exe"
set "METH_STDLIB=%METH_INSTALL_DIR%\stdlib"
set "METH_RUNTIME=%METH_INSTALL_DIR%\src\runtime"

REM Check for target tools
where gcc >nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo Error: gcc not found in PATH. Please install MinGW-w64.
    exit /b 1
)
where nasm >nul 2>&1
if !ERRORLEVEL! NEQ 0 (
    echo Error: nasm not found in PATH. Please install NASM.
    exit /b 1
)

set "INPUT_FILE=%~1"
REM Get pure basename without path or extension
for %%I in ("%INPUT_FILE%") do set "BASENAME=%%~nI"

set "ASM_FILE=%BASENAME%.s"
set "OBJ_FILE=%BASENAME%.o"
set "EXE_FILE=%BASENAME%.exe"

REM Collect remaining arguments
set "ARGS="
:loop
shift
if "%~1"=="" goto continue
set "ARGS=!ARGS! %1"
goto loop
:continue

echo [1/3] Compiling %INPUT_FILE% with Methlang...
"%METH_CC%" "%INPUT_FILE%" --stdlib "%METH_STDLIB%" -o "%ASM_FILE%" !ARGS!
if !ERRORLEVEL! NEQ 0 exit /b 1

echo [2/3] Assembling %ASM_FILE% with NASM...
nasm -f win64 "%ASM_FILE%" -o "%OBJ_FILE%"
if !ERRORLEVEL! NEQ 0 exit /b 1

echo [3/3] Linking with GCC to create %EXE_FILE%...
gcc -nostartfiles "%OBJ_FILE%" "%METH_RUNTIME%\gc.c" "%METH_RUNTIME%\methlang_entry.c" -o "%EXE_FILE%" -lkernel32 -lshell32
if !ERRORLEVEL! NEQ 0 exit /b 1

echo.
echo Build successful: %EXE_FILE%
