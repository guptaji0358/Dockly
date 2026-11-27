# Background Process Guard Stage 3.1 (dry-run only)

Files: `include/guardengine.h`, `src/guardengine.cpp` (pure `dryRun`, `diffSnapshots`), `guardengine_win.cpp`
(`collectFacts`: session id + a PROCESS_TERMINATE open-and-close probe), `guardledger.h/.cpp` (schema, restore
conflict logic, startup plan). Tests: `scripts/main/guard_test.cpp` -> `build\guard_test.exe` (16 cases, separate
from `core_test.cpp`). No AppCore/UI wiring; the app never creates a ledger.

- No executor exists. `Decision::executed` is always false. Actions: Keep / Observe / WouldStop (proposal only).
- Candidate is never authorisation: WouldStop needs master+card switch, explicit exe allow-list, own session (not 0),
  verified terminate access, creation time known, age >= grace, no ambiguity.
- Ledger (SQLite `recovery_entries`, `recovery_events`, `ledger_meta`): stores original value + SHA-256, what we applied
  (removed/rewritten + hash). `evaluateRestore` compares live state; any user change => conflict, never overwrite.
  CHECK allows only run-hkcu / startup-folder. Packaged startup = managed by Windows. No StartupApproved (undocumented).
- Process-start detection = polling Toolhelp snapshot, diff by pid+creation time. ~13 ms/poll at ~345 processes
  (~13 ms CPU). Misses processes that live between polls; no parent-exit info; created==0 => "uncertain".
