@echo off
REM Build Methlang Word Count benchmark
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building word_count.meth (native compiler backend)...
bin\methlang.exe --build --emit-obj --linker internal --release examples\word_count\word_count.meth -o examples\word_count\word_count.exe --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang build failed.
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
echo   Methlang: examples\word_count\word_count.exe
echo   C:       examples\word_count\word_count_c.exe
echo.
echo Running benchmark (Methlang vs C)...
echo ===== Methlang (--release) =====
examples\word_count\word_count.exe
echo.
echo ========= C (gcc -O2) =========
examples\word_count\word_count_c.exe
