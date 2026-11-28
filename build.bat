@echo off
rem Incremental build of Dockly: only what changed is recompiled, and Qt is deployed into .\dist only once.
rem   build.bat                  incremental build + deploy
rem   build.bat nodeploy         incremental build only
rem   build.bat clean            wipe .\build and .\dist first (full rebuild); "clean nodeploy" also works
rem On a successful build, commits everything with .\commit.txt and pushes (if commit.txt exists).
rem commit.txt is deleted once the commit succeeds, so each message is used only once.
cd /d "%~dp0"

set "NODEPLOY="
set "CLEAN="
for %%A in (%*) do (
    if /i "%%A"=="nodeploy" set "NODEPLOY=1"
    if /i "%%A"=="clean" set "CLEAN=1"
)

if defined CLEAN (
    if exist build rmdir /s /q build
    if exist dist rmdir /s /q dist
    if exist build (
        echo Could not delete the build folder - is Dockly.exe still running?
        exit /b 1
    )
)

if defined NODEPLOY (
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1
) else (
    powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -Deploy
)
if errorlevel 1 (
    echo.
    echo BUILD FAILED - if the linker could not write a file, close the running Dockly.exe and retry.
    exit /b 1
)

echo.
echo BUILD OK

rem Commit and push: the message comes from commit.txt (git-ignored, rewritten for each change).
if exist commit.txt (
    git add -A
    git diff --cached --quiet
    if errorlevel 1 (
        echo Committing with commit.txt
        git commit -F commit.txt
        if errorlevel 1 (
            echo COMMIT FAILED
            exit /b 1
        )
        rem The message is used up: remove it so a later build cannot reuse it.
        del commit.txt
        git push
        if errorlevel 1 (
            echo PUSH FAILED
            exit /b 1
        )
    ) else (
        echo Nothing to commit - skipping commit and push.
    )
) else (
    echo commit.txt not found - skipping commit and push.
)

rem Auto-launch: the deployed copy in .\dist, or the raw build output with "nodeploy".
if defined NODEPLOY (
    set "DOCKLY_EXE=build\Dockly.exe"
) else (
    set "DOCKLY_EXE=dist\Dockly.exe"
)
if exist "%DOCKLY_EXE%" (
    echo Launching %DOCKLY_EXE%
    start "" "%DOCKLY_EXE%"
) else (
    echo %DOCKLY_EXE% not found - not launching.
)
