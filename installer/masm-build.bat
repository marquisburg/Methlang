@echo off
setlocal enabledelayedexpansion

REM masm-build.bat - A wrapper script for MethASM
REM Automatically invokes methasm.exe, nasm, and gcc

if "%~1"=="" (
    echo Usage: masm-build ^<input.masm^> [methasm options...]
    exit /b 1
)

set "MASM_INSTALL_DIR=%~dp0.."
set "MASM_CC=%~dp0methasm.exe"
set "MASM_STDLIB=%MASM_INSTALL_DIR%\stdlib"
set "MASM_RUNTIME=%MASM_INSTALL_DIR%\src\runtime"

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

echo [1/3] Compiling %INPUT_FILE% with MethASM...
"%MASM_CC%" "%INPUT_FILE%" --stdlib "%MASM_STDLIB%" -o "%ASM_FILE%" !ARGS!
if !ERRORLEVEL! NEQ 0 exit /b 1

echo [2/3] Assembling %ASM_FILE% with NASM...
nasm -f win64 "%ASM_FILE%" -o "%OBJ_FILE%"
if !ERRORLEVEL! NEQ 0 exit /b 1

echo [3/3] Linking with GCC to create %EXE_FILE%...
gcc -nostartfiles "%OBJ_FILE%" "%MASM_RUNTIME%\gc.c" "%MASM_RUNTIME%\masm_entry.c" -o "%EXE_FILE%" -lkernel32 -lshell32
if !ERRORLEVEL! NEQ 0 exit /b 1

echo.
echo Build successful: %EXE_FILE%
