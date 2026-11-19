# Known issues, doc/code conflicts, unverified areas

Status as of 2026-11-19 (initial analysis). Nothing here was fixed; these are observations.

## Documentation vs code conflicts (code wins)

| Where | Doc says | Code says |
|---|---|---|
| `.claude/CLAUDE.md` | `build.bat` does a clean rebuild | `build.bat` is incremental; only `build.bat clean` wipes `build/` and `dist/` |
| `README.md` "Software" section | Scan reads only Start Menu shortcut folders | Scanner also uses the shell Apps folder, uninstall registry keys and App Paths (software-scanner.md) |
| `README.md` intro | "one core library, one frontend" | `qml/` builds `DocklyApp.dll`; `Dockly.exe` is a loader stub (architecture.md) |
| `README.md` | `Ctrl+1..7` jump to tabs | `Main.qml` binds `Ctrl+1..9` (9 tabs) |
| `README.md` "Look and feel" | skins/nav described as a slim rail or floating dock | Not re-verified; `navigation` setting is sidebar or dock |
| README/header comments | "both frontends" (`appcore.h`, `itemmodel.h`) | Only the QML frontend exists in the repo now |
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
  included. `.claude/memory/SKILL.md` (a stray Selenium log) is currently tracked in git.
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

- `.claude/memory/SKILL.md` and `.claude/codelens-real-world-development/SKILL.md` sit outside
  `.claude/skills`, so neither is registered as a skill; the first is not a skill at all (a log).
- Working tree had an uncommitted change to `scripts/build.ps1` at session start.
