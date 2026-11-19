#include "softwarescanner.h"

#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
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

QString expandEnvironment(const QString &value, bool *ok)
{
    if (ok)
        *ok = true;
    QString out;
    int i = 0;
    while (i < value.size()) {
        if (value[i] == '%') {
            const int end = value.indexOf('%', i + 1);
            if (end > i + 1) {
                const QString key = value.mid(i + 1, end - i - 1);
                const QByteArray k = key.toUtf8();
                if (qEnvironmentVariableIsSet(k.constData())) {
                    out += qEnvironmentVariable(k.constData()); // single pass: the result is never re-expanded
                    i = end + 1;
                    continue;
                }
                if (ok)
                    *ok = false; // undefined variable: the path would be wrong, so the value is unusable
            }
        }
        out += value[i++];
    }
    return out;
}

QString exeFromRegistryValue(const QString &value, QString *args)
{
    if (args)
        args->clear();
    bool ok = true;
    const QString v = expandEnvironment(value.trimmed(), &ok);
    if (!ok || v.isEmpty())
        return {};
    QString exe, rest;
    if (v.startsWith('"')) {
        const int close = v.indexOf('"', 1);
        exe = close < 0 ? v.mid(1) : v.mid(1, close - 1);
        rest = close < 0 ? QString() : v.mid(close + 1);
    } else {
        // unquoted: the path ends at the first ".exe" followed by end, space or comma
        static const QRegularExpression end(QStringLiteral("\\.exe(?=$|[\\s,])"), QRegularExpression::CaseInsensitiveOption);
        const auto m = end.match(v);
        if (!m.hasMatch())
            return {};
        exe = v.left(m.capturedEnd());
        rest = v.mid(m.capturedEnd());
    }
    rest = rest.trimmed();
    if (rest.startsWith(',')) // ",0" / ",-3" is an icon index, not an argument
        rest.clear();
    if (!exe.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        return {};
    if (args)
        *args = rest;
    return QDir::cleanPath(exe);
}

QString exeFromRegistryValue(const QString &value)
{
    return exeFromRegistryValue(value, nullptr);
}

QString stripVersionDecoration(const QString &name)
{
    // Only whole decorations are removed: "(x64)" or a bare trailing "x64", never half of a bracket pair.
    static const QRegularExpression arch(
        QStringLiteral(R"(\s*(?:[(\[]\s*(?:x64|x86|64-bit|32-bit|amd64|arm64)\s*[)\]]|-?\s*(?:x64|x86|64-bit|32-bit|amd64|arm64))\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression tail(
        QStringLiteral(R"(\s*(?:[(\[]\s*v?\d+(?:\.\d+)+[\w.\-]*\s*[)\]]|v(?:ersion)?\s*\d+(?:\.\d+)*[\w.\-]*|\d+(?:\.\d+)+[\w.\-]*)\s*$)"),
        QRegularExpression::CaseInsensitiveOption);
    QString s = name.trimmed();
    for (int i = 0; i < 4; ++i) {
        QString t = s;
        t.remove(arch);
        t.remove(tail);
        t = t.trimmed();
        if (t == s || t.isEmpty())
            break;
        s = t;
    }
    return s;
}

int nameQuality(const QString &nameIn, const QString &exePath)
{
    const QString name = nameIn.trimmed();
    if (name.isEmpty())
        return -1000;
    int q = 0;
    bool upper = false, lower = false;
    for (const QChar c : name) {
        upper = upper || c.isUpper();
        lower = lower || c.isLower();
    }
    const bool humanFormatted = name.contains(' ') || (upper && lower);
    // "notepad" next to notepad.exe is a file name; "Notepad" / "Contoso Studio" are written for people
    if (name.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive)
        || (!humanFormatted && !exePath.isEmpty() && looksLikeExecutableName(name, exePath)))
        q -= 100;
    if (name.contains('\\') || name.contains('/') || name.contains(QLatin1String("://")))
        q -= 60;
    if (name.size() < 2)
        q -= 40;
    if (name.size() > 64)
        q -= 30;
    static const QRegularExpression versionish(QStringLiteral("\\d+\\.\\d+\\.\\d+|\\b(x64|x86|amd64|arm64|64-bit|32-bit)\\b"),
                                               QRegularExpression::CaseInsensitiveOption);
    if (versionish.match(name).hasMatch())
        q -= 25;
    if (name.contains(QLatin1String("operating system"), Qt::CaseInsensitive))
        q -= 30;
    static const QRegularExpression guid(QStringLiteral("^\\{?[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-"));
    if (guid.match(name).hasMatch())
        q -= 100;
    // human-formatted names carry spaces or mixed case; "chrome" / "code_helper" look like file names
    if (humanFormatted)
        q += 4;
    else if (lower && !upper)
        q -= 10;
    return q;
}

QString rankNames(const QVector<NameCandidate> &cands, const QString &exePath)
{
    int bestScore = -100000;
    QString best;
    for (int i = 0; i < cands.size(); ++i) {
        const QString text = cands[i].text.trimmed();
        if (text.isEmpty())
            continue;
        const QString sq = squash(stripVersionDecoration(text));
        int score = cands[i].prior + nameQuality(text, exePath);
        // agreement: other sources spell the same product (equal, or one contains the other)
        for (int j = 0; j < cands.size(); ++j) {
            if (i == j)
                continue;
            const QString other = squash(stripVersionDecoration(cands[j].text));
            if (sq.size() >= 3 && other.size() >= 3 && (sq == other || sq.contains(other) || other.contains(sq)))
                score += 15;
        }
        if (score > bestScore) { // strict: earlier candidates win ties
            bestScore = score;
            best = text;
        }
    }
    // A winner that still looks like a file name or OS text is no better than no name at all.
    return nameQuality(best, exePath) <= -50 ? QString() : best;
}

bool looksLikeInstallerExe(const QString &exePath)
{
    const QString base = QFileInfo(exePath).completeBaseName().toLower();
    static const QRegularExpression byName(
        QStringLiteral("^(unins[\\w.]*|uninst[\\w.]*|uninstall[\\w.]*|setup[\\w.]*|install[\\w.]*|vc_?redist[\\w.]*|vcredist[\\w.]*|msiexec|"
                       "dxsetup|dotnet-?\\w*|windowsdesktop-runtime\\w*|.*[-_ ](setup|installer)([-_ ].*)?)$"));
    if (byName.match(base).hasMatch())
        return true;
    // executables kept in the Windows Installer cache are installer stubs, not the program itself
    if (QDir::fromNativeSeparators(exePath).toLower().contains(QLatin1String("/windows/installer/")))
        return true;
    const QString desc = executableDisplayName(exePath).toLower();
    return desc.contains(QLatin1String("uninstall")) || desc.endsWith(QLatin1String(" installer"))
           || (desc.contains(QLatin1String("setup")) && desc.contains(QLatin1String("install")));
}

QString findMainExeInDir(const QString &dirPath, const QString &displayName, const QString &publisher)
{
    QDir dir(dirPath);
    if (!dir.exists())
        return {};
    QStringList files; // relative to dir
    for (const QString &f : dir.entryList({"*.exe"}, QDir::Files))
        files << f;
    for (const QString &sub : dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) { // one level down ("bin", "app-1.2")
        for (const QString &f : QDir(dir.filePath(sub)).entryList({"*.exe"}, QDir::Files))
            files << sub + '/' + f;
    }
    const QString app = squash(displayName);
    const QString pub = squash(publisher);
    static const QStringList auxiliary = {"update", "crash", "helper", "redist", "repair", "service", "agent", "report",
                                          "launcherhelper", "elevate", "notifier"};
    QString best;
    int bestScore = 0;
    for (const QString &rel : files) {
        const QString full = dir.filePath(rel);
        if (looksLikeInstallerExe(full))
            continue;
        const QString b = squash(QFileInfo(rel).completeBaseName());
        if (b.isEmpty())
            continue;
        int score = 0;
        if (!app.isEmpty() && b == app)
            score = 1000;
        else if (!app.isEmpty() && b.size() >= 3 && (app.contains(b) || b.contains(app)))
            score = 200 + int(b.size());
        // generic evidence from the file itself: its version-info description names the same product
        const QString desc = squash(executableDisplayName(full));
        if (!app.isEmpty() && !desc.isEmpty() && (desc == app || (desc.size() >= 4 && (app.contains(desc) || desc.contains(app)))))
            score += 300;
        if (score == 0 && !pub.isEmpty() && b == pub)
            score = 50; // publisher-named launcher is a weak guess
        if (score == 0)
            continue; // no evidence: size or position alone never selects an executable
        for (const QString &w : auxiliary)
            if (b.contains(w))
                score /= 4;
        if (rel.contains('/'))
            score -= 20; // top-level beats nested when otherwise equal
        if (score > bestScore) {
            bestScore = score;
            best = full;
        }
    }
    return best;
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

namespace {
int sourcePrior(AppSource s)
{
    switch (s) {
    case AppSource::AppsFolder:
    case AppSource::Packaged: return 60; // the name Windows itself shows in Start
    case AppSource::StartMenu: return 50;
    case AppSource::Uninstall: return 48;
    case AppSource::AppPaths: return 20; // keys are file names
    }
    return 0;
}

QString launchKey(const QString &launch, const QString &args)
{
    QString k = isPackagedTarget(launch) ? launch.trimmed() : QDir::cleanPath(launch);
    if (isPackagedTarget(launch)) {
        k.replace('/', '\\');
    } else {
        // 8.3 short names, symlinks and junctions spell one file several ways: use the canonical path when it exists
        const QString canon = QFileInfo(k).canonicalFilePath();
        if (!canon.isEmpty())
            k = canon;
    }
    k = k.toLower();
    const QString a = args.simplified().toLower();
    return a.isEmpty() ? k : k + QChar(0x1f) + a;
}
} // namespace

QVector<ScannedApp> mergeRecords(const QVector<RawRecord> &records, bool fillFromExe, QVector<Rejection> *rejected)
{
    struct Group {
        int resultIndex = -1;
        QVector<NameCandidate> names;
        std::function<QString()> lazy;
        bool alive = true;
    };
    QVector<ScannedApp> result;
    QVector<Group> groups;
    QHash<QString, int> index; // launch key -> position in result/groups
    QHash<QString, int> exeIndex; // launch key without arguments -> first entry for that program
    auto reject = [&](const RawRecord &r, const char *why) {
        if (rejected)
            rejected->append({r.source, r.name, r.launch, QString::fromLatin1(why)});
    };
    auto fill = [](QString &dst, const QString &src) {
        if (dst.isEmpty())
            dst = src.trimmed();
    };
    for (const RawRecord &r : records) {
        if (r.launch.trimmed().isEmpty()) {
            reject(r, "no-launch-target");
            continue;
        }
        if (looksLikeNoise(r.name, r.launch)) {
            reject(r, looksLikeNoise({}, r.launch) ? "noise-target" : "noise-name");
            continue;
        }
        const QString key = launchKey(r.launch, r.args);
        const QString baseKey = launchKey(r.launch, {});
        int hit = index.value(key, -1);
        // Registry entries describe the program, not a shortcut: they carry no arguments, so when the same exe is
        // already known through argument-bearing shortcuts they add to it instead of becoming another card.
        if (hit < 0 && r.args.simplified().isEmpty()
            && (r.source == AppSource::Uninstall || r.source == AppSource::AppPaths))
            hit = exeIndex.value(baseKey, -1);
        if (hit >= 0) {
            ScannedApp &a = result[hit]; // same program, another source: add what was missing
            Group &g = groups[hit];
            a.sources |= 1 << int(r.source);
            fill(a.publisher, r.publisher);
            fill(a.version, r.version);
            fill(a.installDir, r.installDir);
            if (!r.name.trimmed().isEmpty())
                g.names.append({r.name, sourcePrior(r.source)});
            if (!g.lazy)
                g.lazy = r.resolveName;
            continue;
        }
        ScannedApp a;
        a.target = isPackagedTarget(r.launch) ? r.launch.trimmed() : QDir::toNativeSeparators(QDir::cleanPath(r.launch));
        a.args = r.args.simplified();
        a.publisher = r.publisher.trimmed();
        a.version = r.version.trimmed();
        a.installDir = r.installDir.trimmed();
        a.sources = 1 << int(r.source);
        Group g;
        g.resultIndex = result.size();
        if (!r.name.trimmed().isEmpty())
            g.names.append({r.name, sourcePrior(r.source)});
        g.lazy = r.resolveName;
        index.insert(key, result.size());
        if (!exeIndex.contains(baseKey))
            exeIndex.insert(baseKey, result.size());
        result.append(a);
        groups.append(g);
    }
    // Names are decided once every source has been seen, so agreement between sources can count.
    QVector<ScannedApp> kept;
    QHash<int, int> remap;
    for (int i = 0; i < result.size(); ++i) {
        ScannedApp &a = result[i];
        Group &g = groups[i];
        const QString exe = isPackagedTarget(a.target) ? QString() : a.target;
        QString name = rankNames(g.names, exe);
        if ((name.isEmpty() || nameQuality(name, exe) < 0) && g.lazy) {
            const QString viaInfo = g.lazy(); // version info / folder name: only consulted when the names look weak
            if (!viaInfo.trimmed().isEmpty()) {
                QVector<NameCandidate> more = g.names;
                more.append({viaInfo, 39});
                name = rankNames(more, exe);
            }
        }
        const QString clean = stripVersionDecoration(name);
        if (!clean.isEmpty())
            name = clean;
        if (name.isEmpty() || looksLikeNoise(name, a.target)) {
            if (rejected)
                rejected->append({AppSource::StartMenu, g.names.isEmpty() ? QString() : g.names.first().text, a.target,
                                  name.isEmpty() ? QStringLiteral("unresolved-name") : QStringLiteral("noise-name")});
            continue;
        }
        a.name = name;
        kept.append(a);
    }
    result = kept;
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
    // Final pass: one program can still be reached through unrelated spellings (an AUMID and its exe, a versioned
    // and an unversioned install path). Merge only on strong, generic evidence: same display name and same arguments,
    // plus either a lone AUMID/exe pair or nested install folders. Separate installs never nest.
    auto norm = [](const QString &s) { return s.simplified().toLower(); };
    auto exeDir = [](const ScannedApp &a) {
        return isPackagedTarget(a.target) ? QString() : QFileInfo(a.target).absolutePath().toLower().replace('\\', '/');
    };
    auto nested = [](const QString &x, const QString &y) {
        return !x.isEmpty() && !y.isEmpty() && (x == y || x.startsWith(y + '/') || y.startsWith(x + '/'));
    };
    QHash<QString, int> aumidsByName;
    for (const ScannedApp &a : std::as_const(result))
        if (isPackagedTarget(a.target) && a.args.isEmpty())
            aumidsByName[norm(a.name)]++;
    QVector<bool> dropped(result.size(), false);
    QVector<QStringList> dirs; // install folders of every entry merged into a survivor
    for (const ScannedApp &a : std::as_const(result))
        dirs.append(QStringList{exeDir(a)});
    QSet<int> paired; // a packaged entry absorbs at most one exe: a second same-named exe is another install
    for (int j = 1; j < result.size(); ++j) {
        for (int i = 0; i < j && !dropped[j]; ++i) {
            if (dropped[i] || norm(result[i].name) != norm(result[j].name))
                continue;
            const bool mixed = isPackagedTarget(result[i].target) != isPackagedTarget(result[j].target);
            // An AUMID entry (what Start launches) has no arguments by nature, so it may pair with an exe entry
            // that carries its shortcut's arguments; between two exe entries the arguments must agree.
            if (!mixed && result[i].args != result[j].args)
                continue;
            const bool pair = mixed && !paired.contains(i) && !paired.contains(j) && aumidsByName.value(norm(result[i].name)) == 1;
            bool closeBy = false;
            for (const QString &di : std::as_const(dirs[i]))
                closeBy = closeBy || nested(di, exeDir(result[j]));
            if (!pair && !closeBy)
                continue;
            if (isPackagedTarget(result[i].target) != isPackagedTarget(result[j].target)
                && aumidsByName.value(norm(result[i].name)) != 1)
                continue; // ambiguous: several packaged apps share this name
            if (mixed)
                paired.insert(i), paired.insert(j);
            dirs[i] += dirs[j];
            result[i].sources |= result[j].sources;
            fill(result[i].publisher, result[j].publisher);
            fill(result[i].version, result[j].version);
            fill(result[i].installDir, result[j].installDir);
            dropped[j] = true;
        }
    }
    QVector<ScannedApp> unique;
    for (int i = 0; i < result.size(); ++i)
        if (!dropped[i])
            unique.append(result[i]);
    return unique;
}

bool isLaunchableTarget(const QString &path)
{
    static const QSet<QString> ok = {"exe", "com", "bat", "cmd", "msc", "lnk", "url", "jar", "cpl"};
    return ok.contains(QFileInfo(path).suffix().toLower());
}

LinkInfo resolveShortcut(const QString &lnkPath)
{
    LinkInfo info;
#ifdef Q_OS_WIN
    const HRESULT hrInit = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    IShellLinkW *link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))) && link) {
        IPersistFile *file = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_PPV_ARGS(&file))) && file) {
            const std::wstring path = QDir::toNativeSeparators(lnkPath).toStdWString();
            if (SUCCEEDED(file->Load(path.c_str(), STGM_READ))) {
                link->Resolve(nullptr, SLR_NO_UI | SLR_NOSEARCH | SLR_NOTRACK | SLR_NOUPDATE);
                wchar_t buf[32768];
                // the expanded path first (env vars resolved), falling back to the raw stored one
                if (SUCCEEDED(link->GetPath(buf, 32768, nullptr, 0)) && buf[0])
                    info.target = QDir::fromNativeSeparators(QString::fromWCharArray(buf));
                if (SUCCEEDED(link->GetArguments(buf, 32768)))
                    info.args = QString::fromWCharArray(buf).trimmed();
                if (SUCCEEDED(link->GetWorkingDirectory(buf, 32768)))
                    info.workDir = QDir::fromNativeSeparators(QString::fromWCharArray(buf));
            }
            file->Release();
        }
        link->Release();
    }
    if (SUCCEEDED(hrInit))
        CoUninitialize();
#endif
    if (info.target.isEmpty()) { // Qt's resolver as a fallback
        const QString t = QFileInfo(lnkPath).symLinkTarget();
        if (!t.isEmpty())
            info.target = t;
    }
    return info;
}

namespace {
void collectStartMenu(QVector<RawRecord> &out, QVector<Rejection> &rej)
{
    for (const QString &root : QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation)) {
        QDirIterator it(root, {"*.lnk"}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QFileInfo link(it.next());
            const QString name = link.completeBaseName();
            const LinkInfo li = resolveShortcut(link.absoluteFilePath());
            auto drop = [&](const char *why) {
                rej.append({AppSource::StartMenu, name, li.target.isEmpty() ? link.absoluteFilePath() : li.target,
                            QString::fromLatin1(why)});
            };
            RawRecord r;
            r.source = AppSource::StartMenu;
            r.name = name;
            if (li.target.isEmpty()) {
                // Store apps and installer-advertised shortcuts have no resolvable target: the shortcut itself
                // is still a valid way to start the program, so it is kept as a shell launch target.
                r.launch = link.absoluteFilePath();
                out.append(std::move(r));
                continue;
            }
            const QString suffix = QFileInfo(li.target).suffix().toLower();
            if (!isLaunchableTarget(li.target)) {
                drop("non-program-target"); // folders, documents, help files
                continue;
            }
            if (!QFileInfo::exists(li.target)) {
                drop("target-missing");
                continue;
            }
            if (suffix == QLatin1String("exe") && looksLikeInstallerExe(li.target)) {
                drop("installer-target");
                continue;
            }
            r.launch = li.target;
            r.args = li.args;
            if (suffix == QLatin1String("exe")) {
                const QString target = li.target;
                r.resolveName = [name, target] { return resolveFriendlyName(name, target); };
            }
            out.append(std::move(r));
        }
    }
}

void addRegistryExe(QVector<RawRecord> &out, QVector<Rejection> &rej, const QString &name, const QString &exe,
                    AppSource source, const QString &publisher = {}, const QString &version = {},
                    const QString &installDir = {})
{
    if (name.isEmpty() || exe.isEmpty()) {
        rej.append({source, name, exe, QStringLiteral("no-resolvable-exe")});
        return;
    }
    if (!QFileInfo::exists(exe)) {
        rej.append({source, name, exe, QStringLiteral("target-missing")});
        return;
    }
    if (looksLikeInstallerExe(exe)) {
        rej.append({source, name, exe, QStringLiteral("installer-target")});
        return;
    }
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

// Both registry views (64-bit and 32-bit) are read explicitly, instead of relying on the WOW6432Node redirect.
const QSettings::Format kViews[] = {QSettings::Registry64Format, QSettings::Registry32Format};

void collectUninstall(QVector<RawRecord> &out, QVector<Rejection> &rej)
{
    const QStringList roots = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    };
    for (const QSettings::Format view : kViews) {
        for (const QString &root : roots) {
            QSettings reg(root, view);
            for (const QString &sub : reg.childGroups()) {
                reg.beginGroup(sub);
                const QString display = reg.value("DisplayName").toString().trimmed();
                // Updates and patches are registered as children of the product they update.
                const QString release = reg.value("ReleaseType").toString();
                const bool system = reg.value("SystemComponent").toInt() == 1;
                const bool update = !reg.value("ParentKeyName").toString().isEmpty() || release.contains("Update", Qt::CaseInsensitive)
                                    || release.contains("Hotfix", Qt::CaseInsensitive);
                if (display.isEmpty()) {
                    // key without a display name: not a user-visible product
                } else if (system) {
                    rej.append({AppSource::Uninstall, display, QString(), QStringLiteral("system-component")});
                } else if (update) {
                    rej.append({AppSource::Uninstall, display, QString(), QStringLiteral("update-or-patch")});
                } else {
                    const QString dir = expandEnvironment(reg.value("InstallLocation").toString().trimmed().remove('"'));
                    const bool dirOk = !dir.isEmpty() && QDir(dir).exists();
                    // DisplayIcon often points at the uninstaller (or a .ico/.dll): then look in InstallLocation instead.
                    QString exe = exeFromRegistryValue(reg.value("DisplayIcon").toString());
                    if (exe.isEmpty() || looksLikeInstallerExe(exe))
                        exe = dirOk ? findMainExeInDir(dir, display, reg.value("Publisher").toString()) : QString();
                    addRegistryExe(out, rej, display, exe, AppSource::Uninstall, reg.value("Publisher").toString(),
                                   reg.value("DisplayVersion").toString(),
                                   dirOk ? QDir::toNativeSeparators(QDir::cleanPath(dir)) : QString());
                }
                reg.endGroup();
            }
        }
    }
}

void collectAppPaths(QVector<RawRecord> &out, QVector<Rejection> &rej)
{
    const QStringList roots = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
    };
    for (const QSettings::Format view : kViews) {
        for (const QString &root : roots) {
            QSettings reg(root, view);
            for (const QString &sub : reg.childGroups()) {
                reg.beginGroup(sub);
                addRegistryExe(out, rej, QFileInfo(sub).completeBaseName(), exeFromRegistryValue(reg.value("Default").toString()),
                               AppSource::AppPaths);
                reg.endGroup();
            }
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

// An AUMID ("<PackageFamilyName>!<AppId>" for packaged apps, a plain dotted id for some desktop apps) has no
// path or URL characters. The package *family* name is stable across updates, unlike the versioned full name.
bool looksLikeAumid(const QString &id)
{
    return !id.isEmpty() && !id.contains('\\') && !id.contains('/') && !id.contains(':') && !id.startsWith('{');
}

QString normalizeAumid(const QString &id)
{
    QString s = id.trimmed();
    while (s.startsWith('\\') || s.startsWith('/'))
        s.remove(0, 1);
    return s;
}

// "{FOLDERID-GUID}\sub\path.exe" is how the Apps folder spells desktop programs: expand the known folder.
QString expandKnownFolderPath(const QString &parsing)
{
    static const QRegularExpression lead(QStringLiteral("^\\{([0-9A-Fa-f\\-]{36})\\}[\\\\/]?(.*)$"));
    const auto m = lead.match(parsing);
    if (!m.hasMatch())
        return parsing;
    GUID g{};
    const std::wstring w = (QStringLiteral("{") + m.captured(1) + QStringLiteral("}")).toStdWString();
    if (FAILED(CLSIDFromString(w.c_str(), &g)))
        return {};
    PWSTR folder = nullptr;
    if (FAILED(SHGetKnownFolderPath(g, KF_FLAG_DEFAULT, nullptr, &folder)) || !folder)
        return {};
    const QString base = QDir::fromNativeSeparators(takeString(folder));
    return QDir::cleanPath(base + '/' + QDir::fromNativeSeparators(m.captured(2)));
}

// Primary discovery source: the shell's Apps folder is the same list Start shows, with Windows' own display names.
void collectAppsFolder(QVector<RawRecord> &out, QVector<Rejection> &rej, int &entries)
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
            const QString parsing = takeString(parse).trimmed();
            item->Release();
            item = nullptr;
            ++entries;
            if (display.isEmpty() || parsing.isEmpty()) {
                rej.append({AppSource::AppsFolder, display, parsing, QStringLiteral("empty-entry")});
                continue;
            }
            RawRecord r;
            r.source = AppSource::AppsFolder;
            r.name = display;
            const QString id = normalizeAumid(parsing);
            if (looksLikeAumid(id)) {
                // packaged apps (and any app known only by AUMID): started through the shell, no .exe needed
                r.launch = kPackagedPrefix + id;
                out.append(std::move(r));
                continue;
            }
            if (parsing.startsWith(QLatin1String("http"), Qt::CaseInsensitive)) {
                rej.append({AppSource::AppsFolder, display, parsing, QStringLiteral("web-link")});
                continue;
            }
            QString path = expandKnownFolderPath(QDir::fromNativeSeparators(parsing));
            if (path.isEmpty()) {
                rej.append({AppSource::AppsFolder, display, parsing, QStringLiteral("unresolved-known-folder")});
                continue;
            }
            QString args;
            if (path.endsWith(QLatin1String(".lnk"), Qt::CaseInsensitive)) {
                const LinkInfo li = resolveShortcut(path);
                if (!li.target.isEmpty()) {
                    path = li.target;
                    args = li.args;
                }
            }
            if (!isLaunchableTarget(path)) {
                rej.append({AppSource::AppsFolder, display, path, QStringLiteral("non-program-target")});
                continue;
            }
            if (!QFileInfo::exists(path)) {
                rej.append({AppSource::AppsFolder, display, path, QStringLiteral("target-missing")});
                continue;
            }
            if (path.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive) && looksLikeInstallerExe(path)) {
                rej.append({AppSource::AppsFolder, display, path, QStringLiteral("installer-target")});
                continue;
            }
            r.launch = path;
            r.args = args;
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
void collectAppsFolder(QVector<RawRecord> &, QVector<Rejection> &, int &) {}
}
bool packagedAppExists(const QString &) { return false; }
bool launchPackagedApp(const QString &) { return false; }
QImage packagedAppIcon(const QString &, int) { return {}; }
#endif

QString ScanReport::toText() const
{
    static const char *names[] = {"StartMenu", "Uninstall", "AppPaths", "Packaged", "AppsFolder"};
    QString t;
    t += QStringLiteral("apps kept: %1, rejected: %2, shell Apps-folder entries: %3 (kept %4), %5 ms\n")
             .arg(apps.size()).arg(rejected.size()).arg(shellEntries).arg(shellEntriesKept).arg(elapsedMs);
    for (int i = 0; i < 5; ++i)
        t += QStringLiteral("  source %1: raw %2, in catalog %3\n").arg(QLatin1String(names[i])).arg(seenBySource.value(i)).arg(keptBySource.value(i));
    QHash<QString, int> byReason;
    for (const Rejection &r : rejected)
        byReason[r.reason]++;
    t += QStringLiteral("rejections by reason:\n");
    for (auto it = byReason.constBegin(); it != byReason.constEnd(); ++it)
        t += QStringLiteral("  %1: %2\n").arg(it.key()).arg(it.value());
    t += QStringLiteral("rejected entries:\n");
    for (const Rejection &r : rejected)
        t += QStringLiteral("  [%1] %2 | %3 | %4\n").arg(QLatin1String(names[int(r.source)]), r.reason, r.name, r.launch);
    return t;
}

ScanReport scanInstalledSoftwareReport()
{
    QElapsedTimer timer;
    timer.start();
    ScanReport rep;
    // Order matters: the Apps folder is the primary source (its names win and its entries define coverage),
    // then Start Menu shortcuts, then the registry. The first record for a program keeps its place.
    QVector<RawRecord> records;
    auto mark = [&](AppSource s, int before) { rep.seenBySource[int(s)] += int(records.size()) - before; };
    int before = int(records.size());
    collectAppsFolder(records, rep.rejected, rep.shellEntries);
    mark(AppSource::AppsFolder, before);
    before = int(records.size());
    collectStartMenu(records, rep.rejected);
    mark(AppSource::StartMenu, before);
    before = int(records.size());
    collectUninstall(records, rep.rejected);
    mark(AppSource::Uninstall, before);
    before = int(records.size());
    collectAppPaths(records, rep.rejected);
    mark(AppSource::AppPaths, before);
    rep.apps = mergeRecords(records, true, &rep.rejected);
    for (const ScannedApp &a : rep.apps) {
        for (int i = 0; i < 5; ++i)
            if (a.sources & (1 << i))
                rep.keptBySource[i]++;
        if (a.sources & (1 << int(AppSource::AppsFolder)))
            rep.shellEntriesKept++;
    }
    rep.elapsedMs = timer.elapsed();
    return rep;
}

QVector<ScannedApp> scanInstalledSoftware()
{
    return scanInstalledSoftwareReport().apps;
}
