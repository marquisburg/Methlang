@echo off
REM Build Methlang Sum-of-Squares benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building sum_squares.meth (native compiler backend)...
bin\methlang.exe --build --emit-obj --linker internal --release examples\sum_squares\sum_squares.meth -o examples\sum_squares\sum_squares.exe --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang build failed.
    exit /b 1
)

echo.
echo Building C counterpart...
gcc -O2 -o examples\sum_squares\sum_squares_c.exe examples\sum_squares\sum_squares.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Methlang: examples\sum_squares\sum_squares.exe
echo   C:       examples\sum_squares\sum_squares_c.exe
echo.
echo Running benchmark (Methlang vs C)...
echo ===== Methlang (--release) =====
examples\sum_squares\sum_squares.exe
echo.
echo ========= C (gcc -O2) =========
examples\sum_squares\sum_squares_c.exe
