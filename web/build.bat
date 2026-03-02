@echo off
REM Build Methlang web server (pure Methlang + gc runtime + Winsock)
set WEB=%~dp0
set ROOT=%WEB%..
cd /d "%ROOT%"

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling server.meth...
bin\methlang.exe --release web\server.meth -o web\server.s
if %ERRORLEVEL% NEQ 0 (
    echo Methlang compilation failed.
    exit /b 1
)

echo Assembling and linking...
where nasm >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    nasm -f win64 web\server.s -o web\server.o
    if %ERRORLEVEL% NEQ 0 (
        echo NASM assembly failed.
        exit /b 1
    )
    gcc -c src\runtime\gc.c -o web\gc.o -Isrc
    if %ERRORLEVEL% NEQ 0 exit /b 1
    gcc -nostartfiles web\server.o web\gc.o -o web\server.exe -L"%MINGW_PREFIX%\lib" -lws2_32 -lkernel32
) else (
    echo NASM required. Install from https://www.nasm.us/
    exit /b 1
)

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful. Run: cd web ^&^& server.exe
    echo Then open http://localhost:5000
) else (
    echo Link failed.
    exit /b 1
)
