# Configure + build Dockly with the Qt 6.11.1 MinGW kit, then (optionally) deploy.
#   .\build.ps1            build
#   .\build.ps1 -Deploy    build + copy Qt DLLs/plugins into .\dist so Dockly.exe runs anywhere
param([switch]$Deploy)

$qtRoot = 'D:\Apps\Qt6'
$qt     = "$qtRoot\6.11.1\mingw_64"
$env:PATH = "$qtRoot\Tools\mingw1310_64\bin;$qtRoot\Tools\Ninja;$qtRoot\Tools\CMake_64\bin;$qt\bin;$env:PATH"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release "-DCMAKE_PREFIX_PATH=$qt" `
      "-DCMAKE_CXX_COMPILER=$qtRoot/Tools/mingw1310_64/bin/g++.exe"
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build
if ($LASTEXITCODE) { exit $LASTEXITCODE }

if ($Deploy) {
    # dist\Dockly.exe is a stub; the app (DocklyApp.dll) and all Qt DLLs live in dist\DLLs
    if (Test-Path dist) { Remove-Item dist -Recurse -Force }
    New-Item -ItemType Directory -Force dist\DLLs | Out-Null
    Copy-Item build\Dockly.exe dist\ -Force
    Copy-Item build\connectors dist\ -Recurse -Force
    Copy-Item build\DocklyApp.dll dist\DLLs\ -Force
    windeployqt --release --compiler-runtime --no-translations --qmldir qml --dir dist\DLLs build\DocklyApp.dll
    # Qt plugin folders (platforms, qml, styles ...) must sit next to the exe; only loose DLLs stay in DLLs
    Get-ChildItem dist\DLLs -Directory | Move-Item -Destination dist -Force
}
