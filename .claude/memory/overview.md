# Overview

Dockly is a Windows launcher / hub / shortcut plate written in C++17 with Qt 6 (Qt Quick, GPU
rendered). Version 1.0.0 (`CMakeLists.txt`). Author/org string: "Robin Gupta Studios". Windows only
in practice (Win32 APIs, MinGW kit). Evidence: `README.md`, `CMakeLists.txt`, `scripts/main/app_main.cpp`.

## Item model

An **Item** (`scripts/include/item.h`) is one card: id, category, name, target (file path or URL),
args (shortcut arguments, programs only), custom icon, favorite, tags, status, lastOpened.

Categories (`lhtypes.h`): Software, Document, Entertainment, Web, Collection. `SiteViewBase = 100`
marks pseudo-categories for per-site views. Status: Normal / Warning / Missing (validated on startup
and every 5 minutes on a worker thread when not in Lite mode).

## Main window tabs (`qml/Main.qml`)

Home, Software, Documents, Entertainment, Web, Collections, Sites, Connectors, Settings
(`Ctrl+1..` jumps). Pages are lazily loaded (`win.keep(i)`) and unloaded on memory trim.

## Three launch surfaces

1. **Main window** - frameless `ApplicationWindow`; layouts: hex (honeycomb), orbit, grid; list /
   compact view styles; radial ring (`Ctrl+Space`, `F2`, middle click, orb).
2. **Edge dock** (`DockWindow.qml`) - slim frameless strip on a screen edge, on by default
   (`dockEnabled`), opened by hover or global hotkey `Ctrl+Alt+D`. Can bend into an L or split
   left+right. Tray icon or its gear opens the full window.
3. **Quick launcher** (`QuickLauncher.qml`) - type-to-launch box, global `Alt+Space`. Ranking: name
   prefix, word prefix, contains, path contains; favourites win ties; empty box shows recents then
   favourites; `#` lists filters (categories, then user tags). `Ctrl+Enter` shows in folder.

## Features worth knowing

- **Software is a hidden catalog**: scanning is on demand; only ticked programs become cards
  (see software-scanner.md). Deleting a card leaves the catalog entry.
- **Workspaces**: a collection launches all members one after another (`workspaceDelay`), optional
  `startupWorkspace` opens 4 s after start.
- **Sites**: rules that file web links into groups and give a dock icon (connectors-and-sites.md).
- **Connectors**: browser extension bridge, bookmarks, GitHub repos, VS Code recents.
- **Running-app awareness**: marks open programs, focuses an existing window instead of relaunching
  (`RunningApps`, settings `showRunning`, `focusRunning`).
- **Quick actions**: run as administrator, run with arguments, copy path/link, show in folder.
- **Drag & drop**: files/folders/links routed to the matching hub; drop on a collection or the dock.
- **Gamepad**: XInput controller drives the radial ring (only while the main window is in front).
- **Lite mode**: manual or automatic (CPU/GPU/RAM thresholds) - no animations/glow, cheaper icons.
- **Appearance**: skins neon / playful / editorial, light/dark/system, colour palettes (ocean,
  forest, sunset, rose, violet, graphite), custom accent; effects level low/balanced/high.
- **Backup/restore** of the library as JSON; data migration from older `data.json`.

## Not implemented (by README/code)

No update server (`checkForUpdates` only reports version); language list is English only.
See known-issues.md.
