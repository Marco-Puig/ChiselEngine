@echo off
title Chisel Engine Launcher
echo Starting Chisel Engine...

:: Check if the executable exists
if not exist "bin\ChiselEngine.exe" (
    echo Error: ChiselEngine.exe not found in bin\ folder.
    echo Please build the project in Visual Studio first.
    pause
    exit /b
)

:: Run the engine
start "" "bin\ChiselEngine.exe"
echo Engine launched successfully!
exit
