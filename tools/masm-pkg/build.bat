@echo off
REM Build masm-pkg (MethASM package manager)
set PKG=%~dp0
set ROOT=%PKG%..\..
cd /d "%ROOT%"

if not exist bin\methasm.exe (
    echo Building MethASM compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling masm-pkg...
bin\methasm.exe tools\masm-pkg\main.masm -o tools\masm-pkg\main.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo MethASM compilation failed.
    exit /b 1
)

echo Assembling and linking...
nasm -f win64 tools\masm-pkg\main.s -o tools\masm-pkg\main.o
if %ERRORLEVEL% NEQ 0 (
    echo NASM assembly failed.
    exit /b 1
)

gcc -c src\runtime\gc.c -o tools\masm-pkg\gc.o -Isrc
if %ERRORLEVEL% NEQ 0 exit /b 1

gcc -c src\runtime\masm_entry.c -o tools\masm-pkg\masm_entry.o -Isrc
if %ERRORLEVEL% NEQ 0 exit /b 1

gcc -c stdlib\dir_helpers.c -o tools\masm-pkg\dir_helpers.o -Istdlib
if %ERRORLEVEL% NEQ 0 exit /b 1

gcc -nostartfiles tools\masm-pkg\main.o tools\masm-pkg\gc.o tools\masm-pkg\masm_entry.o tools\masm-pkg\dir_helpers.o -o tools\masm-pkg\masm-pkg.exe -lkernel32 -lshell32
if %ERRORLEVEL% NEQ 0 (
    echo Link failed.
    exit /b 1
)

copy /Y tools\masm-pkg\masm-pkg.exe bin\masm-pkg.exe >nul
if %ERRORLEVEL% NEQ 0 (
    echo Failed to copy masm-pkg.exe to bin.
    exit /b 1
)

echo.
echo Build successful: tools\masm-pkg\masm-pkg.exe
echo Published: bin\masm-pkg.exe
echo Run: tools\masm-pkg\masm-pkg.exe install
echo      tools\masm-pkg\masm-pkg.exe build
