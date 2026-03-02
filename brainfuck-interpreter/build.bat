@echo off
REM Build Methlang brainfuck interpreter
set BF=%~dp0
set ROOT=%BF%..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling bfinterpreter.meth...
bin\methlang.exe brainfuck-interpreter\bfinterpreter.meth -o brainfuck-interpreter\bfinterpreter.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
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
