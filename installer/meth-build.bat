@echo off
setlocal

if "%~1"=="" (
    echo Usage: meth-build ^<input.meth^> [methlang options...]
    exit /b 1
)

"%~dp0methlang.exe" --build %*
exit /b %ERRORLEVEL%
