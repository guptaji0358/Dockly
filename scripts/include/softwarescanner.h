#pragma once

#include <QImage>
#include <QString>
#include <QStringList>
#include <QVector>

#include <functional>

struct ScannedApp {
    QString name;
    QString target; // resolved .exe path
    QString publisher;
    QString version;
    QString installDir;
    int sources = 0; // bitmask of 1 << int(AppSource): every provider that reported this program
};

// Where a raw record was discovered.
enum class AppSource { StartMenu, Uninstall, AppPaths, Packaged };

// Packaged (Store/MSIX/UWP) apps have no usable .exe path; they are launched through the shell with
// their AppUserModelID, stored as the target "shell:AppsFolder\<AUMID>".
bool isPackagedTarget(const QString &target);
QString packagedAppId(const QString &target);      // the AUMID, or an empty string for any other target
bool packagedAppExists(const QString &target);     // still registered (not uninstalled)?
bool launchPackagedApp(const QString &target);
QImage packagedAppIcon(const QString &target, int size); // null image when the shell has none

// One candidate produced by a discovery provider, before merging.
struct RawRecord {
    AppSource source = AppSource::StartMenu;
    QString name;                        // name as registered (may just be the exe name)
    QString launch;                      // what to launch: a .exe, or a .lnk that has no resolvable .exe
    std::function<QString()> resolveName; // optional: final human-readable name (may read file version info)
    QString publisher;
    QString version;
    QString installDir;
};

// Merges raw records in the given order: drops noise, keeps the first record per launch path
// (case-insensitive) and resolves display names. Later records for the same program only fill
// fields the earlier ones left empty, and add their source to `sources`.
// With `fillFromExe`, publisher/version still missing are read from the exe's version info and a
// missing install directory defaults to the exe's folder. Pure except for resolveName/fillFromExe.
QVector<ScannedApp> mergeRecords(const QVector<RawRecord> &records, bool fillFromExe = false);

// Publisher (CompanyName) and version (ProductVersion, else FileVersion) from an exe's version info.
struct ExeInfo { QString publisher; QString version; };
ExeInfo executableInfo(const QString &exePath);

// Extracts an .exe path from a registry value such as `"C:\x\a.exe",0` or `C:\x\a.exe,-3`.
// Returns an empty string when the value holds no .exe path.
QString exeFromRegistryValue(const QString &value);

// Picks the likeliest main executable for a product from the exe file names found in its install
// folder (exact name match first, then partial; updaters/helpers/installers only as a last resort).
// Returns an empty string when nothing matches.
QString pickMainExe(const QString &displayName, const QStringList &exeFileNames);

// True for uninstallers, installers, readmes, help shortcuts and the like.
bool looksLikeNoise(const QString &name, const QString &target);

// Scans the Start Menu shortcuts (all users + current user), then the registry
// (Uninstall entries and App Paths), for installed programs.
// Safe to call from a worker thread.
QVector<ScannedApp> scanInstalledSoftware();

// Human-readable name of an executable (version-info FileDescription/ProductName),
// or an empty string when it has none.
QString executableDisplayName(const QString &exePath);

// True when `name` is just the executable's file name (e.g. "chrome", "msedge.exe", "Code_Helper").
bool looksLikeExecutableName(const QString &name, const QString &exePath);

// Best human-readable name for an executable, found dynamically: `candidate` if it is already
// a real name, else version info, else the install folder, else a prettified file name.
QString resolveFriendlyName(const QString &candidate, const QString &exePath);
