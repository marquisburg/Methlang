@echo off
setlocal

if "%~1"=="" (
    echo Usage: mettle-build ^<input.mettle^> [mettle options...]
    exit /b 1
)

"%~dp0mettle.exe" --build %*
exit /b %ERRORLEVEL%
