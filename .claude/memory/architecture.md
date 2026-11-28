# Architecture

## Directory map

```
CMakeLists.txt        root: C/C++/RC project, Qt6 components, adds scripts/ then qml/
cmake/version.rc.in   Windows version resource template (lh_add_version_info function)
scripts/              C++ (despite the name): include/ headers, src/ implementation, main/ entry points
  CMakeLists.txt      lhcore static lib, core_test, Dockly launcher stub
  build.ps1           configure + build + optional deploy
qml/                  Qt Quick frontend, built into DocklyApp.dll as QML module URI `DocklyUI`
  components/ dialogs/ pages/ shaders/aurora.frag
connectors/browser/   Chrome/Edge MV3 extension (copied next to the build output)
build.bat             wrapper around scripts/build.ps1 + commit/push + launch
build/  dist/         build output / deployed copy (both git-ignored); any build-*/ ignored too
```

## CMake targets

| Target | Kind | Notes |
|---|---|---|
| `lhcore` | static lib | All logic: AppCore, Database, Sites, Connectors, DockGuard, scanner, etc. Links Qt Core/Gui/Widgets/Network/Concurrent/Sql/Svg plus Win32 libs (psapi dwmapi pdh version ole32 shell32 uuid gdi32). Defines `LH_VERSION`. |
| `core_test` | exe | QtTest, `scripts/main/core_test.cpp`; registered with `add_test` |
| `Dockly` | WIN32 exe | C stub `launcher_main.c`, statically linked, no Qt |
| `DocklyApp` | shared lib (DLL) | `app_main.cpp` + image providers + QML module; exports `dockly_main`; post-build copies `connectors/` beside it |

Optional native layer (CMake `DOCKLY_NATIVE`, default OFF): Rust crate `native/` builds
`dockly_native.dll` (C ABI in `native/include/dockly_native.h`: `dn_abi_version`,
`dn_system_info_read`). Only `nativebridge.h/.cpp` calls it, loading it at run time; so far it is
used for one startup log line. Wiring: `cmake/native.cmake`. Design: `docs/native-integration-proposal.md`.

Qt policies QTP0001 / QTP0004 are set to NEW in `qml/CMakeLists.txt`. Shaders compile to `.qsb`
via `qt_add_shaders` (prefix `/qt/qml/DocklyUI`).

## Packaging (verified in `launcher_main.c`, `build.ps1`)

`dist/Dockly.exe` is only a stub. It sets `SetDllDirectoryW(resources\DLLs)`, `QT_PLUGIN_PATH =
resources`, `QML2_IMPORT_PATH = resources\qml`, loads `resources\DLLs\DocklyApp.dll` and calls
`dockly_main(argc, argv)`. All Qt DLLs live in `resources\DLLs`; plugin folders and QML modules are
moved up into `resources\` by `build.ps1` after `windeployqt`.

## Startup sequence (`dockly_main`, `scripts/main/app_main.cpp`)

1. Read `graphicsApi` straight from `settings.json` before any window (software / d3d11 / opengl;
   OpenGL adds 4x MSAA). `LH_NO_GL=1` skips this.
2. Create `QApplication`, `QQuickStyle` = Basic, parse CLI flags (data-and-settings.md).
3. **Single instance** per data folder: a `QLocalServer` named `Dockly-<md5 of data dir>`; a second
   launch sends "show" to the first and exits.
4. Open `dockly.log` in the data folder (reset above 512 KB), install a message handler.
5. `AppCore::instance()->init()`: data dir, settings load, open DBs, prune catalog, load Sites,
   start RunningApps/Gamepad/PerfMonitor/Connectors, check items, optional startup workspace.
6. Create `TrayController`, register QML types (`LH` enums, `ItemModel`), image providers
   (`item`, `icon`, `screen`), context properties (`core`, `tray`, `dockGuard`, `workArea`,
   `screenRect`, `startRing`, `startDialog`, `startPage`, `dockAllowed`, `dockPinned`).
7. Load three root objects in order: `Main` (window), `DockWindow`, `QuickLauncher`. Fewer than
   three means a fatal "interface failed to load" dialog.
8. Wire global hotkeys (`GlobalHotkey` ids 0x444B dock, 0x514C launcher), tray signals, memory trim.
9. Decide what to show: dock-only when `dockEnabled`; tray-only when tray active; otherwise bring
   the main window to front (with foreground-stealing workaround `grabForeground`).

## Layering rules

- QML talks only to `AppCore` (`core`) and `ItemModel`; no SQL or filesystem logic in QML.
- Heavy work (scans, validation) runs on worker threads via `QFutureWatcher`.
- Main window and quick launcher set `setPersistentSceneGraph(false)` so GPU resources are freed
  when hidden.

## Memory model

Views keep only lists of row ids; each visible card is fetched from SQLite (`m_cache` row cache,
cleared on every change). Icons: small RAM `QCache` (4 MB) in front of a PNG-per-icon disk cache
under the Qt cache location (`.../pix`). Idle trim (`memoryIdleTrim` seconds) drops caches and
QML pages, then `engine.trimComponentCache()` + `collectGarbage()`.
