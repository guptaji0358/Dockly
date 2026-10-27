# Configure + build Dockly with the Qt 6.11.1 MinGW kit, then (optionally) deploy.
#   .\scripts\build.ps1            build
#   .\scripts\build.ps1 -Deploy    build + copy Qt DLLs/plugins into .\dist so Dockly.exe runs anywhere
param([switch]$Deploy)
Set-Location (Split-Path $PSScriptRoot -Parent)

$qtRoot = 'D:\Apps\Qt6'
$qt     = "$qtRoot\6.11.1\mingw_64"
$env:PATH = "$qtRoot\Tools\mingw1310_64\bin;$qtRoot\Tools\Ninja;$qtRoot\Tools\CMake_64\bin;$qt\bin;$env:PATH"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$qt" `
      "-DCMAKE_CXX_COMPILER=$qtRoot/Tools/mingw1310_64/bin/g++.exe"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build
if ($LASTEXITCODE) { exit $LASTEXITCODE }

if ($Deploy) {
    # dist\Dockly.exe is a stub; everything else lives in dist\resources:
    #   resources\DLLs  DocklyApp.dll + all Qt DLLs      resources\<plugin dirs>, resources\qml  Qt plugins / QML modules
    if (Test-Path dist) { Remove-Item dist -Recurse -Force }
    New-Item -ItemType Directory -Force dist\resources\DLLs | Out-Null
    Copy-Item build\Dockly.exe dist\ -Force
    Copy-Item build\connectors dist\resources\ -Recurse -Force
    Copy-Item build\DocklyApp.dll dist\resources\DLLs\ -Force
    windeployqt --release --compiler-runtime --no-translations --qmldir qml --dir dist\resources\DLLs build\DocklyApp.dll
    # plugin folders (platforms, qml, styles ...) go up into resources; only loose DLLs stay in resources\DLLs
    Get-ChildItem dist\resources\DLLs -Directory | Move-Item -Destination dist\resources -Force
}
