#include "softwarescanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>

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
    auto addExe = [&](QString name, QString exe) {
        exe = exe.trimmed();
        if (exe.startsWith('"'))
            exe = exe.section('"', 1, 1);
        const int comma = exe.lastIndexOf(',');
        if (comma > 0 && !exe.endsWith(".exe", Qt::CaseInsensitive))
            exe = exe.left(comma);
        exe = QDir::cleanPath(exe);
        if (name.isEmpty() || !exe.endsWith(".exe", Qt::CaseInsensitive) || !QFileInfo::exists(exe))
            return;
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
            addExe(QFileInfo(sub).completeBaseName(), reg.value("Default").toString());
            reg.endGroup();
        }
    }
    return result;
}
