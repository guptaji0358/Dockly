#include "softwarescanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString squash(QString s)
{
    s = s.toLower().trimmed();
    if (s.endsWith(QLatin1String(".exe")))
        s.chop(4);
    QString out;
    for (const QChar c : s)
        if (c.isLetterOrNumber())
            out += c;
    return out;
}

// "visualStudioCode", "code_helper", "notepad-plus" -> "Visual Studio Code", "Code Helper", "Notepad Plus"
QString prettifyFileName(const QString &base)
{
    QString s;
    for (int i = 0; i < base.size(); ++i) {
        const QChar c = base[i];
        if (c == '_' || c == '-' || c == '.') {
            s += ' ';
            continue;
        }
        if (i > 0 && c.isUpper() && base[i - 1].isLower())
            s += ' ';
        s += c;
    }
    QStringList words = s.simplified().split(' ', Qt::SkipEmptyParts);
    for (QString &w : words)
        if (w == w.toLower() || w == w.toUpper())
            w = w.left(1).toUpper() + w.mid(1).toLower();
    return words.join(' ');
}
} // namespace

bool looksLikeNoise(const QString &name, const QString &target)
{
    // Whole-word match, so "Helpdesk" or "Setup Factory Pro" style names are not caught by accident.
    static const QRegularExpression noise(
        QStringLiteral("\\b(uninstall\\w*|uninst\\w*|readme|release notes|documentation|manual|help|website|license|"
                       "changelog|what's new|repair|setup)\\b"),
        QRegularExpression::CaseInsensitiveOption);
    if (noise.match(name).hasMatch())
        return true;
    const QString base = QFileInfo(target).fileName().toLower();
    return base.startsWith("unins") || base == "msiexec.exe";
}

QString pickMainExe(const QString &displayName, const QStringList &exeFileNames)
{
    static const QStringList auxiliary = {"update", "crash", "helper", "setup", "install", "unins", "redist", "repair",
                                          "service", "agent", "report", "launcherhelper"};
    const QString app = squash(displayName);
    QString best;
    int bestScore = 0;
    for (const QString &file : exeFileNames) {
        const QString b = squash(QFileInfo(file).completeBaseName());
        if (b.isEmpty() || app.isEmpty() || looksLikeNoise({}, file))
            continue;
        bool aux = false;
        for (const QString &w : auxiliary)
            aux = aux || b.contains(w);
        int score = 0;
        if (b == app)
            score = 1000;
        else if (app.contains(b) || b.contains(app))
            score = 100 + int(b.size());
        if (aux)
            score /= 4; // helpers/updaters only win when nothing else matches
        if (score > bestScore) {
            bestScore = score;
            best = file;
        }
    }
    return best;
}

bool looksLikeExecutableName(const QString &name, const QString &exePath)
{
    const QString n = name.trimmed();
    if (n.isEmpty())
        return true;
    if (n.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        return true;
    return squash(n) == squash(QFileInfo(exePath).completeBaseName());
}

QString resolveFriendlyName(const QString &candidate, const QString &exePath)
{
    if (!looksLikeExecutableName(candidate, exePath))
        return candidate;
    const QString version = executableDisplayName(exePath);
    if (!version.isEmpty())
        return version;
    // No usable version info: the install folder usually carries the product name.
    static const QSet<QString> generic = {"bin", "app", "application", "program files", "program files (x86)", "programs",
                                          "x64", "x86", "win32", "win64", "release", "system32", "windows", "users",
                                          "appdata", "local", "roaming", "current", "latest"};
    QDir dir = QFileInfo(exePath).dir();
    for (int depth = 0; depth < 3 && !dir.isRoot(); ++depth) {
        const QString folder = dir.dirName();
        const bool versionLike = !folder.isEmpty() && folder[0].isDigit();
        if (!folder.isEmpty() && !folder.contains(':') && !versionLike && !generic.contains(folder.toLower())
            && !folder.startsWith(QLatin1String("app-"), Qt::CaseInsensitive))
            return prettifyFileName(folder);
        if (!dir.cdUp())
            break;
    }
    return prettifyFileName(QFileInfo(exePath).completeBaseName());
}

QString executableDisplayName(const QString &exePath)
{
#ifdef Q_OS_WIN
    if (!exePath.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        return {};
    const std::wstring path = QDir::toNativeSeparators(exePath).toStdWString();
    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &ignored);
    if (!size)
        return {};
    QByteArray data(int(size), 0);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data()))
        return {};
    struct LangCodepage { WORD lang, codepage; } *tr = nullptr;
    UINT trLen = 0;
    QList<QString> prefixes;
    if (VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&tr), &trLen) && tr) {
        for (UINT i = 0; i < trLen / sizeof(LangCodepage); ++i)
            prefixes << QStringLiteral("%1%2").arg(tr[i].lang, 4, 16, QChar('0')).arg(tr[i].codepage, 4, 16, QChar('0'));
    }
    prefixes << QStringLiteral("040904b0") << QStringLiteral("040904e4");
    // FileDescription is usually the user-facing name; ProductName is often a vendor suite or generic text.
    for (const QString &key : {QStringLiteral("FileDescription"), QStringLiteral("ProductName")}) {
        for (const QString &pre : prefixes) {
            const std::wstring q = QStringLiteral("\\StringFileInfo\\%1\\%2").arg(pre, key).toStdWString();
            wchar_t *val = nullptr;
            UINT len = 0;
            if (VerQueryValueW(data.data(), q.c_str(), reinterpret_cast<void **>(&val), &len) && val && len > 1) {
                const QString v = QString::fromWCharArray(val).trimmed();
                // skip values that just repeat the file name or describe the OS component
                if (!v.isEmpty() && !looksLikeExecutableName(v, exePath)
                    && !v.contains(QLatin1String("operating system"), Qt::CaseInsensitive))
                    return v;
            }
        }
    }
#else
    Q_UNUSED(exePath)
#endif
    return {};
}

QString exeFromRegistryValue(const QString &value)
{
    QString exe = value.trimmed();
    // REG_EXPAND_SZ values: Qt hands them back unexpanded (e.g. %ProgramFiles%/App/a.exe).
    static const QRegularExpression envVar(QStringLiteral("%([^%]+)%"));
    for (int guard = 0; guard < 8; ++guard) {
        const auto m = envVar.match(exe);
        if (!m.hasMatch())
            break;
        exe.replace(m.capturedStart(), m.capturedLength(), qEnvironmentVariable(m.captured(1).toUtf8().constData()));
    }
    if (exe.startsWith('"'))
        exe = exe.section('"', 1, 1);
    const int comma = exe.lastIndexOf(',');
    if (comma > 0 && !exe.endsWith(".exe", Qt::CaseInsensitive))
        exe = exe.left(comma);
    exe = QDir::cleanPath(exe);
    return exe.endsWith(".exe", Qt::CaseInsensitive) ? exe : QString();
}

ExeInfo executableInfo(const QString &exePath)
{
    ExeInfo info;
#ifdef Q_OS_WIN
    if (!exePath.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        return info;
    const std::wstring path = QDir::toNativeSeparators(exePath).toStdWString();
    DWORD ignored = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &ignored);
    if (!size)
        return info;
    QByteArray data(int(size), 0);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data()))
        return info;
    struct LangCodepage { WORD lang, codepage; } *tr = nullptr;
    UINT trLen = 0;
    QStringList prefixes;
    if (VerQueryValueW(data.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&tr), &trLen) && tr) {
        for (UINT i = 0; i < trLen / sizeof(LangCodepage); ++i)
            prefixes << QStringLiteral("%1%2").arg(tr[i].lang, 4, 16, QChar('0')).arg(tr[i].codepage, 4, 16, QChar('0'));
    }
    prefixes << QStringLiteral("040904b0") << QStringLiteral("040904e4");
    auto read = [&](const QString &key) -> QString {
        for (const QString &pre : prefixes) {
            const std::wstring q = QStringLiteral("\\StringFileInfo\\%1\\%2").arg(pre, key).toStdWString();
            wchar_t *val = nullptr;
            UINT len = 0;
            if (VerQueryValueW(data.data(), q.c_str(), reinterpret_cast<void **>(&val), &len) && val && len > 1) {
                const QString v = QString::fromWCharArray(val).trimmed();
                if (!v.isEmpty())
                    return v;
            }
        }
        return {};
    };
    info.publisher = read(QStringLiteral("CompanyName"));
    info.version = read(QStringLiteral("ProductVersion"));
    if (info.version.isEmpty())
        info.version = read(QStringLiteral("FileVersion"));
#else
    Q_UNUSED(exePath)
#endif
    return info;
}

QVector<ScannedApp> mergeRecords(const QVector<RawRecord> &records, bool fillFromExe)
{
    QVector<ScannedApp> result;
    QHash<QString, int> index; // launch key -> position in result
    auto fill = [](QString &dst, const QString &src) {
        if (dst.isEmpty())
            dst = src.trimmed();
    };
    for (const RawRecord &r : records) {
        if (looksLikeNoise(r.name, r.launch))
            continue;
        const QString key = QDir::cleanPath(r.launch).toLower();
        if (const auto it = index.constFind(key); it != index.constEnd()) {
            ScannedApp &a = result[*it]; // same program, another source: keep the name, add what was missing
            a.sources |= 1 << int(r.source);
            fill(a.publisher, r.publisher);
            fill(a.version, r.version);
            fill(a.installDir, r.installDir);
            continue;
        }
        const QString shown = r.resolveName ? r.resolveName() : r.name;
        if (shown.isEmpty() || looksLikeNoise(shown, r.launch))
            continue;
        ScannedApp a;
        a.name = shown;
        a.target = QDir::toNativeSeparators(QDir::cleanPath(r.launch));
        a.publisher = r.publisher.trimmed();
        a.version = r.version.trimmed();
        a.installDir = r.installDir.trimmed();
        a.sources = 1 << int(r.source);
        index.insert(key, result.size());
        result.append(a);
    }
    if (fillFromExe) {
        for (ScannedApp &a : result) {
            if ((a.publisher.isEmpty() || a.version.isEmpty()) && a.target.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive)) {
                const ExeInfo info = executableInfo(a.target);
                fill(a.publisher, info.publisher);
                fill(a.version, info.version);
            }
            if (a.installDir.isEmpty() && a.target.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
                a.installDir = QDir::toNativeSeparators(QFileInfo(a.target).absolutePath());
        }
    }
    return result;
}

namespace {
void collectStartMenu(QVector<RawRecord> &out)
{
    for (const QString &root : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)) {
        QDirIterator it(root, {"*.lnk"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo link(it.next());
            const QString target = link.symLinkTarget();
            const QString name = link.completeBaseName();
            // Store apps and installer-advertised shortcuts have no resolvable .exe target;
            // keep the shortcut itself so it still launches through the shell.
            const bool exeTarget = target.endsWith(".exe", Qt::CaseInsensitive);
            if (!target.isEmpty() && !exeTarget)
                continue;
            if (exeTarget && !QFileInfo::exists(target))
                continue;
            RawRecord r;
            r.source = AppSource::StartMenu;
            r.name = name;
            r.launch = exeTarget ? target : link.absoluteFilePath();
            if (exeTarget)
                r.resolveName = [name, target] { return resolveFriendlyName(name, target); };
            out.append(std::move(r));
        }
    }
}

void addRegistryExe(QVector<RawRecord> &out, const QString &name, const QString &value, AppSource source,
                    const QString &publisher = {}, const QString &version = {}, const QString &installDir = {})
{
    const QString exe = exeFromRegistryValue(value);
    if (name.isEmpty() || exe.isEmpty() || !QFileInfo::exists(exe))
        return;
    RawRecord r;
    r.source = source;
    r.name = name;
    r.launch = exe;
    r.publisher = publisher;
    r.version = version;
    r.installDir = installDir;
    if (source == AppSource::AppPaths) {
        // App Paths keys are file names; always look for the real product name
        r.resolveName = [exe] {
            const QString friendly = executableDisplayName(exe);
            return friendly.isEmpty() ? resolveFriendlyName(QFileInfo(exe).completeBaseName(), exe) : friendly;
        };
    } else {
        r.resolveName = [name, exe] { return resolveFriendlyName(name, exe); };
    }
    out.append(std::move(r));
}

void collectUninstall(QVector<RawRecord> &out)
{
    const QStringList roots = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    };
    for (const QString &root : roots) {
        QSettings reg(root, QSettings::NativeFormat);
        for (const QString &sub : reg.childGroups()) {
            reg.beginGroup(sub);
            if (reg.value("SystemComponent").toInt() != 1) {
                const QString display = reg.value("DisplayName").toString();
                // Updates and patches are registered as children of the product they update.
                const QString release = reg.value("ReleaseType").toString();
                if (!reg.value("ParentKeyName").toString().isEmpty() || release.contains("Update", Qt::CaseInsensitive)
                    || release.contains("Hotfix", Qt::CaseInsensitive)) {
                    reg.endGroup();
                    continue;
                }
                QString icon = reg.value("DisplayIcon").toString();
                const QString dir = reg.value("InstallLocation").toString().trimmed().remove('"');
                const bool dirOk = !dir.isEmpty() && QDir(dir).exists();
                // DisplayIcon often points at the uninstaller (or a .ico/.dll): then look in InstallLocation instead.
                const QString iconExe = exeFromRegistryValue(icon);
                if (iconExe.isEmpty() || looksLikeNoise({}, iconExe)) {
                    icon.clear();
                    if (dirOk) {
                        const QString main = pickMainExe(display, QDir(dir).entryList({"*.exe"}, QDir::Files));
                        if (!main.isEmpty())
                            icon = QDir(dir).filePath(main);
                    }
                }
                addRegistryExe(out, display, icon, AppSource::Uninstall, reg.value("Publisher").toString(),
                               reg.value("DisplayVersion").toString(), dirOk ? QDir::toNativeSeparators(QDir::cleanPath(dir)) : QString());
            }
            reg.endGroup();
        }
    }
}

void collectAppPaths(QVector<RawRecord> &out)
{
    const QStringList roots = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
    };
    for (const QString &root : roots) {
        QSettings reg(root, QSettings::NativeFormat);
        for (const QString &sub : reg.childGroups()) {
            reg.beginGroup(sub);
            addRegistryExe(out, QFileInfo(sub).completeBaseName(), reg.value("Default").toString(), AppSource::AppPaths);
            reg.endGroup();
        }
    }
}
} // namespace

// ---------------------------------------------------------------------------
// Packaged (Store / MSIX) apps, through the shell's "Apps" folder
// ---------------------------------------------------------------------------

namespace {
const QLatin1String kPackagedPrefix("shell:AppsFolder\\");
}

bool isPackagedTarget(const QString &target)
{
    const QString t = target.trimmed();
    return t.size() > kPackagedPrefix.size() && t.startsWith(QLatin1String("shell:AppsFolder"), Qt::CaseInsensitive)
           && (t[16] == '\\' || t[16] == '/');
}

QString packagedAppId(const QString &target)
{
    return isPackagedTarget(target) ? target.trimmed().mid(kPackagedPrefix.size()) : QString();
}

#ifdef Q_OS_WIN
#include <shobjidl.h>
#include <shlobj.h>

namespace {
// Shell COM calls need COM on the calling thread (scan, status checks and icons run on pool threads).
struct ComScope {
    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    ~ComScope()
    {
        if (SUCCEEDED(hr))
            CoUninitialize();
    }
};

// {1E87508D-89C2-42F0-8A7E-645A0F50CA58}
const GUID kAppsFolder = {0x1e87508d, 0x89c2, 0x42f0, {0x8a, 0x7e, 0x64, 0x5a, 0x0f, 0x50, 0xca, 0x58}};
// BHID_EnumItems {94F60519-2850-4924-AA5A-D15E84868039}
const GUID kEnumItems = {0x94f60519, 0x2850, 0x4924, {0xaa, 0x5a, 0xd1, 0x5e, 0x84, 0x86, 0x80, 0x39}};

QString takeString(PWSTR s)
{
    const QString out = s ? QString::fromWCharArray(s) : QString();
    CoTaskMemFree(s);
    return out;
}

// an AUMID of a packaged app looks like "<PackageFamilyName>!<AppId>"
bool looksLikePackagedId(const QString &id)
{
    return id.contains('!') && !id.contains('\\') && !id.contains('/') && !id.contains(':');
}

void collectPackaged(QVector<RawRecord> &out)
{
    ComScope com;
    IShellItem *folder = nullptr;
    if (FAILED(SHGetKnownFolderItem(kAppsFolder, KF_FLAG_DEFAULT, nullptr, IID_PPV_ARGS(&folder))) || !folder)
        return;
    IEnumShellItems *items = nullptr;
    if (SUCCEEDED(folder->BindToHandler(nullptr, kEnumItems, IID_PPV_ARGS(&items))) && items) {
        IShellItem *item = nullptr;
        ULONG got = 0;
        while (items->Next(1, &item, &got) == S_OK && item) {
            PWSTR name = nullptr, parse = nullptr;
            item->GetDisplayName(SIGDN_NORMALDISPLAY, &name);
            item->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &parse);
            const QString display = takeString(name).trimmed();
            QString parsing = takeString(parse);
            item->Release();
            item = nullptr;
            // The parsing name of an Apps-folder entry is its AUMID. Desktop apps show a file path or URL
            // there (already found by the other providers), so only "<package family>!<app id>" is kept.
            const QString id = parsing.trimmed();
            if (display.isEmpty() || !looksLikePackagedId(id))
                continue;
            RawRecord r;
            r.source = AppSource::Packaged;
            r.name = display;
            r.launch = kPackagedPrefix + id;
            out.append(std::move(r));
        }
        items->Release();
    }
    folder->Release();
}
} // namespace

bool packagedAppExists(const QString &target)
{
    if (!isPackagedTarget(target))
        return false;
    ComScope com;
    const std::wstring path = QDir::toNativeSeparators(target.trimmed()).toStdWString();
    IShellItem *item = nullptr;
    const bool ok = SUCCEEDED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&item))) && item;
    if (item)
        item->Release();
    return ok;
}

bool launchPackagedApp(const QString &target)
{
    if (!isPackagedTarget(target))
        return false;
    const std::wstring path = QDir::toNativeSeparators(target.trimmed()).toStdWString();
    const auto r = reinterpret_cast<quintptr>(ShellExecuteW(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL));
    return r > 32;
}

QImage packagedAppIcon(const QString &target, int size)
{
    if (!isPackagedTarget(target) || size <= 0)
        return {};
    ComScope com;
    const std::wstring path = QDir::toNativeSeparators(target.trimmed()).toStdWString();
    IShellItemImageFactory *factory = nullptr;
    if (FAILED(SHCreateItemFromParsingName(path.c_str(), nullptr, IID_PPV_ARGS(&factory))) || !factory)
        return {};
    HBITMAP bmp = nullptr;
    const SIZE sz = {size, size};
    QImage img;
    if (SUCCEEDED(factory->GetImage(sz, SIIGBF_ICONONLY | SIIGBF_BIGGERSIZEOK, &bmp)) && bmp) {
        BITMAP info{};
        if (GetObject(bmp, sizeof(info), &info) && info.bmWidth > 0 && info.bmHeight > 0) {
            BITMAPINFO bi{};
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = info.bmWidth;
            bi.bmiHeader.biHeight = -info.bmHeight; // top-down
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            QImage tmp(info.bmWidth, info.bmHeight, QImage::Format_ARGB32_Premultiplied);
            HDC dc = GetDC(nullptr);
            if (GetDIBits(dc, bmp, 0, UINT(info.bmHeight), tmp.bits(), &bi, DIB_RGB_COLORS))
                img = tmp;
            ReleaseDC(nullptr, dc);
        }
        DeleteObject(bmp);
    }
    factory->Release();
    return img;
}
#else
namespace {
void collectPackaged(QVector<RawRecord> &) {}
}
bool packagedAppExists(const QString &) { return false; }
bool launchPackagedApp(const QString &) { return false; }
QImage packagedAppIcon(const QString &, int) { return {}; }
#endif

QVector<ScannedApp> scanInstalledSoftware()
{
    // Order matters: the first record for an exe wins (Start Menu, then Uninstall, then App Paths).
    QVector<RawRecord> records;
    collectStartMenu(records);
    collectUninstall(records);
    collectAppPaths(records);
    collectPackaged(records);
    return mergeRecords(records, true);
}
