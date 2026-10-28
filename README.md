# Dockly

A launcher / hub / shortcut plate for Windows, built with C++17 and Qt 6 (Qt Quick, GPU rendered).
One core library, one frontend (`Dockly.exe`):

```
scripts/  C++ code: include/ (headers), src/ (.cpp), main/ (entry points); build.ps1 / build.bat
qml/      Qt Quick frontend: honeycomb / orbit / grid layouts, radial ring, GLSL aurora shader
          core = SQLite, settings, launching, tray, theme (static lib); main/core_test.cpp = QtTest tests
```

## Interface

- **Honeycomb / Orbit / Classic grid** - switch from the top bar. Honeycomb and orbit are pan (drag) and zoom (wheel) maps of floating glass hexagons that tilt toward the pointer.
- **Radial ring** - the glowing orb (or `Ctrl+Space`, `F2`, middle click) opens a ring: sections on the inner ring, favourites on the outer one.
- **Search** - `/` or `Ctrl+F`; `Ctrl+1..7` jump between hubs.

## Connectors

The **Connectors** tab links Dockly with other apps:

| Connector | What it does |
|---|---|
| Chrome / Edge / Brave | Browser extension (`connectors/browser`): add the current page, a link or all tabs as web shortcuts. Dockly listens on `127.0.0.1` only, requires a pairing code (secret token), accepts only `http(s)` links and answers cross-origin requests only for browser extensions. Load it via `chrome://extensions` > Developer mode > Load unpacked. The popup shows the site a page will be filed under (and "Already in Dockly"), lets you pick the group, make a new site from the current host, choose individual tabs, and browse your sites; the toolbar badge shows a check for saved pages and `+` for pages one of your sites covers. Shortcuts: `Alt+Shift+D` adds the page, `Alt+Shift+T` adds all tabs. |
| Browser bookmarks | Pick bookmarks from Chrome, Edge or Brave and pin them. |
| GitHub | Pin any user's public repositories (no sign-in). |
| Visual Studio Code | Pin recent folders / workspaces; they open in VS Code (`vscode://`). |

## Sites

The **Sites** tab keeps links to the websites you use most (GitHub, ChatGPT, Claude, YouTube... or any you add). A site is a small rule you edit: a name, the hosts it covers, and how its links are grouped (one group, by first folder such as a GitHub owner, or by subdomain). Any web shortcut you add, from the browser extension, an import, a drop or by hand, is filed into its site's group automatically, and each site with links gets its own dock icon. When you have 5 or more loose links on a host that is not a site yet, Dockly offers to make it one. Rules are stored in `sites.json` in the data folder.

## GPU settings (Settings > Performance)

| Setting | Effect |
|---|---|
| Effects level: Low | flat gradient background, no motion effects |
| Effects level: Balanced | animated aurora shader at 30 fps, tilt and floating tiles |
| Effects level: High | aurora at full rate, star field, parallax, glow halos |
| Graphics backend | OpenGL, Direct3D 11 or Software (no GPU); applies after restart |
| Pause effects when in background | stops all animated effects while the window is not active |

## Build

```powershell
.\scripts\build.ps1            # build both apps into .\build
.\scripts\build.ps1 -Deploy    # also copy Qt runtime into .\dist (runs on any PC)
```

Needs Qt 6.11 (MinGW 64) + CMake + Ninja; adjust `$qtRoot` in `scripts/build.ps1` if Qt lives elsewhere.
Open the top-level `CMakeLists.txt` in Qt Creator to work on it there.

## Data: two SQLite files, read on demand

Stored in `%APPDATA%\Robin Gupta Studios\Dockly\` (override with the `LH_DATA_DIR` environment variable):

| File | Holds |
|---|---|
| `library.db` | your cards, collections (membership + order), recents |
| `catalog.db` | the *hidden* catalog of detected programs |
| `settings.json` | settings |
| `icons\`, `backups\` | favicon / thumbnail cache, exported backups |

Memory: views keep only a list of row **ids**; each visible card is read from SQLite when needed
(small row cache, cleared on every change). The catalog is never loaded wholesale: the picker
queries it with search + `LIMIT`.

Older `data.json` files are migrated automatically on first start (auto-detected programs go to the
catalog instead of becoming cards; the old file is renamed `data.json.migrated`).

## Software: hidden catalog, you choose what becomes a card

* Nothing is scanned at startup and detected programs are **not shown**.
* **Scan this PC** (Software tab or the Add Software dialog) reads only the Start Menu shortcut
  folders — it never walks whole drives — and stores the result in `catalog.db`.
* **Add Software** opens a searchable list of that catalog (tick the ones you want) or lets you
  browse for any `.exe`. Only ticked programs become cards.
* Deleting a card removes just the card; the program stays in the catalog.

## Editing cards

* Hover a card: **edit**, **favorite** and **delete** (SVG) buttons appear on it.
* Click a card: the **detail panel** opens (name, path/URL, browse, save, open, favorite,
  locate again, delete). Double-click / Enter opens the item, `F2` edits, right-click for more.

## Look and feel

* **Three skins** (Settings -> Appearance): *Neon* (glow), *Playful* (tinted rounded tiles),
  *Editorial* (thick outlines, hard shadows, serif headings), each in light or dark.
* **Icon-only navigation**: a slim rail (or a floating dock) with tooltips; `Ctrl+1..7` jump to a tab.
* **View toolbar** next to the search box on every page: Grid / List / Compact switch and a
  card-size slider (these moved out of Settings).
* **Custom SVG icons**: the whole icon set (navigation, toolbar, card buttons, warnings, the
  rocket logo / app icon) is drawn from SVG markup embedded in `scripts/src/iconpack.cpp`; any colour
  is applied at render time, so icons follow the skin.
* **Web shortcuts show the site's real icon**: the page's `<link rel="icon">` / apple-touch-icon,
  then `/favicon.ico`, cached on disk. Until it arrives (or if a site has none) the card shows a
  coloured letter tile - never a generic globe.
* **Bigger, roomier cards** with sharper icons (rendered at device resolution).
* **Animations**: hover lift + glow cross-fade, staggered card entrance, animated reordering,
  page fades, slide-in detail panel, pressed-button feedback. All switch off with the
  *Animations* setting.
* **OpenGL**: the scene graph renders with OpenGL by default (4x multisampling); pick another backend
  in Settings, or set `LH_NO_GL=1` to use Qt's default.

## Drag & drop

* Drop **files, folders or links** anywhere on the window: programs go to Software, media to
  Entertainment, other files to Documents (or the hub of the tab you are on, for unknown types),
  `http(s)` links to Web Shortcuts. Duplicates are skipped.
* Drag any **card onto a collection** (Collections tab) to add it.
* Drag **members inside a collection** to reorder them (the order is saved in `library.db`).

## Flowchart -> code

* **Initialization**: `AppCore::init()` loads settings, opens the databases (creating them on first run).
* **Hubs**: Home (favorites, recent, global search), Software, Documents, Entertainment, Web
  Shortcuts, Collections, Settings.
* **Item exists?**: every launch re-validates. A missing target or invalid URL shows a warning
  badge on the card and offers *Locate Again*, *Edit Path / URL* or *Delete*.
* **Background checks**: missing files / invalid URLs are checked on startup and every 5 minutes
  on a worker thread; card status updates live.
* **Settings**: everything in the flowchart's Settings tab (the auto-refresh option was removed:
  scanning is now strictly on demand).

## Notes / known limits

* *Check for Updates* just reports the version: no update server is configured.
* *Language* lists English only: strings are not translated yet, so the option is a placeholder.
* Website icons are fetched from the page's `<link rel="icon">` / apple-touch-icon, then
  `/apple-touch-icon.png` and `/favicon.ico`, and cached locally.
* Some icons look blurry (for example Google, Spotify and Hugging Face). This is a source-file
  size limit, not a rendering error: those sites publish only a 16-32px icon, which is scaled up
  to fill a tile. Sites that publish a larger icon look sharp.
* Command-line: `--minimized` (start in tray when tray is enabled), `--screenshot=file.png`
  and `--page=N` (diagnostics).
