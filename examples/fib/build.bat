@echo off
REM Build Mettle Fibonacci benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\mettle.exe (
    echo Building Mettle compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building fib.mettle (native compiler backend)...
bin\mettle.exe --build --emit-obj --linker internal --release examples\fib\fib.mettle -o examples\fib\fib.exe
if %ERRORLEVEL% NEQ 0 (
    echo Mettle build failed.
    exit /b 1
)

echo.
echo Building C counterpart...
gcc -O3 -o examples\fib\fib_c.exe examples\fib\fib.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Mettle: examples\fib\fib.exe
echo   C:       examples\fib\fib_c.exe
echo.
echo Running benchmark (Mettle vs C)...
echo ===== Mettle (--release) =====
examples\fib\fib.exe
echo.
echo ========= C (gcc -O3) =========
examples\fib\fib_c.exe
