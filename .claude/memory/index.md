# Dockly project memory – index

Memory is a map, not the truth: current source and config win. Verify before relying on a detail,
and fix the memory when it disagrees. Every file here stays at 200 lines or fewer.

Last full pass: 2026-11-19 (initial analysis; not every `.qml` file was read line by line).

## Files

| File | Read it when you need... |
|---|---|
| [overview.md](overview.md) | What Dockly is, its hubs, features, and user-visible behaviour |
| [architecture.md](architecture.md) | CMake targets, DLL/launcher packaging, startup sequence, windows, memory model |
| [core-modules.md](core-modules.md) | Responsibility of each C++ class in `scripts/` (AppCore, Database, DockGuard...) |
| [data-and-settings.md](data-and-settings.md) | Data folder, SQLite files, settings keys, env vars, CLI flags |
| [qml-frontend.md](qml-frontend.md) | The QML app: main window, dock window, quick launcher, pages, effects |
| [connectors-and-sites.md](connectors-and-sites.md) | Browser extension bridge, importers, Sites rules |
| [software-scanner.md](software-scanner.md) | How installed programs are discovered, merged, named |
| [build-test-workflow.md](build-test-workflow.md) | Build, deploy, tests, commit routine, repo conventions |
| [process-inspection.md](process-inspection.md) | Stage 2 read-only process discovery, attribution, Inspect dialog |
| [known-issues.md](known-issues.md) | Verified limits, doc/code conflicts, unverified areas |

## Task shortcuts

- **Add or change a setting**: data-and-settings.md (defaults live in `appsettings.cpp`), then qml-frontend.md.
- **New card/launch behaviour**: core-modules.md (AppCore), data-and-settings.md (schema).
- **Dock behaviour**: qml-frontend.md (`DockWindow.qml`) plus core-modules.md (DockGuard).
- **Browser extension / bridge**: connectors-and-sites.md.
- **Missing or wrongly named programs**: software-scanner.md.
- **Build fails or release/deploy question**: build-test-workflow.md.

## Cross-links

- AppCore owns nearly everything and is exposed to QML as the `core` context property
  (architecture.md -> core-modules.md -> qml-frontend.md).
- Settings keys are shared by C++ (`AppSettings`) and QML (`core.settings.values.<key>`).

## Not part of this memory

`.claude/memory/SKILL.md` is an unrelated Selenium run log (a Tindog bot), not a skill and not
project memory. Leave it alone unless the user decides what to do with it.
