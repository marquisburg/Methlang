@echo off
setlocal
set "MASM_BIN=%~dp0"
set "MASM_PKG_EXE=%MASM_BIN%masm-pkg.exe"
if not exist "%MASM_PKG_EXE%" (
    echo Error: %MASM_PKG_EXE% not found.
    exit /b 1
)
"%MASM_PKG_EXE%" %*
