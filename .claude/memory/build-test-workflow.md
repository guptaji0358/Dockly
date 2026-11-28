# Build, test, commit workflow

## Toolchain (from `scripts/build.ps1`)

Qt **6.12.0 MinGW 64** at `C:\Qt\6.12.0\mingw_64`, MinGW 13.1 (`C:\Qt\Tools\mingw1310_64`), Ninja,
CMake under `C:\Qt\Tools`. Change `$qtRoot` in `build.ps1` if Qt lives elsewhere. Release build in
`.\build`. Qt components: Core Gui Widgets Network Concurrent Sql Svg Qml Quick QuickControls2
ShaderTools GuiPrivate (+ Test for `core_test`).

## Commands

```powershell
.\scripts\build.ps1            # incremental build into .\build
.\scripts\build.ps1 -Deploy    # also deploy Qt runtime into .\dist
build.bat                      # build + deploy, then commit/push if commit.txt exists, then launch dist\Dockly.exe
build.bat nodeploy             # build only, launches build\Dockly.exe
build.bat clean                # wipe build and dist first (also: clean nodeploy)
```

- Deploy is incremental: `windeployqt` runs only if `dist\.qt-deployed` is missing; `Dockly.exe`
  and `DocklyApp.dll` are copied only when their hash changed. Close a running Dockly before
  building or the linker/copy fails.
- Open the top-level `CMakeLists.txt` in Qt Creator for IDE work.

`.\scripts\build.ps1 -Native` also builds the Rust layer (cargo needed; seen with MSYS2 ucrt64
Rust 1.100, whose own gcc must link it). Without the switch `DOCKLY_NATIVE` is set OFF.
`build.bat` does not pass it. With it, `ctest` adds `native_test` and `native_rust`.

## Tests

Runs with `LH_DATA_DIR` set (tests included) leave the `HKCU\...\Run\Dockly` autostart entry
alone since 2026-11-28; builds older than that rewrite it.

`guard_test` (16) and `guard3_test` (23) are separate executables; `core_test` (QtTest, `scripts/main/core_test.cpp`, ~1390 lines, 57 test cases plus `initTestCase`) is built with the
project and registered via `add_test`. Run `build\core_test.exe` (needs the Qt DLLs on PATH) or
`ctest` in `build`. Groups: items/dedupe/status/collections, scanner and naming (synthetic and
real-machine), catalog, backup/restore, drag-drop routing, quick search ranking and `#` filters,
running apps, colour themes, icon pack, search escaping, bridge security, bookmarks, sites,
process inspection, Background Process Guard (card switch, master switch persistence, and the
SQLite `guard` column migration from an old-schema `library.db`).
`initTestCase` sets `LH_DATA_DIR` to a temp dir, so tests never touch the real profile.
Verification status: Stage 2 (process inspection, guard UI, guard persistence/migration tests) was
verified and accepted by the user. This memory pass did not re-run the suite; exact pass counts
are not recorded here.

## Commit routine (project `.claude/CLAUDE.md`)

- Write the commit message to `commit.txt` (git-ignored) before running `build.bat`: subject line,
  blank line, body, then the Co-Authored-By trailer required by CLAUDE.md.
- `build.bat` on success does `git add -A`, `git commit -F commit.txt`, `git push` (skipped when
  nothing is staged or `commit.txt` is missing; build failure commits nothing). It deletes
  `commit.txt` once the commit succeeds (even if the push then fails), so write a new one each time.
- User-level rule (`~/.claude/rules/git-commit-push.md`): when asked to commit/push, use the LIAS
  `git-commit-push` automation if the project has it. This project has no `.claude/lias` folder,
  so say so and ask before using plain git.
- `CLAUDE.md` also defines an independent-tester policy: when evaluating, do not fix bugs; report
  with ratings and evidence; use CodeLens MCP for reading code.

## Git facts

Remote `origin`; branches seen: `main`, `feature/edge-dock`, `fix/catalog-duplicates-stale`,
`fix/tile-icons-and-labels`. About 56 commits. Recent work: scanner rewrite around the shell Apps
folder, scanner tests, dynamic software names.

## Gotchas

- `.claude/CLAUDE.md` says `build.bat` does a clean rebuild; the real `build.bat` is incremental
  and cleans only with the `clean` argument (known-issues.md).
- Any `build-*/` folder is ignored; never commit build output. A `build/` configured under another
  path makes CMake refuse to configure; since 2026-11-28 `build.ps1` detects that and recreates
  `build/` itself.
- `build\Dockly.exe` is only the loader and needs `resources\` beside it (use `dist\` or a copy).

## Kept on purpose although the app does not call them (2026-11-27)

`Guard::diffSnapshots`, `ProcInspect::stillSameProcess` (polling design, tested), settings key
`guardEnforce` (reserved, tested), `DryRunReport::executedAnything`, `Database::itemCount` and
`AppCore::lastScanReport` (test hooks), diagnostic CLI flags, legacy data migration.
