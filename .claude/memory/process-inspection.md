# Process inspection (Stage 2, read-only)

Files: `scripts/include/procinspect.h`, `src/procinspect.cpp` (helpers, CardRef), `procinspect_attr.cpp`
(pure `attribute()`), `procinspect_win.cpp` (Toolhelp snapshot, limited-query handles, window owners).
AppCore: `inspectProcesses(id)` (async) -> signal `processInspection(id, result)`. UI:
`qml/dialogs/InspectDialog.qml`, card menu "Inspect Processes...". Diagnostic: `--dialog inspect|inspect-all`.

- Collected: PID, PPID, creation time, exe path, package family, visible-window flag. Command lines are
  never read or stored. No termination, registry, service, task or elevation code.
- Identity = pid + creation time; a parent is valid only if created no later than its child.
- Signals: exact exe path / package family (strong); install folder (+publisher) ; parent (supporting only).
  Needs a strong signal or folder+publisher. Ties between cards => Unknown/ambiguous. File name alone never counts.
- Shared install folders (Windows, Program Files root, profile roots) are never used.
- Classes (conservative): Main = exact exe (packaged: strong package signal + visible window). Essential = valid
  ancestor of the card's Main only. Candidate-background = attributed, no window, not a service child, not a
  descendant of a live Main, not under a shared system folder, nothing visible below it (packaged: only if a
  windowed Main exists). Everything else attributed stays Unknown (window owner, service child, Main's children,
  shared-location exe, packaged app with no window) with the reason shown.
- Tests: `proc*` cases in core_test.cpp. Stage 3 (enforcement) is NOT started.

## Diagnostics (dev/test only, never reached in normal use)
`--dialog inspect[-all][:Card]`, `--dialog menu:Card`, `--uiscript "click:x,y;right:x,y;scroll:x,y,dy;wait:ms;shot:f;shotall:prefix;quit"`
(coordinates are window pixels as in `--screenshot`; events go only to Dockly's own window), env `LH_DEMO_PACKAGED="Name=AUMID;..."`
(read only inside `--demo`). Popup menus did not render (width 12 px) in scripted runs even on the Stage 1 Main.qml: unresolved.
