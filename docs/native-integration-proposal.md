# Dockly: Rust / C integration, investigation and proposal

Date: 2026-11-27. Covers Phase 1 (investigation), Phase 2 (proposal) and the result of Phase 3
(section 14). Sections 2 to 4 describe the project as it was before Phase 3.

## 1. Recommendation

Add **one small Rust DLL with a C ABI**, and grow it only when a feature or a measurement needs it.

- Do not rewrite or move existing C++ code. Nothing measured so far justifies it.
- Do not add new C code yet. There is no vendor SDK, driver interface or C library to wrap.
  `scripts/main/launcher_main.c` stays as it is.
- Do not build a custom allocator or memory engine. No allocation bottleneck has been shown.
- Expect **new capability** (hardware detection) from this work, not a faster launcher.

## 2. Current architecture

| Area | Today |
|---|---|
| Languages | C++17 (Qt 6.12), QML, one C launcher stub, browser extension JavaScript |
| Targets | `lhcore` static lib, `DocklyApp.dll`, `Dockly.exe` stub, three test executables |
| Compiler | Qt's MinGW 13.1, `x86_64-w64-mingw32`, msvcrt |
| Rust | 1.100.0 from MSYS2 ucrt64 (`x86_64-pc-windows-gnu`, UCRT); no `rustup`, no `cbindgen` |
| Concurrency | UI-thread `QTimer`s; `QtConcurrent::run` for item checks, scans, inspection, guard actions |
| Memory | 4 MB icon `QCache` + PNG disk cache, SQLite row cache, idle trim with `EmptyWorkingSet` |

### Existing hardware and OS access

| Code | What it does | Cadence |
|---|---|---|
| `gamepad.cpp` | XInput loaded at runtime | 33 ms when enabled |
| `perfmonitor.cpp` | CPU, RAM, GPU load (PDH) | 1 s when auto Lite is on |
| `dockguard.cpp` | Pointer, foreground window, full screen | 100 ms |
| `runningapps.cpp` | Window to executable mapping | 2 s when enabled |
| `procinspect_win.cpp`, `guard*_win.cpp` | Process inspection, guard actions, startup entries | on demand / guard |

A search found no USB, serial, Bluetooth, HID, storage, network adapter or device-change code.
No current feature consumes such data.

