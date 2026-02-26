@echo off
REM Build MethASM web server (pure MethASM + gc runtime + Winsock)
set WEB=%~dp0
set ROOT=%WEB%..
cd /d "%ROOT%"

if not exist bin\methasm.exe (
    echo Building MethASM compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Compiling server.masm...
bin\methasm.exe web\server.masm -o web\server.s
if %ERRORLEVEL% NEQ 0 (
    echo MethASM compilation failed.
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
