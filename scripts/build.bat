@echo off
rem Clean rebuild of Dockly (wipes .\build, reconfigures, compiles, deploys Qt DLLs into .\dist).
rem   scripts\build.bat         full clean rebuild + deploy
rem   scripts\build.bat nodeploy  full clean rebuild only
cd /d "%~dp0.."

if exist build rmdir /s /q build
if exist build (
    echo Could not delete the build folder - is Dockly.exe still running?
    exit /b 1
)

if /i "%~1"=="nodeploy" (
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Deploy
)
if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)

echo.
echo BUILD OK
