# Core C++ modules (`scripts/include/*.h`, `scripts/src/*.cpp`)

All in static lib `lhcore` except the three image providers (in `DocklyApp`). Line counts are
approximate sizes at time of analysis.

| Module | Role |
|---|---|
| `lhtypes.h` | `LH` namespace enums: Category, Status, Mode (Category/Favorites/Recent/Members), LaunchResult; `SiteViewBase = 100` |
| `item.h/.cpp` | `Item` struct, JSON (de)serialisation, `normalizedTarget`, `normalizedLaunch` (target + args = card identity), `parseTags` (lowercase, unique, sorted, max 12) |
| `appsettings` | Flat key/value store; `defaults()` is the authoritative key list; `set()` ignores unknown keys and converts to the default's type; JSON load/save via `QSaveFile` |
| `database` | SQLite access (see data-and-settings.md): items, members, recents, catalog, backup export/import |
| `appcore` (1.7k lines) | Central façade, singleton `AppCore::instance()`. Owns settings, theme, Database, Connectors, Sites, RunningApps, Gamepad, PerfMonitor. Item CRUD, launching, quick search, drag-drop, collections, catalog scan, validation, backup, icon caches, favicon fetch, memory trim. Most `Q_INVOKABLE`s that QML calls live here |
| `itemmodel` | `QAbstractListModel` over id lists; properties category/mode/search/collectionId/tag; roles include Running and Tags. Registered to QML as `LH.ItemModel` |
| `theme` | Resolves skin (neon/playful/editorial), dark/light, palette, accent and shape tokens into QML-bindable properties; `palettes()`, `categoryColor()`, `cardColor()` |
| `softwarescanner` | Program discovery and naming (software-scanner.md) |
| `sites` | User-editable site rules stored in `sites.json` (connectors-and-sites.md) |
| `connectors` | Loopback HTTP bridge + importers (connectors-and-sites.md) |
| `dockguard` | Edge-dock behaviour engine: fullscreen detection, pointer-intent scoring, click-through, idle hide, "calm apps", lifting the dock above topmost windows. Supports a second window/zones for a bent or split dock |
| `runningapps` | Polls top-level windows (about every 2 s, only when enabled); maps exe path to item ids; `focus(id)` brings a window forward |
| `perfmonitor` | Samples CPU/RAM/GPU (PDH); drives Lite mode (manual or auto with per-resource thresholds, a few seconds hot/cool hysteresis) |
| `gamepad` | XInput loaded at runtime; stick/d-pad/buttons/chord signals; polled only when enabled |
| `hotkey` | `GlobalHotkey`: Windows `RegisterHotKey` + native event filter; `setSequence` returns false on parse failure or conflict |
| `tray` | `TrayController` (`QSystemTrayIcon`); signals to show window, reveal dock, open item, quit |
| `iconpack` | SVG icon set embedded as markup (nav, toolbar, card buttons, logo); recoloured at render time |
| `item/icon/screen imageprovider` | QML `image://item/`, `image://icon/`, `image://screen/` sources |

## AppCore key behaviours (verified in headers; details in `appcore.cpp`)

- `launch(id)` returns an `LH::LaunchResult`; brings an already-open program forward when
  `focusRunning`. `launchNew` always starts a fresh instance. `launchAll(collection)` spaces items
  by `workspaceDelay` and skips missing ones.
- `quickSearch(query, limit)` returns rows `{id, name, subtitle, category, categoryName, iconSource}`;
  `#` queries return filter rows (`filter = true`, `token`).
- Web shortcuts are auto-filed into their Site group unless `setAutoFile(false)` (importers that
  pick their own group switch it off).
- `changed()` clears the row cache and emits `itemsChanged`; saves are debounced via `m_saveTimer`.
- `addDropped` routes by type: programs -> Software, media -> Entertainment, other files ->
  Documents (or the current hub for unknown types), `http(s)` links -> Web; duplicates skipped.
- Lite mode (`applyLite`) slows Dockly's own background checks and lightens icons.
- Backup/restore: `backupTo`/`restoreFrom` use `Database::exportAll/importAll` (replaces library).

## Conventions seen in code

- C++17, Qt idioms (signals/slots, `Q_PROPERTY` for QML binding, `Q_INVOKABLE` for actions).
- Pure helpers in `softwarescanner` are separated from Win32 collectors so tests can feed
  synthetic records (`mergeRecords`, `rankNames`, `looksLikeNoise`).
- Windows-only code is guarded by `#ifdef Q_OS_WIN` with stub fallbacks.
