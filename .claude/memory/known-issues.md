# Known issues, doc/code conflicts, unverified areas

Status as of 2026-11-19 (initial analysis). Nothing here was fixed; these are observations.

## Documentation vs code conflicts (code wins)

| Where | Doc says | Code says |
|---|---|---|
| `.claude/CLAUDE.md` | `build.bat` does a clean rebuild | `build.bat` is incremental; only `build.bat clean` wipes `build/` and `dist/` |
| `Main.qml` tabs | 9 tabs | `Ctrl+1..N` Repeater has `model: 8`, so Settings has no shortcut |
| README "Flowchart -> code" | Hubs listed, flowchart-based | Heading is historical; do not treat as a spec |

## Limits stated by the project

- "Check for Updates" only reports the version; no update server.
- Language option lists English only; strings are not translated.
- Some site icons look blurry because those sites publish only 16-32 px icons.
- Default `graphicsApi` is `software`, so the aurora shader and glow effects are disabled until the
  user picks OpenGL or Direct3D 11 and restarts (verified in `appsettings.cpp`, `Fx.qml`).

## Risks / things to be careful about (observed, not tested)

- Building or deploying while `Dockly.exe` runs fails on file locks (build.bat prints a hint).
- `build.bat` runs `git add -A` before committing: anything untracked (outside .gitignore) is
  included.
- `AppCore` is ~1.7k lines and `softwarescanner.cpp` ~1.1k lines; large single files.
- The Windows-only paths (`DockGuard`, `RunningApps`, `GlobalHotkey`, `PerfMonitor`) have no tests
  in `core_test.cpp` except `runningAppsTracksTheTestProcess`.
- `realMachineCoverageAndNaming` test depends on the machine it runs on.

## Not verified in the initial pass

- Line-by-line behaviour of `appcore.cpp`, `dockguard.cpp`, `perfmonitor.cpp`, `database.cpp` SQL
  beyond table creation, `theme.cpp` token values.
- QML page internals (`CategoryPage`, `SettingsPage`, `HomePage`, dialogs) and dock layout maths.
- Browser extension popup logic (`popup.js`) beyond the API client.
- That the project currently builds and that `core_test` passes (nothing was built or run).

## Housekeeping

- `.claude/codelens-real-world-development/SKILL.md` sits outside `.claude/skills`, so it is not
  registered as a skill.
- Quitting after the dock was shown logs `DockWindow.qml:278 ... dockVisible of null` (seen
  2026-11-27 on builds before and after the cleanup).
