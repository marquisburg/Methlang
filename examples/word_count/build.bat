@echo off
REM Build MethASM Word Count benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methasm.exe (
    echo Building MethASM compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling word_count.masm...
bin\methasm.exe --release examples\word_count\word_count.masm -o examples\word_count\word_count.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo MethASM compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\word_count\word_count.s -o examples\word_count\word_count.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\word_count\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\word_count\word_count.o examples\word_count\gc.o -o examples\word_count\word_count.exe -lkernel32
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
gcc -O2 -o examples\word_count\word_count_c.exe examples\word_count\word_count.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   MethASM: examples\word_count\word_count.exe
echo   C:       examples\word_count\word_count_c.exe
echo.
echo Running benchmark (MethASM vs C)...
echo ===== MethASM (--release) =====
examples\word_count\word_count.exe
echo.
echo ========= C (gcc -O2) =========
examples\word_count\word_count_c.exe
