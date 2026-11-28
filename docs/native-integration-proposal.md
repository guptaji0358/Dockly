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

## 3. Measurements (baseline)

Single runs on this machine (12 logical cores). Not a profile.

Installed Dockly, running 314 minutes, dock, gamepad, running-apps and auto Lite enabled:

| Metric | Value |
|---|---|
| CPU over 60 s | 0.5 s CPU time, about 0.8% of one core |
| CPU over a separate 20 s | 78 ms |
| Private memory | 181 MB, unchanged over 60 s |
| Working set | 33 MB (peak 239 MB) |
| Threads / handles | 24 / about 1771, no growth over 60 s |

Existing tests on the current `build\` output:

| Suite | Result | Time |
|---|---|---|
| `core_test` | 59 passed, 0 failed | 2.6 s |
| `guard_test` | 16 passed, 0 failed | 0.6 s |
| `guard3_test` | 23 passed, 0 failed | 0.2 s |

The slowest single `core_test` functions, including process start, took under 0.9 s
(catalog scan, packaged-app inspection, catalog refresh).

Not measured: startup time, UI frame time, per-timer CPU cost, allocation counts, thread
contention, leaks over long runs. The one-minute samples do not rule out a leak.

## 4. Toolchain probe (throwaway code outside the repo)

A Rust library exporting `probe_sum(ptr, len)`, using a thread and `println!`, called from C++
built with Qt's `g++`:

| Linking | Result |
|---|---|
| Rust `cdylib` (DLL) | Links and runs, correct result. 261 KB. Imports only Windows system and UCRT DLLs |
| Rust `staticlib`, first try | Link errors: `WaitOnAddress` family missing |
| `staticlib` + `-lsynchronization -static` | Links, then **segfaults inside `probe_sum`**; `-lucrt` did not help |

The crash was not root-caused. A msvcrt/UCRT toolchain mismatch is the suspected reason.
**Use the DLL.** Static linking should wait for a matching toolchain (official `rustup`
`x86_64-pc-windows-gnu`, untested here).

## 5. Language responsibilities

| Language | Owns | Does not own |
|---|---|---|
| C++ / Qt | UI, application logic, existing Win32 modules, orchestration | New device back ends |
| Rust | New hardware detection and device I/O, their worker threads | Qt objects, UI, database |
| C | The launcher; later, only a thin shim where an SDK requires it | Anything Rust can bind directly |

