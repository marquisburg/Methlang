@echo off
REM Build MethASM brainfuck interpreter
set BF=%~dp0
set ROOT=%BF%..
cd /d "%ROOT%"

if not exist bin\methasm.exe (
    echo Building MethASM compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling bfinterpreter.masm...
bin\methasm.exe brainfuck-interpreter\bfinterpreter.masm -o brainfuck-interpreter\bfinterpreter.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo MethASM compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 brainfuck-interpreter\bfinterpreter.s -o brainfuck-interpreter\bfinterpreter.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o brainfuck-interpreter\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles brainfuck-interpreter\bfinterpreter.o brainfuck-interpreter\gc.o -o brainfuck-interpreter\bfinterpreter.exe -lkernel32
) else (
    echo NASM required. Install from https://www.nasm.us/
    exit /b 1
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful. Run: cd brainfuck-interpreter ^&^& bfinterpreter.exe
) else (
    echo Link failed.
    exit /b 1
)
