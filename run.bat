@echo off
setlocal

rem Run this from the project root (same folder as CMakeLists.txt).
rem Optional first argument: build config, e.g. "run.bat Release" (default: Debug)
cd /d "%~dp0"

set CONFIG=Debug
if not "%~1"=="" set CONFIG=%~1

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
rem Run from the exe's own folder so relative paths (e.g. Resources\) resolve correctly.
pushd "%EXE%\.."
"ChiselEngine.exe"
popd