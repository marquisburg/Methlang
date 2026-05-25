@echo off
setlocal
pushd "%~dp0" || exit /b 1
powershell -NoProfile -Command "if (Get-Process engine_editor -ErrorAction SilentlyContinue) { exit 7 } else { exit 0 }" >nul 2>nul
if errorlevel 1 (
    echo engine_editor.exe is running. Close the editor before rebuilding examples\engine_editor.exe.
    popd
    exit /b 1
)

set "LUA_DLL_DIR="
if exist "%LOCALAPPDATA%\Programs\Lua\lua55.dll" set "LUA_DLL_DIR=%LOCALAPPDATA%\Programs\Lua"
if "%LUA_DLL_DIR%"=="" if defined LUA_HOME if exist "%LUA_HOME%\lua55.dll" set "LUA_DLL_DIR=%LUA_HOME%"
if "%LUA_DLL_DIR%"=="" (
    echo Could not find lua55.dll. Install Lua 5.5 to %%LOCALAPPDATA%%\Programs\Lua or set LUA_HOME.
    popd
    exit /b 2
)
set "PATH=%LUA_DLL_DIR%;%PATH%"

bin\mettle.exe --profile --build --emit-obj --linker internal --link-arg -lvulkan-1 --link-arg -lcomdlg32 --link-arg -llua55 examples\engine_editor.mettle -o examples\engine_editor.exe
set "CODE=%ERRORLEVEL%"
if "%CODE%"=="0" (
    copy /Y "%LUA_DLL_DIR%\lua55.dll" examples\lua55.dll >nul
) else (
    popd
    exit /b %CODE%
)

bin\mettle.exe --profile --build --emit-obj --linker internal --link-arg -llua55 examples\engine_play_server.mettle -o examples\engine_play_server.exe
set "CODE2=%ERRORLEVEL%"
if "%CODE2%"=="0" (
    copy /Y "%LUA_DLL_DIR%\lua55.dll" examples\lua55.dll >nul
) else (
    popd
    exit /b %CODE2%
)

bin\mettle.exe --profile --build --emit-obj --linker internal --link-arg -lvulkan-1 --link-arg -llua55 examples\engine_play_client.mettle -o examples\engine_play_client.exe
set "CODE3=%ERRORLEVEL%"
if "%CODE3%"=="0" (
    copy /Y "%LUA_DLL_DIR%\lua55.dll" examples\lua55.dll >nul
) else (
    popd
    exit /b %CODE3%
)

popd
exit /b 0
