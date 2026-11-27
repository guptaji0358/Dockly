#pragma once

#include <QImage>
#include <QString>
#include <QHash>
#include <QStringList>
#include <QVector>

#include <functional>

struct ScannedApp {
    QString name;
    QString target; // resolved .exe path
    QString publisher;
    QString version;
    QString installDir;
    QString args;   // command-line arguments preserved from the shortcut that advertised the program
    int sources = 0; // bitmask of 1 << int(AppSource): every provider that reported this program
};

// Where a raw record was discovered.
enum class AppSource { StartMenu, Uninstall, AppPaths, Packaged, AppsFolder };

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
    QString args;                        // shortcut arguments, kept verbatim
};

// Merges raw records in the given order: drops noise, keeps the first record per launch path
// (case-insensitive) and resolves display names. Later records for the same program only fill
// fields the earlier ones left empty, and add their source to `sources`.
// With `fillFromExe`, publisher/version still missing are read from the exe's version info and a
// missing install directory defaults to the exe's folder. Pure except for resolveName/fillFromExe.
struct Rejection {
    AppSource source = AppSource::StartMenu;
    QString name;
    QString launch;
    QString reason; // stable machine-readable code: noise-name, noise-target, installer-target, unresolved-name, ...
};
QVector<ScannedApp> mergeRecords(const QVector<RawRecord> &records, bool fillFromExe = false,
                                 QVector<Rejection> *rejected = nullptr);

// --- generic display-name ranking --------------------------------------------------------------
// Each source proposes a name with a prior (how much that source is usually trusted). The winner is
// chosen by generic quality signals only (does it look like a file name, a version string, an OS
// component; do other sources agree with it) -- never by product-specific rules.
struct NameCandidate {
    QString text;
    int prior = 0;
};
int nameQuality(const QString &name, const QString &exePath = {});          // higher = more human-readable
QString rankNames(const QVector<NameCandidate> &candidates, const QString &exePath = {}); // best, or empty
// Strips trailing version / architecture decorations: "Foo 1.2.3 (x64)" -> "Foo".
QString stripVersionDecoration(const QString &name);

// --- scan diagnostics ----------------------------------------------------------------------------
struct ScanReport {
    QVector<ScannedApp> apps;
    QVector<Rejection> rejected;
    QHash<int, int> seenBySource;   // AppSource -> raw records produced
    QHash<int, int> keptBySource;   // AppSource -> apps that list this source
    int shellEntries = 0;           // entries enumerated in the shell Apps folder (ground truth for coverage)
    int shellEntriesKept = 0;       // ...of which are in `apps`
    qint64 elapsedMs = 0;
    QString toText() const;
};
// Scans the shell Apps folder, the Start Menu shortcuts and the registry (Uninstall entries and
// App Paths) for installed programs. Safe to call from a worker thread.
ScanReport scanInstalledSoftwareReport();

// Resolves a .lnk: target, arguments and working directory (empty target when unresolvable).
struct LinkInfo { QString target; QString args; QString workDir; };
LinkInfo resolveShortcut(const QString &lnkPath);

// True for launch targets Dockly can start through the shell besides .exe (.bat, .cmd, .msc, .lnk, .url ...).
bool isLaunchableTarget(const QString &path);

// Expands %VAR% references. `ok` is false when a variable is undefined (the value is then unusable).
QString expandEnvironment(const QString &value, bool *ok = nullptr);

// Like exeFromRegistryValue, but also returns arguments that follow the exe (`"C:.exe" --flag`).
QString exeFromRegistryValue(const QString &value, QString *args);

// Looks into an install folder (and one level of sub-folders) for the main executable of a product.
// Uses file names and version-info (never file size). Installers/uninstallers/redistributables are rejected.
QString findMainExeInDir(const QString &dir, const QString &displayName, const QString &publisher = {});
// True for setup/installer/uninstaller/redistributable executables, from file name and version info.
bool looksLikeInstallerExe(const QString &exePath);

// Publisher (CompanyName) and version (ProductVersion, else FileVersion) from an exe's version info.
struct ExeInfo { QString publisher; QString version; };
ExeInfo executableInfo(const QString &exePath);

// Extracts an .exe path from a registry value such as `"C:\x\a.exe",0` or `C:\x\a.exe,-3`.
// Returns an empty string when the value holds no .exe path.
QString exeFromRegistryValue(const QString &value);

// True for uninstallers, installers, readmes, help shortcuts and the like.
bool looksLikeNoise(const QString &name, const QString &target);

// Human-readable name of an executable (version-info FileDescription/ProductName),
// or an empty string when it has none.
QString executableDisplayName(const QString &exePath);

// True when `name` is just the executable's file name (e.g. "chrome", "msedge.exe", "Code_Helper").
bool looksLikeExecutableName(const QString &name, const QString &exePath);

// Best human-readable name for an executable, found dynamically: `candidate` if it is already
// a real name, else version info, else the install folder, else a prettified file name.
QString resolveFriendlyName(const QString &candidate, const QString &exePath);
