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

echo Building grep.meth (native compiler backend)...
bin\methlang.exe --build --emit-obj --linker internal --release examples\grep\grep.meth -o examples\grep\grep.exe --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang build failed.
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
