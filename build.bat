@echo off
setlocal

rem Run this from the project root (same folder as CMakeLists.txt).
cd /d "%~dp0"

if exist build (
    echo Removing existing build folder...
    rmdir /s /q build
)

echo Configuring...
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.6
if errorlevel 1 (
    echo Configure failed.
    exit /b 1
)

echo Building...
cmake --build build
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Done.
