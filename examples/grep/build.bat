@echo off
REM Build Methlang Grep benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling grep.meth...
bin\methlang.exe --release examples\grep\grep.meth -o examples\grep\grep.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\grep\grep.s -o examples\grep\grep.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\grep\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\grep\grep.o examples\grep\gc.o -o examples\grep\grep.exe -lkernel32
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
gcc -O2 -o examples\grep\grep_c.exe examples\grep\grep.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Methlang: examples\grep\grep.exe
echo   C:       examples\grep\grep_c.exe
echo.
echo Running benchmark (Methlang vs C)...
echo ===== Methlang (--release) =====
examples\grep\grep.exe
echo.
echo ========= C (gcc -O2) =========
examples\grep\grep_c.exe
