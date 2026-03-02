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

echo Compiling sum_squares.meth...
bin\methlang.exe --release examples\sum_squares\sum_squares.meth -o examples\sum_squares\sum_squares.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\sum_squares\sum_squares.s -o examples\sum_squares\sum_squares.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\sum_squares\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\sum_squares\sum_squares.o examples\sum_squares\gc.o -o examples\sum_squares\sum_squares.exe -lkernel32
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
