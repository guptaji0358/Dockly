# QML frontend (`qml/`)

One QML module `DocklyUI` (version 1.0, `NO_PLUGIN`) inside `DocklyApp.dll`; every file must be
listed in `qml/CMakeLists.txt` `QML_FILES` or it will not be compiled in. `components/Fx.qml` is a
singleton (`QT_QML_SINGLETON_TYPE`). QML imports `LH` for enums and `ItemModel`.

## Root windows (loaded in this order by `app_main.cpp`)

| File | Window | Notes |
|---|---|---|
| `Main.qml` (~890 lines) | `ApplicationWindow`, frameless, 1240x780, min 900x560 | Tab list, search box with `#tag` filters, `StackLayout` of lazily loaded pages, radial `Ring`, DropArea, dialogs. Exposes `edit(id)` and `launch(id)` invoked from C++ |
| `DockWindow.qml` (~1140 lines) | frameless edge strip | Collapsed = thin accent line; slides open on hover/hotkey. Modes: categories (hover a category for a grid) or favorites strip. Bends into an L in a top corner (second window) or splits left+right. Drag grip to reposition; ghost strips show the snap spot. Exposes `reveal()` and `toggle()` |
| `QuickLauncher.qml` (~430 lines) | type-to-launch box | `toggle()`, `openWith(text)`; uses `core.quickSearch` |

If fewer than three root objects load, startup fails with a message pointing to `dockly.log`.

## Pages (`qml/pages/`)

`HomePage` (favorites, recents, global search), `CategoryPage` (Software/Documents/Entertainment/
Web, parameterised by category), `CollectionsPage`, `SitesPage`, `ConnectorsPage`, `SettingsPage`.

## Components (`qml/components/`)

- Cards/layout: `Card`, `Tile` (floating glass hexagon, tilts toward pointer), `ItemGrid`,
  `ItemSpace` (honeycomb/orbit pan-zoom map), `DetailPanel`, `ViewToolbar`.
- Navigation/chrome: `Ring` (radial command ring, sections inner / favourites outer), `OrbButton`,
  `Glass`, `HudField`, `AppButton`, `Icon`, `SiteIcon`.
- Background/effects: `Nebula` (gradient / aurora shader / star field by GPU level),
  `Fx` (singleton deciding which effects run), `shaders/aurora.frag` (compiled to `.qsb`).
- Settings widgets: `SettingGroup`, `SettingCheck`, `SettingCombo`.

## Dialogs (`qml/dialogs/`)

`ItemDialog`, `AppDialog`, `PickDialog`, `PickSoftwareDialog` (searchable catalog picker),
`ChoiceDialog`, `ArgsDialog` (run with arguments), `SiteDialog`, `SiteIconPromptDialog`,
`ConnectorPicker`.

## Effects and Lite mode (`Fx.qml`)

- `low`: flat gradient, no motion. `balanced`: aurora at 30 fps (interval 33 ms), tilt, floating
  tiles. `high`: aurora ~60 fps (16 ms), stars, parallax, halo glow.
- `Fx.lite` mirrors `core.perf.lite`; aurora/glow/stars are off in Lite mode and with the Software
  renderer (`softwareRenderer` is set by `Nebula` from `GraphicsInfo.api`).
- Effects run only while the window is active unless `pauseWhenInactive` is false.
- Note the default `graphicsApi` is `software`, so by default the aurora shader is disabled.

## Patterns

- Theme tokens: `readonly property var th: core.theme`, then `th.bg`, `th.accent`, etc.
- Page unloading: `Main.qml` keeps `visited`; `core.memoryTrimmed` resets it to the current tab.
- Search box supports `#software`, `#docs`/`#documents`, `#media`/`#entertainment`, `#links`/`#web`,
  `#workspaces`/`#collections`, plus user tags (`userTags` from `core.tagList(-1)`).
- Keyboard: `Ctrl+Space`/`F2` ring, `/` or `Ctrl+F` search, `Ctrl+1..8` tabs (9 tabs; Settings has no shortcut), `F2` edit card
  (README), `Enter`/double-click opens an item.

## Unverified

Per-page internals (`CategoryPage`, `SettingsPage`, dialogs) and the full dock layout maths were
not read in detail in the initial pass.
