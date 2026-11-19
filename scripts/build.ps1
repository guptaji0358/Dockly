# Configure + build Dockly with the Qt 6.12.0 MinGW kit, then (optionally) deploy.
#   .\scripts\build.ps1            incremental build (Ninja recompiles only what changed)
#   .\scripts\build.ps1 -Deploy    build + copy Qt DLLs/plugins into .\dist so Dockly.exe runs anywhere
param([switch]$Deploy)
Set-Location (Split-Path $PSScriptRoot -Parent)

$qtRoot = 'C:\Qt'
$qt     = "$qtRoot\6.12.0\mingw_64"
$env:PATH = "$qtRoot\Tools\mingw1310_64\bin;$qtRoot\Tools\Ninja;$qtRoot\Tools\CMake_64\bin;$qt\bin;$env:PATH"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$qt" `
      "-DCMAKE_CXX_COMPILER=$qtRoot/Tools/mingw1310_64/bin/g++.exe"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build
if ($LASTEXITCODE) { exit $LASTEXITCODE }

if ($Deploy) {
    # dist\Dockly.exe is a stub; everything else lives in dist\resources:
    #   resources\DLLs  DocklyApp.dll + all Qt DLLs      resources\<plugin dirs>, resources\qml  Qt plugins / QML modules
    # Incremental: the slow Qt deployment (windeployqt) only runs when it has never completed
    # (delete .\dist or run "build.bat clean" to force it). Our own files are refreshed only when they changed.
    $stamp = 'dist\.qt-deployed'
    if (-not (Test-Path $stamp) -and (Test-Path dist\resources\DLLs\Qt6Core.dll)) {
        # a deployment from before this stamp existed: keep it (it may be in use) and just mark it done
        Set-Content $stamp $qt
    }
    if (-not (Test-Path $stamp)) {
        if (Test-Path dist) { Remove-Item dist -Recurse -Force }
        New-Item -ItemType Directory -Force dist\resources\DLLs | Out-Null
        Copy-Item build\DocklyApp.dll dist\resources\DLLs\ -Force
        windeployqt --release --compiler-runtime --no-translations --qmldir qml --dir dist\resources\DLLs build\DocklyApp.dll
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        # plugin folders (platforms, qml, styles ...) go up into resources; only loose DLLs stay in resources\DLLs
        Get-ChildItem dist\resources\DLLs -Directory | Move-Item -Destination dist\resources -Force
        Set-Content $stamp $qt
    }
    function Sync-IfChanged($from, $to) {
        if (-not (Test-Path $to) -or (Get-FileHash $from).Hash -ne (Get-FileHash $to).Hash) {
            try { Copy-Item $from $to -Force -ErrorAction Stop }
            catch { Write-Host "Cannot update $to - close the running Dockly.exe and run again."; exit 1 }
            Write-Host "updated $to"
        }
    }
    Sync-IfChanged build\Dockly.exe dist\Dockly.exe
    Sync-IfChanged build\DocklyApp.dll dist\resources\DLLs\DocklyApp.dll
    Copy-Item build\connectors dist\resources\ -Recurse -Force
}
