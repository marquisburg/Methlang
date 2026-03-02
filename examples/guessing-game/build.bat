@echo off
REM Build Methlang Number Guessing Game
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling guessing_game.meth...
bin\methlang.exe examples\guessing-game\guessing_game.meth -o examples\guessing-game\guessing_game.s --stdlib stdlib
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\guessing-game\guessing_game.s -o examples\guessing-game\guessing_game.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\guessing-game\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\guessing-game\guessing_game.o examples\guessing-game\gc.o -o examples\guessing-game\guessing_game.exe -lkernel32
) else (
    echo NASM required. Install from https://www.nasm.us/
    exit /b 1
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Run: examples\guessing-game\guessing_game.exe
) else (
    echo Link failed.
    exit /b 1
)
