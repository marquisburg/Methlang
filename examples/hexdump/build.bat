@echo off
REM Build Mettle Hex Dump Utility
REM argc/argv startup is emitted directly through CRT __getmainargs.
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
    gcc -nostartfiles examples\hexdump\hexdump.o -o examples\hexdump\hexdump.exe -lkernel32
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
