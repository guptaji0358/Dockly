# Software scanner (`scripts/include/softwarescanner.h`, `scripts/src/softwarescanner.cpp`, ~1100 lines)

Purpose: find installed programs on demand and store them in the hidden `catalog.db`; the user
chooses which become cards. Nothing is scanned at startup. Triggered by `AppCore::refreshSoftware()`
(worker thread, `m_scanWatcher`), from the Software tab or Add Software dialog.

## Pipeline (`scanInstalledSoftwareReport()`)

1. **Providers** each append `RawRecord`s (source enum `AppSource`: StartMenu, Uninstall, AppPaths,
   Packaged, AppsFolder). Order matters, first record for a program wins:
   `collectAppsFolder` (shell Apps folder = primary source and coverage ground truth) ->
   `collectStartMenu` -> `collectUninstall` -> `collectAppPaths`.
2. `mergeRecords(records, fillFromExe=true, &rejected)`: drops noise, keeps first record per launch
   path (case-insensitive), later records only fill empty fields and add their source bit to
   `ScannedApp::sources`; resolves display names; fills publisher/version/installDir from exe info.
3. `ScanReport` records apps, rejections (stable reason codes such as `noise-name`, `noise-target`,
   `installer-target`, `unresolved-name`), per-source counts, shell-entry coverage, elapsed ms.
   `ScanReport::toText()` renders it; `AppCore::lastScanReport()` exposes it.
4. `Database::replaceCatalog` swaps the catalog; `pruneCatalog` drops rows whose program vanished
   at startup (packaged targets are kept; only a scan revalidates them).

## Key helpers (pure, unit-tested)

- `looksLikeNoise(name, target)` (whole-word matching), `looksLikeInstallerExe`.
- `findMainExeInDir` - file names and version info only, **never file size**.
- `exeFromRegistryValue` (+ args overload), `expandEnvironment` (undefined variable => unusable).
- `resolveShortcut(.lnk)` -> {target, args, workDir}; args are preserved on cards.
- Naming: `NameCandidate` + `rankNames` choose by generic quality signals (`nameQuality`), not
  per-product rules; `stripVersionDecoration`; `resolveFriendlyName`, `executableDisplayName`,
  `looksLikeExecutableName`. Recent commit: names resolved dynamically instead of exe names.
- `isLaunchableTarget` (.bat, .cmd, .msc, .lnk, .url besides .exe).

## Packaged (Store/MSIX/UWP) apps

Target form `shell:AppsFolder\<AUMID>`. API: `isPackagedTarget`, `packagedAppId`,
`packagedAppExists`, `launchPackagedApp`, `packagedAppIcon`. AUMID comparison is case- and
separator-stable (`normalizeAumid`). Non-Windows builds get empty stubs.

## Tests (`core_test.cpp`)

Synthetic-fixture tests: registryValueParsing, noiseRules, mergeKeepsFirstRecordPerExe,
mergeFillsMissingMetadataFromLaterSources, nameRankingIsGeneric, mainExeNeverChosenBySizeAlone,
mergeReportsRejectionReasons, argumentsAndNonExeTargetsSurvive, aumidIdentityIsCaseAndSeparatorStable,
crossSourceDuplicatesMerge, catalogRefreshReflectsCurrentState, scanFillsCatalogOnlyOnDemand,
catalogRefreshNeverTouchesUserCards, catalogIsHiddenUntilPicked. `realMachineCoverageAndNaming`
runs against the real machine, so its result depends on the installed software.

## Gotchas

- Same exe with different shortcut arguments stays as separate catalog/card entries.
- Deleting a card never removes the catalog entry.
