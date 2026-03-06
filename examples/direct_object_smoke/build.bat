@echo off
setlocal

REM Build and run the direct-object backend smoke example.
set APP=%~dp0
set ROOT=%APP%..\..
cd /d "%ROOT%"

set SRC=examples\direct_object_smoke\direct_object_smoke.meth
set OBJ=examples\direct_object_smoke\direct_object_smoke.obj
set EXE=examples\direct_object_smoke\direct_object_smoke.exe
set EXPECTED_EXIT=17

if not exist bin\methlang.exe (
    echo Building Methlang compiler...
    call build.bat
    if %ERRORLEVEL% NEQ 0 exit /b 1
)

echo Emitting COFF object with the direct-object backend...
bin\methlang.exe --emit-obj "%SRC%" -o "%OBJ%"
if %ERRORLEVEL% NEQ 0 (
    echo Direct object emission failed.
    exit /b 1
)

where objdump >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo.
    echo Symbol table:
    objdump -t "%OBJ%"
    echo.
    echo Relocations:
    objdump -r "%OBJ%"
)

echo.
echo Building executable from the emitted object...
bin\methlang.exe --build --emit-obj "%SRC%" -o "%EXE%"
if %ERRORLEVEL% NEQ 0 (
    echo Direct object build failed.
    exit /b 1
)

echo.
echo Running executable...
"%EXE%"
set EXIT_CODE=%ERRORLEVEL%
echo Program exit code: %EXIT_CODE%

if not "%EXIT_CODE%"=="%EXPECTED_EXIT%" (
    echo Unexpected exit code. Expected %EXPECTED_EXIT%.
    exit /b 1
)

echo Smoke example passed.
exit /b 0
