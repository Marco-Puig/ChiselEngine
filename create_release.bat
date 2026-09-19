@echo off
setlocal

rem Builds ChiselEngine in Release config and packages the output into a
rem distributable folder + zip under release\.
rem Run this from the project root (same folder as CMakeLists.txt).
rem Optional first argument: a version/name tag for the package,
rem e.g. "create_release.bat v0.1.0" (default: "latest")

cd /d "%~dp0"
set "ROOT=%~dp0"
set "CONFIG=Release"

set "VERSION=%~1"
if "%VERSION%"=="" set "VERSION=latest"

set "RELEASE_NAME=ChiselEngine-%VERSION%"
set "RELEASE_DIR=%ROOT%release\%RELEASE_NAME%"
set "ZIP_PATH=%ROOT%release\%RELEASE_NAME%.zip"

echo ============================================
echo  Building ChiselEngine (%CONFIG%)
echo ============================================
cmake --build build --config %CONFIG%
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

rem Locate the built executable (same search order as run.bat)
set "EXE="
if exist "build\bin\%CONFIG%\ChiselEngine.exe" set "EXE=build\bin\%CONFIG%\ChiselEngine.exe"
if "%EXE%"=="" if exist "build\%CONFIG%\ChiselEngine.exe" set "EXE=build\%CONFIG%\ChiselEngine.exe"
if "%EXE%"=="" if exist "build\bin\ChiselEngine.exe" set "EXE=build\bin\ChiselEngine.exe"
if "%EXE%"=="" if exist "build\ChiselEngine.exe" set "EXE=build\ChiselEngine.exe"

if "%EXE%"=="" (
    echo Could not find ChiselEngine.exe under build\. Build it first.
    exit /b 1
)

for %%F in ("%EXE%") do set "EXE_DIR=%%~dpF"

echo.
echo ============================================
echo  Packaging release: %RELEASE_NAME%
echo ============================================

if exist "%RELEASE_DIR%" (
    echo Cleaning previous release folder...
    rmdir /s /q "%RELEASE_DIR%"
)
mkdir "%RELEASE_DIR%" 2>nul

rem Copy everything next to the exe (Resources\ is already placed there post-build)
echo Copying "%EXE_DIR%" -^> "%RELEASE_DIR%"
xcopy "%EXE_DIR%*" "%RELEASE_DIR%\" /e /i /y /q >nul
if errorlevel 1 (
    echo Copy failed.
    exit /b 1
)

rem Strip build artifacts not needed for distribution
del /q "%RELEASE_DIR%\*.pdb" 2>nul
del /q "%RELEASE_DIR%\*.ilk" 2>nul
del /q "%RELEASE_DIR%\*.exp" 2>nul
del /q "%RELEASE_DIR%\*.lib" 2>nul

echo.
echo ============================================
echo  Zipping release
echo ============================================
if exist "%ZIP_PATH%" del /q "%ZIP_PATH%"
powershell -NoProfile -Command "Compress-Archive -Path '%RELEASE_DIR%\*' -DestinationPath '%ZIP_PATH%' -Force"
if errorlevel 1 (
    echo Zipping failed ^(the unzipped folder is still available^).
    exit /b 1
)

echo.
echo Release ready:
echo   Folder: %RELEASE_DIR%
echo   Zip:    %ZIP_PATH%
exit /b 0
