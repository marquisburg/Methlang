@echo off
REM Build Mettle Sum-of-Squares benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\mettle.exe (
    echo Building Mettle compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building sum_squares.mettle (native compiler backend)...
bin\mettle.exe --build --emit-obj --linker internal --release examples\sum_squares\sum_squares.mettle -o examples\sum_squares\sum_squares.exe
if %ERRORLEVEL% NEQ 0 (
    echo Mettle build failed.
    exit /b 1
)

echo.
echo Building C counterpart...
gcc -O3 -o examples\sum_squares\sum_squares_c.exe examples\sum_squares\sum_squares.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Mettle: examples\sum_squares\sum_squares.exe
echo   C:       examples\sum_squares\sum_squares_c.exe
echo.
echo Running benchmark (Mettle vs C)...
echo ===== Mettle (--release) =====
examples\sum_squares\sum_squares.exe
echo.
echo ========= C (gcc -O3) =========
examples\sum_squares\sum_squares_c.exe
