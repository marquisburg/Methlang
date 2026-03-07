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

echo Building fib.meth (native compiler backend)...
bin\methlang.exe --build --emit-obj --linker internal --release examples\fib\fib.meth -o examples\fib\fib.exe --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang build failed.
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
