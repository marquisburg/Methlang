@echo off
REM Build Mettle Word Count benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\mettle.exe (
    echo Building Mettle compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building word_count.mettle (native compiler backend)...
bin\mettle.exe --build --emit-obj --linker internal --release examples\word_count\word_count.mettle -o examples\word_count\word_count.exe
if %ERRORLEVEL% NEQ 0 (
    echo Mettle build failed.
    exit /b 1
)

echo.
echo Building C counterpart...
gcc -O3 -o examples\word_count\word_count_c.exe examples\word_count\word_count.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Mettle: examples\word_count\word_count.exe
echo   C:       examples\word_count\word_count_c.exe
echo.
echo Running benchmark (Mettle vs C)...
echo ===== Mettle (--release) =====
examples\word_count\word_count.exe
echo.
echo ========= C (gcc -O3) =========
examples\word_count\word_count_c.exe
