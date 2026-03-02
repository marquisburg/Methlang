@echo off
REM Build Methlang Collatz benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling collatz.meth...
bin\methlang.exe --release examples\collatz\collatz.meth -o examples\collatz\collatz.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\collatz\collatz.s -o examples\collatz\collatz.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\collatz\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\collatz\collatz.o examples\collatz\gc.o -o examples\collatz\collatz.exe -lkernel32
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
gcc -O2 -o examples\collatz\collatz_c.exe examples\collatz\collatz.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Methlang: examples\collatz\collatz.exe
echo   C:       examples\collatz\collatz_c.exe
echo.
echo Running benchmark (Methlang vs C)...
echo ===== Methlang (--release) =====
examples\collatz\collatz.exe
echo.
echo ========= C (gcc -O2) =========
examples\collatz\collatz_c.exe
