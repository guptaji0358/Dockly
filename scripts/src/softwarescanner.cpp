#include "softwarescanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
bool looksLikeNoise(const QString &name, const QString &target)
{
    static const QStringList noise = {
        "uninstall", "uninst", "readme", "release notes", "documentation", "manual",
        "help", "website", "license", "changelog", "what's new", "repair", "setup",
    };
    const QString n = name.toLower();
    for (const QString &w : noise)
        if (n.contains(w))
            return true;
    const QString base = QFileInfo(target).fileName().toLower();
    return base.startsWith("unins") || base == "msiexec.exe";
}
} // namespace

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
    for (const QString &key : {QStringLiteral("ProductName"), QStringLiteral("FileDescription")}) {
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
    }
#else
    Q_UNUSED(exePath)
#endif
    return {};
}

QVector<ScannedApp> scanInstalledSoftware()
{
    QVector<ScannedApp> result;
    QSet<QString> seen;

    const QStringList roots = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString &root : roots) {
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
            const QString launch = exeTarget ? target : link.absoluteFilePath();
            if (looksLikeNoise(name, launch))
                continue;
            const QString key = QDir::cleanPath(exeTarget ? target : launch).toLower();
            if (seen.contains(key))
                continue;
            seen.insert(key);
            result.append({name, QDir::toNativeSeparators(QDir::cleanPath(launch))});
        }
    }

    // Programs without a Start Menu shortcut: registry "Uninstall" entries and "App Paths".
    auto addExe = [&](QString name, QString exe, bool fromAppPaths = false) {
        exe = exe.trimmed();
        if (exe.startsWith('"'))
            exe = exe.section('"', 1, 1);
        const int comma = exe.lastIndexOf(',');
        if (comma > 0 && !exe.endsWith(".exe", Qt::CaseInsensitive))
            exe = exe.left(comma);
        exe = QDir::cleanPath(exe);
        if (name.isEmpty() || !exe.endsWith(".exe", Qt::CaseInsensitive) || !QFileInfo::exists(exe))
            return;
        if (fromAppPaths) {
            const QString friendly = executableDisplayName(exe);
            if (!friendly.isEmpty())
                name = friendly;
        }
        if (looksLikeNoise(name, exe))
            return;
        const QString key = exe.toLower();
        if (seen.contains(key))
            return;
        seen.insert(key);
        result.append({name, QDir::toNativeSeparators(exe)});
    };

    const QStringList uninstallRoots = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    };
    for (const QString &root : uninstallRoots) {
        QSettings reg(root, QSettings::NativeFormat);
        for (const QString &sub : reg.childGroups()) {
            reg.beginGroup(sub);
            if (reg.value("SystemComponent").toInt() != 1) {
                const QString display = reg.value("DisplayName").toString();
                QString icon = reg.value("DisplayIcon").toString();
                if (!icon.contains(".exe", Qt::CaseInsensitive)) {
                    // No usable DisplayIcon: look in InstallLocation for an exe named like the app.
                    const QString dir = reg.value("InstallLocation").toString().trimmed().remove('"');
                    if (!dir.isEmpty() && QDir(dir).exists()) {
                        const QString squashed = QString(display).remove(' ').toLower();
                        const QStringList exes = QDir(dir).entryList({"*.exe"}, QDir::Files);
                        for (const QString &e : exes) {
                            const QString b = QFileInfo(e).completeBaseName().toLower();
                            if (!b.isEmpty() && (squashed.contains(b) || b.contains(squashed))) {
                                icon = QDir(dir).filePath(e);
                                break;
                            }
                        }
                    }
                }
                addExe(display, icon);
            }
            reg.endGroup();
        }
    }

    const QStringList appPathRoots = {
        "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
        "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths",
    };
    for (const QString &root : appPathRoots) {
        QSettings reg(root, QSettings::NativeFormat);
        for (const QString &sub : reg.childGroups()) {
            reg.beginGroup(sub);
            addExe(QFileInfo(sub).completeBaseName(), reg.value("Default").toString(), true);
            reg.endGroup();
        }
    }
    return result;
}
