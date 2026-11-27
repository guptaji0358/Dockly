# Background Process Guard Stages 3.2-3.4 / 3.6 (user-triggered enforcement)

Files (scripts/): `guardexec.h/.cpp/_win.cpp` (ProcessOps adapter, enforceClosed, planOpen, applyOpen),
`guardstore.h/.cpp` (SQLite guard.db: DPAPI-protected originals, action log, per-card policy, soft-control
records, revertPendingSoft), `guardstartup.h/.cpp/_win.cpp` (StartupBackend, StartupManager), `appcore_guard.cpp`
(AppCore glue). Tests: `guard3_test.cpp` -> `build\guard3_test.exe` (19 cases, fake adapters + isolated registry key).

- No background loop. Enforcement runs only when the user presses "Run closed-app policy now" / "Apply open-app
  controls now" in Inspect Processes, on an inspection < 60 s old; master + card switch + explicit allow-list needed.
- Identity (pid + creation time + exe) re-verified right before each action; mismatch => nothing done.
- Closed apps: WM_CLOSE, grace period, force ONLY with separate per-card allowForce. No silent fallback.
- Open apps: observation by default; only user-selected exes; lower priority (reversible, recorded in guard.db so a
  crash can revert at next start); force stop needs selection + acknowledgement. Windowed/unknown/shared = protected.
- Startup: opt-in setting `startupManagement` (default false); HKCU Run + user Startup folder only; original value
  encrypted (DPAPI); restore compares live state, conflict keeps the user's newer state; planned rows recovered.
- Qt gotchas fixed here: a null QString binds as SQL NULL (NOT NULL columns); `?1` numbered placeholders mixed with
  addBindValue mis-bind; QFile::exists() on a .lnk follows the target (use GetFileAttributesW).
- Windows limits: polling has gaps; launchers/services/Task Scheduler can relaunch anything; packaged (MSIX)
  startup is managed in Windows Settings; protected/elevated processes cannot be opened; no kernel/ELEVATED helper.
