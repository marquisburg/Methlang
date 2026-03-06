@echo off
setlocal enabledelayedexpansion
REM Build Methlang web server with the built-in one-command build path.
set "WEB=%~dp0"
set "ROOT=%WEB%.."
cd /d "%ROOT%"

set "METH_CC=bin\methlang.exe"
if not exist "%METH_CC%" (
    echo Building Methlang compiler...
    call build.bat
    if !ERRORLEVEL! NEQ 0 exit /b 1
)
if not exist "%METH_CC%" (
    where methlang >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set "METH_CC=methlang"
    )
)

echo Building server.meth...
"%METH_CC%" --build --release web\server.meth -o web\server.exe --link-arg -lws2_32
if !ERRORLEVEL! NEQ 0 (
    echo Web server build failed.
    exit /b 1
)

echo.
echo Build successful. Run: cd web ^&^& server.exe
echo Then open http://localhost:5000
