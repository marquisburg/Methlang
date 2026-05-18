@echo off
REM Build Mettle Hex Dump Utility
REM Requires: argc/argv -> mettle_entry.o and -lshell32
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\mettle.exe (
    echo Building Mettle compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling hexdump.mettle...
bin\mettle.exe examples\hexdump\hexdump.mettle -o examples\hexdump\hexdump.s
if %ERRORLEVEL% NEQ 0 (
    echo Mettle compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 examples\hexdump\hexdump.s -o examples\hexdump\hexdump.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o examples\hexdump\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -c src\runtime\mettle_entry.c -o examples\hexdump\mettle_entry.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles examples\hexdump\hexdump.o examples\hexdump\gc.o examples\hexdump\mettle_entry.o -o examples\hexdump\hexdump.exe -lkernel32 -lshell32
) else (
    echo NASM required. Install from https://www.nasm.us/
    exit /b 1
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Run: hexdump.exe ^<filename^>
    echo Example: examples\hexdump\hexdump.exe examples\hexdump\hexdump.mettle
) else (
    echo Link failed.
    exit /b 1
)
