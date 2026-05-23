@echo off
REM Build Mettle Hello World (Console)
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

if not exist bin\mettle.exe (
    echo Building Mettle compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Building hello.mettle...
bin\mettle.exe --build --emit-obj --linker internal examples\hello\hello.mettle -o examples\hello\hello.exe
if %ERRORLEVEL% NEQ 0 (
    echo Mettle build failed.
    exit /b 1
)

echo.
echo Build successful! Run: examples\hello\hello.exe
echo.
examples\hello\hello.exe
