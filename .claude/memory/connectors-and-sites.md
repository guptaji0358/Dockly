# Connectors and Sites

Sources: `scripts/include/connectors.h`, `sites.h`, `scripts/src/connectors.cpp`,
`connectors/browser/*`, `README.md`.

## Browser bridge (`Connectors`)

- Loopback-only `QTcpServer` on `127.0.0.1`, default port 47821 (`bridgePort`), enabled by
  `bridgeEnabled` (default false). Properties for QML: `bridgeRunning`, `bridgePort`,
  `pairingCode`, `received`, `busy`.
- **Pairing code** = `<port>.<token>`; the token is stored in `bridge.token` (random hex, regenerate
  with `regenerateToken()`).
- Every request needs header `Authorization: Bearer <token>`. CORS answers are only for browser
  extension origins; only `http(s)` links are accepted (README claim, covered by test
  `bridgeOnlyAcceptsPairedHttpLinks`).
- Endpoints seen in `connectors.cpp::handle`: `GET /ping`, `POST /match` (is a URL saved, which site
  would take it), `POST /site` (create a site from a host), `POST /add` (items with url, title,
  collection, icon, optional `group`, `noAuto`).
- `addFromJson` adds web shortcuts via AppCore; `addEntries` / `addEntriesToNewCollection` are the
  importer entry points ({name, target} entries).

## Importers (all return plain `{name, target, sub}` entries; UI lets the user choose)

- Browser bookmarks: `installedBrowsers()` ("chrome", "edge", "brave"), `browserBookmarks(browser)`,
  `bookmarksFromFile(path)` (Chrome-format JSON).
- VS Code: `vscodeRecent()` - folders/workspaces opened via `vscode://`.
- GitHub: `fetchGithub(user)` async -> `githubReady(entries, error)`; public repos, no sign-in.

## Browser extension (`connectors/browser`, MV3, v1.2.0)

Files: `manifest.json`, `background.js` (69 lines), `dockly.js` (API client: `ping`, `match`,
`addSite`, `add`), `popup.html/.css/.js` (258 lines). Permissions: activeTab, contextMenus, storage,
tabs; host permission `http://127.0.0.1/*`. Shortcuts `Alt+Shift+D` (add page) and `Alt+Shift+T`
(add all tabs). Pairing is stored in `chrome.storage.local` as `pair`. The extension is copied
next to the build (`DocklyApp` post-build step) and into `dist\resources\connectors` on deploy;
users load it unpacked via `chrome://extensions`.
Placement values sent to `/add`: `""` follow site rules, `"__none__"` leave loose, else a group name.

## Sites (`Sites`, `sites.json`)

- A Site = {id, name, hosts[], groupBy (one group | first folder | subdomain), autoFile, icon,
  color, optional own image}. Presets are seeded on first run (GitHub is one; README also names
  ChatGPT, Claude, YouTube).
- New web shortcuts matching a site are filed into a **group** (an ordinary collection, see
  `isSiteGroup`) by `autoFile(itemId)`. Disable per site (`autoFile`) or globally via
  `AppCore::setAutoFile(false)`.
- Views: `viewIds(siteIndex, search)` lists links of all groups for a site (category value
  `LH::SiteViewBase + n`); `dockEntries()` gives sites that have groups, each shown as a dock icon.
- Housekeeping: `looseCount()` / `organizeAll()` file loose links; `suggestions()` offers to create a
  site for hosts with 5+ loose links; `dismissSuggestion` remembers refusals; they are saved in
  `sites.json` under `dismissed`.
- Rules are edited in `SitesPage.qml` / `SiteDialog.qml`.

## Tests covering this area (`core_test.cpp`)

`bridgeOnlyAcceptsPairedHttpLinks`, `bookmarksAreParsedFromChromeFormat`,
`sitesFileLinksAutomatically`, `importerPickedGroupIsNotOverridden`,
`sitesCanBeDefinedByTheUser`, `importersAreSafeToCall`.
