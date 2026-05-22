@echo off
REM Build Mettle Grep benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\mettle.exe (
    echo Building Mettle compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building grep.mettle (native compiler backend)...
bin\mettle.exe --build --emit-obj --linker internal --release examples\grep\grep.mettle -o examples\grep\grep.exe
if %ERRORLEVEL% NEQ 0 (
    echo Mettle build failed.
    exit /b 1
)

echo.
echo Building C counterpart...
gcc -O3 -o examples\grep\grep_c.exe examples\grep\grep.c -lkernel32
if %ERRORLEVEL% NEQ 0 (
    echo C build failed.
    exit /b 1
)

echo.
echo Build successful!
echo   Mettle: examples\grep\grep.exe
echo   C:       examples\grep\grep_c.exe
echo.
echo Running benchmark (Mettle vs C)...
echo ===== Mettle (--release) =====
examples\grep\grep.exe
echo.
echo ========= C (gcc -O3) =========
examples\grep\grep_c.exe
