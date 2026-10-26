#include "softwarescanner.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSet>
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
            if (target.isEmpty() || !target.endsWith(".exe", Qt::CaseInsensitive))
                continue;
            if (!QFileInfo::exists(target))
                continue;
            const QString name = link.completeBaseName();
            if (looksLikeNoise(name, target))
                continue;
            const QString key = QDir::cleanPath(target).toLower();
            if (seen.contains(key))
                continue;
            seen.insert(key);
            result.append({name, QDir::toNativeSeparators(QDir::cleanPath(target))});
        }
    }
    return result;
}
