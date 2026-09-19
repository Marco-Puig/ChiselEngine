@echo off
setlocal

rem Run this from the project root (same folder as CMakeLists.txt).
rem Always builds and runs the Release configuration.
cd /d "%~dp0"
set "ROOT=%~dp0"

set CONFIG=Release

echo Building %CONFIG% ...
cmake --build build --config %CONFIG%
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

set EXE=

if exist "build\bin\%CONFIG%\ChiselEngine.exe" set EXE=build\bin\%CONFIG%\ChiselEngine.exe
if "%EXE%"=="" if exist "build\%CONFIG%\ChiselEngine.exe" set EXE=build\%CONFIG%\ChiselEngine.exe
if "%EXE%"=="" if exist "build\bin\ChiselEngine.exe" set EXE=build\bin\ChiselEngine.exe
if "%EXE%"=="" if exist "build\ChiselEngine.exe" set EXE=build\ChiselEngine.exe

if "%EXE%"=="" (
    echo Could not find ChiselEngine.exe under build\. Build it first with rebuild.bat.
    exit /b 1
)

echo Running %EXE% ...
rem Run from the exe's own folder so relative paths (e.g. resources\) resolve correctly.
set "EXE_DIR=%ROOT%build\bin\%CONFIG%"
pushd "%EXE_DIR%"
if errorlevel 1 (
    echo Could not enter the executable directory.
    pause
    exit /b 1
)
.\ChiselEngine.exe
set RUN_CODE=%ERRORLEVEL%
popd
if not "%RUN_CODE%"=="0" (
    echo ChiselEngine exited with code %RUN_CODE%.
    pause
)
exit /b %RUN_CODE%
