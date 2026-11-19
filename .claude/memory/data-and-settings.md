# Data, settings, environment, CLI

## Data folder

Default `%APPDATA%\Robin Gupta Studios\Dockly\` (Qt `AppDataLocation`; org/app names set in
`app_main.cpp`). Override with env `LH_DATA_DIR` (separate profile, tests, portable use; legacy
data migration is skipped when it is set).

| Path | Content |
|---|---|
| `library.db` | tables `items`, `members` (collection membership + order), `recents` (WAL, foreign keys on) |
| `catalog.db` | hidden catalog table `apps`; `PRAGMA user_version` currently 5 (older versions are rebuilt) |
| `settings.json` | all settings, indented JSON, written with `QSaveFile` |
| `sites.json` | Sites rules (connectors-and-sites.md) |
| `bridge.token` | browser-bridge secret (>= 24 chars; regenerated if shorter) |
| `dockly.log` | Qt/QML messages; deleted when larger than 512 KB at startup |
| `icons\`, `backups\` | favicon / thumbnail cache; exported backups |
| `data.json` | legacy store; imported once when `library.db` is new, then renamed `data.json.migrated` |

Rendered-icon disk cache is separate, under Qt `CacheLocation\pix`, cleared at start.

Evidence: `database.cpp` lines ~44-80, `appcore.cpp::init`, `app_main.cpp`.

## Settings keys (defaults in `scripts/src/appsettings.cpp::defaults()`)

Only keys present in `defaults()` can be set; a new setting needs an entry there first.

- **General**: `startWithWindows`, `minimizeToTray`, `defaultTab` (home|software|custom),
  `language` (English only), `defaultBrowser` (empty = system), `confirmDelete`,
  `checkMissingOnStartup`, `recentEnabled`, `maxRecent` (12).
- **Look**: `theme` (light|dark|system), `style` (neon|playful|editorial), `navigation`
  (sidebar|dock), `accent`, `palette`, `cardSize`, `viewStyle` (grid|list|compact), `layoutMode`
  (hex|orbit|grid), `animations`, `floatingTiles`.
- **Performance**: `gpuLevel` (low|balanced|high), `graphicsApi` (default **software**; opengl|d3d11|
  software, needs restart), `pauseWhenInactive`, `memoryIdleTrim` (30 s), `liteMode`, `liteAuto`,
  `liteCpu` 50, `liteGpu` 60, `liteRam` 60.
- **Launcher / workspaces**: `launcherEnabled`, `launcherHotkey` (Alt+Space), `launcherPos`,
  `workspaceDelay` (400 ms), `startupWorkspace`.
- **Dock**: `dockEnabled` (true), `dockMode` (categories|favorites), `dockEdge` (right),
  `dockEdge2`, `dockSplit`, `dockPos`, `dockHotkey` (Ctrl+Alt+D), `dockAutoHide`,
  `dockHideFullscreen`, `dockCollapseDelay` (100 ms), `dockIdleHide` (30 s), `dockCalmApps`
  (list of exe names such as browsers and creative tools), `dockOpacity` (40).
- **Awareness / input**: `showRunning`, `focusRunning`, `gamepadEnabled`.
- **Connector**: `bridgeEnabled` (false), `bridgePort` (47821).

QML reads `core.settings.values.<key>`; changes emit `AppSettings::changed(key)` which app_main
uses to re-register hotkeys (`dockHotkey`, `dockEnabled`, `launcherHotkey`, `launcherEnabled`).

## Background Process Guard (Stage 1 only: stores the switch, enforces nothing)

- `items.guard INTEGER NOT NULL DEFAULT 1` (added by `ALTER`, so existing cards become 1). `Item::guard`
  defaults true; backup JSON writes `"guard": false` only when off, a missing key means on.
- Settings: `guardEnabled` (true, master switch) and `guardEnforce` (false, reserved, unused).
- `AppCore::setGuard(id, on)` works on Software cards only; `itemInfo` adds `guard` and `guardState`
  (`none` | `off` | `notEnforced`; never "protected" until enforcement exists). UI: card context menu
  entry in `Main.qml`, one checkbox in `SettingsPage.qml`. Test: `backgroundGuardSwitchDefaultsOnAndSurvivesBackup`.
- Stage 0 findings (StartupApproved format, task and packaged-app permissions) were reported in chat only.

## Environment variables

`LH_DATA_DIR` (data folder), `LH_NO_GL=1` (skip the graphics-API pre-read, use Qt's default).

## Command-line flags (`app_main.cpp`)

`--minimized` start hidden in tray; `--window` open main window even with dock enabled.
Diagnostics: `--screenshot=file` (also skips the single-instance probe and the dock), `--page=N`,
`--ring`, `--demo` (sample cards when library empty), `--dialog=software`, `--dblclick=x,y`,
`--dockshot=file`, `--launchershot=file`, `--launcherquery=text`.

## Item identity and dedupe

Program cards are identified by `Item::normalizedLaunch(target, args)`: same exe with different
arguments is a different card. Other items dedupe by normalised target per category
(`Database::findByNorm`). Packaged (Store) apps use target `shell:AppsFolder\<AUMID>`.
