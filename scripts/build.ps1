# Configure + build Dockly with the Qt 6.12.0 MinGW kit, then (optionally) deploy.
#   .\scripts\build.ps1            incremental build (Ninja recompiles only what changed)
#   .\scripts\build.ps1 -Deploy    build + copy Qt DLLs/plugins into .\dist so Dockly.exe runs anywhere
param([switch]$Deploy)
Set-Location (Split-Path $PSScriptRoot -Parent)

$qtRoot = 'C:\Qt'
$qt     = "$qtRoot\6.12.0\mingw_64"
$env:PATH = "$qtRoot\Tools\mingw1310_64\bin;$qtRoot\Tools\Ninja;$qtRoot\Tools\CMake_64\bin;$qt\bin;$env:PATH"

# CMake caches absolute paths, so a build folder left over from before the project was moved
# or renamed is unusable: recreate it. It only holds generated files.
$cache = 'build\CMakeCache.txt'
if (Test-Path $cache) {
    $line = Select-String -Path $cache -Pattern '^CMAKE_CACHEFILE_DIR:INTERNAL=(.+)$' | Select-Object -First 1
    $want = (Join-Path (Get-Location).Path 'build').Replace('\', '/')
    if ($line -and $line.Matches[0].Groups[1].Value -ne $want) {
        Write-Host "The build folder belongs to another location - recreating it."
        try { Remove-Item build -Recurse -Force -ErrorAction Stop }
        catch { Write-Host "Cannot delete the build folder - close anything running from it and retry."; exit 1 }
    }
}

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
    if (-not (Test-Path $stamp)) {
        # Deploy in place (never wipe dist: a running Dockly.exe may lock files, and a half-wiped dist is broken).
        New-Item -ItemType Directory -Force dist\resources\DLLs | Out-Null
        Copy-Item build\DocklyApp.dll dist\resources\DLLs\ -Force -ErrorAction Stop
        windeployqt --release --compiler-runtime --no-translations --qmldir qml --dir dist\resources\DLLs build\DocklyApp.dll
        if ($LASTEXITCODE) { exit $LASTEXITCODE }
        # plugin folders (platforms, qml, styles ...) go up into resources; only loose DLLs stay in resources\DLLs
        foreach ($d in Get-ChildItem dist\resources\DLLs -Directory) {
            robocopy $d.FullName "dist\resources\$($d.Name)" /E /MOVE /NFL /NDL /NJH /NJS /NP | Out-Null
            if ($LASTEXITCODE -ge 8) { Write-Host "Could not move $($d.Name)"; exit 1 }
            Remove-Item $d.FullName -Recurse -Force # robocopy leaves files that were already identical in place
        }
        $global:LASTEXITCODE = 0
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
