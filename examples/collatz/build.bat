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

echo Building collatz.meth (native compiler backend)...
bin\methlang.exe --build --emit-obj --linker internal --release examples\collatz\collatz.meth -o examples\collatz\collatz.exe --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang build failed.
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
