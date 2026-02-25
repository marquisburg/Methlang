@echo off
REM Windows build script for MethASM

REM Check if gcc is available
where gcc >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: gcc not found. Please install MinGW-w64 or similar.
    echo You can download it from: https://www.mingw-w64.org/downloads/
    exit /b 1
)

REM Create directories
if not exist obj mkdir obj
if not exist obj\lexer mkdir obj\lexer
if not exist obj\parser mkdir obj\parser
if not exist obj\semantic mkdir obj\semantic
if not exist obj\codegen mkdir obj\codegen
if not exist obj\debug mkdir obj\debug
if not exist obj\error mkdir obj\error
if not exist obj\runtime mkdir obj\runtime
if not exist bin mkdir bin

REM Compile source files
echo Compiling lexer...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\lexer\lexer.c -o obj\lexer\lexer.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Compiling parser...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\parser\ast.c -o obj\parser\ast.o
if %ERRORLEVEL% NEQ 0 exit /b 1

gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\parser\parser.c -o obj\parser\parser.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Compiling semantic analysis...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\semantic\symbol_table.c -o obj\semantic\symbol_table.o
if %ERRORLEVEL% NEQ 0 exit /b 1

gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\semantic\type_checker.c -o obj\semantic\type_checker.o
if %ERRORLEVEL% NEQ 0 exit /b 1

gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\semantic\register_allocator.c -o obj\semantic\register_allocator.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Compiling code generator modules...
for %%f in (src\\codegen\\*.c) do (
    echo   %%~nxf
    gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c %%f -o obj\\codegen\\%%~nf.o
    if errorlevel 1 exit /b 1
)

echo Compiling debug info...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\debug\debug_info.c -o obj\debug\debug_info.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Compiling gc runtime...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\runtime\gc.c -o obj\runtime\gc.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Compiling error reporter...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\error\error_reporter.c -o obj\error\error_reporter.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Compiling main...
gcc -Wall -Wextra -std=c99 -g -O0 -D_GNU_SOURCE -c src\main.c -o obj\main.o
if %ERRORLEVEL% NEQ 0 exit /b 1

echo Linking...
gcc obj\lexer\lexer.o obj\parser\ast.o obj\parser\parser.o obj\semantic\symbol_table.o obj\semantic\type_checker.o obj\semantic\register_allocator.o obj\\codegen\\*.o obj\debug\debug_info.o obj\runtime\gc.o obj\error\error_reporter.o obj\main.o -o bin\methasm.exe

if %ERRORLEVEL% EQU 0 (
    echo Build successful! Executable created at bin\methasm.exe
) else (
    echo Build failed!
    exit /b 1
)


