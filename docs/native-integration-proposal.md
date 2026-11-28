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

## 6. Module layout

Seven separate modules would be mostly empty, so start with one crate and split by file.

```
native/
  Cargo.toml  Cargo.lock
  include/dockly_native.h     the whole contract, hand-written
  src/lib.rs                  exports only
  src/ffi.rs                  pointer and length checks; all boundary unsafe code
  src/sysinfo.rs              first module
scripts/include/nativebridge.h, scripts/src/nativebridge.cpp   Qt wrapper, the only C++ caller
scripts/main/native_test.cpp  boundary tests from C++
```

Later modules (`devices.rs`, `events.rs`) are added when a feature needs them.

## 7. FFI contract

1. C ABI only; symbols prefixed `dn_`; fixed-width integer types.
2. `dn_abi_version()` is checked at load. A mismatch disables native features.
3. Structs are `#[repr(C)]`, start with `uint32_t struct_size`, and are mirrored by
   `static_assert` on size and offsets in C++ and the same checks in Rust.
4. **Caller owns buffers.** Functions take pointer + capacity and report the required size.
   No allocation crosses the boundary.
5. Long-lived state is an opaque handle with matching `dn_*_open` / `dn_*_close`, wrapped in C++
   by `std::unique_ptr` with a custom deleter.
6. Every call returns an `int32_t` status; results go through out-pointers. Text is UTF-8 with an
   explicit length.
7. Exports catch panics (`catch_unwind`, so release builds keep `panic = "unwind"`) and return an
   error status. C++ callbacks are `noexcept`. Nothing unwinds across the boundary.
8. Callbacks arrive on a Rust thread; the bridge re-posts them to the Qt thread with a queued call.
   `close` stops and joins the thread, and no callback runs after it returns.
9. Each handle documents whether it is thread-safe. Default: one thread.
10. Each `unsafe` block states the invariant it relies on.

## 8. Build and deployment

- CMake option `DOCKLY_NATIVE`, default OFF until stable; `build.ps1 -Native` turns it on.
  OFF builds the application without any native code.
- `cmake/native.cmake` runs `cargo build --release --locked` and copies the DLL into `build\`.
- The DLL is **loaded at run time** by full path (no import library), so a missing or mismatched
  file disables native features instead of stopping Dockly.
- `build.ps1 -Native -Deploy` copies `dockly_native.dll` into `dist\resources\DLLs`.
- Prefer no third-party crates at first (hand-declared Win32 functions). `windows-sys` is the
  likely later choice; it needs one download and should then be vendored or locked.
- `native/target/` goes into `.gitignore`. This matters because `build.bat` runs `git add -A`.

## 9. Hardware support

All items use normal user permissions. No driver, elevation or physical memory access.

| Capability | Windows route | Status |
|---|---|---|
| CPU, RAM, OS | `GetLogicalProcessorInformationEx`, `GlobalMemoryStatusEx` | First module |
| Storage volumes | `GetLogicalDrives`, `GetDiskFreeSpaceExW` | Cheap, read-only |
| Network adapters | `GetAdaptersAddresses` | Cheap, read-only |
| GPU adapters | DXGI enumeration | Read-only; load stays in `perfmonitor` |
| USB, HID, serial port list | SetupAPI / Configuration Manager | After a feature asks |
| Connect / disconnect events | `CM_Register_Notification` | With the device list |
| Serial and HID data transfer | `CreateFileW`, overlapped I/O, timeouts | Needs a target device |
| Bluetooth / BLE | WinRT or Win32 GATT | Needs a target device; larger work |
| Sensors, vendor SDKs, MMIO | vendor or driver specific | Out of scope until named |

## 10. Memory management

Keep the standard allocators. First measure where the 181 MB private memory sits (likely
candidates are Qt Quick, graphics and caches, none confirmed), for example with VMMap or Windows
Performance Recorder, comparing software and OpenGL rendering, before and after idle trim.
`EmptyWorkingSet` reduces the working set, not private memory. Any change to caching would be
C++/QML work, which a Rust allocator does not reach.

## 11. Phases and exit checks

| Phase | Work | Done when |
|---|---|---|
| 3 | Crate, header, bridge, CMake; `dn_abi_version` and one system-info call; no UI change | All three suites and `native_test` pass; `DOCKLY_NATIVE=OFF` builds; deployed app starts |
| 4 | Read-only device inventory and events, for a chosen feature | Simulated-source tests; hot-plug checked by hand |
| 5 | Profile; optimise only what the profile shows | Before/after numbers over repeated runs |
| 6 | Boundary tests (null, short buffer, bad size, double close), long-run handle and memory check | Results recorded |

Rust unit tests and Miri cover pure logic only; Miri cannot execute the Win32 calls, and Miri
needs a nightly toolchain that is not installed.

## 12. Risks

| Risk | Mitigation |
|---|---|
| Two C runtimes in one process | No CRT objects, memory or file handles cross the boundary |
| MSYS2 Rust updates independently of Qt's compiler | Record the version; ABI and link test in CI or build |
| Second toolchain required to build | `DOCKLY_NATIVE` switch |
| Duplicating `perfmonitor` | Native module reports inventory, not load, unless it replaces it |
| A crash in native code ends Dockly | Boundary validation and tests; keep the module small |

## 13. Decision needed

Which user-visible feature should use the hardware layer (for example a system panel, drive or
device tiles, a serial or BLE device)? Phase 3 does not depend on the answer; Phase 4 does.

## 14. Phase 3 result (2026-11-27)

Added: `native/` (crate and header), `nativebridge.h/.cpp`, `native_test.cpp`,
`native_badabi.c` (test fixture), `cmake/native.cmake`, the CMake option, `build.ps1 -Native`, and
one startup log line in `app_main.cpp`. The interface is `dn_abi_version` and
`dn_system_info_read` (processors, memory, Windows version). No UI change.

| Check | Result |
|---|---|
| `build.ps1 -Native`, then `ctest` | 5 of 5 passed: `core_test`, `guard_test`, `guard3_test`, `native_test`, `native_rust` |
| `native_rust` / `native_test` | 8 Rust tests; 15 QtTest results including set-up and clean-up |
| `build.ps1` (OFF), then `ctest` | 3 of 3 passed |
| Scratch copy of `dist` with the DLL | Starts, logs the system summary, exits 0 (screenshot mode) |
| Same copy without the DLL | Starts, logs "native layer unavailable", exits 0 |
| DLL | 235 KB; exports the two `dn_` functions; imports Windows system and UCRT DLLs |

Call cost, three runs: `dn_system_info_read` 3.4 to 3.9 µs; `GlobalMemoryStatusEx` alone
0.40 to 0.45 µs; a rejected call (boundary only) about 5 ns. The snapshot does more work than the
single Win32 query, so these are not a like-for-like speed comparison.

Not checked: `build.ps1 -Native -Deploy` into the real `dist\` (Dockly was running), a machine
without MSYS2 or with more than 64 logical processors, a `rustup` toolchain, Miri.
