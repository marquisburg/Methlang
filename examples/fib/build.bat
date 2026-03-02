@echo off
REM Build Methlang Fibonacci benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling fib.meth...
bin\methlang.exe --release examples\fib\fib.meth -o examples\fib\fib.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\fib\fib.s -o examples\fib\fib.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\fib\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\fib\fib.o examples\fib\gc.o -o examples\fib\fib.exe -lkernel32
) else (
    echo NASM required. Install from https://www.nasm.us/
    exit /b 1
)

if %ERRORLEVEL% NEQ 0 (
    echo Link failed.
    exit /b 1
)

echo.
echo Building C counterpart...
gcc -O2 -o examples\fib\fib_c.exe examples\fib\fib.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Methlang: examples\fib\fib.exe
echo   C:       examples\fib\fib_c.exe
echo.
echo Running benchmark (Methlang vs C)...
echo ===== Methlang (--release) =====
examples\fib\fib.exe
echo.
echo ========= C (gcc -O2) =========
examples\fib\fib_c.exe
