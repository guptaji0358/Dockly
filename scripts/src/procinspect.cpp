#include "procinspect.h"
#include "softwarescanner.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QSet>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace ProcInspect {

QString classKey(Class c)
{
    switch (c) {
    case Class::Main: return QStringLiteral("main");
    case Class::Essential: return QStringLiteral("essential");
    case Class::CandidateBackground: return QStringLiteral("candidate");
    default: return QStringLiteral("unknown");
    }
}

QString classLabel(Class c)
{
    switch (c) {
    case Class::Main: return QStringLiteral("Main");
    case Class::Essential: return QStringLiteral("Essential");
    case Class::CandidateBackground: return QStringLiteral("Candidate-background");
    default: return QStringLiteral("Unknown");
    }
}

static QString norm(const QString &p)
{
    return QDir::toNativeSeparators(QDir::cleanPath(p)).toLower();
}

bool isSharedLocation(const QString &dir)
{
    if (dir.trimmed().isEmpty())
        return true;
    const QString d = norm(dir);
    QStringList roots; // a folder equal to, or above, one of these is shared
    for (const char *v : {"SystemRoot", "ProgramFiles", "ProgramFiles(x86)", "ProgramW6432", "ProgramData",
                          "LOCALAPPDATA", "APPDATA", "USERPROFILE", "PUBLIC", "SystemDrive"}) {
        const QString e = qEnvironmentVariable(v);
        if (!e.isEmpty())
            roots << norm(e);
    }
    roots << QStringLiteral("c:\\windows");
    for (const QString &r : roots) {
        if (d == r || r.startsWith(d + QLatin1Char('\\')))
            return true;
    }
    // the Windows folder and everything in it (System32, SysWOW64, WinSxS ...) is shared
    const QString win = qEnvironmentVariable("SystemRoot").isEmpty() ? QStringLiteral("c:\\windows")
                                                                      : norm(qEnvironmentVariable("SystemRoot"));
    if (d == win || d.startsWith(win + QLatin1Char('\\')))
        return true;
    if (d.length() <= 3) // a drive root
        return true;
    const QString lower = d;
    for (const char *common : {"\\users\\public", "\\downloads", "\\desktop", "\\documents"})
        if (lower.endsWith(QLatin1String(common)))
            return true;
    return false;
}

QString packageFamilyOfAumid(const QString &aumid)
{
    const int i = aumid.indexOf(QLatin1Char('!'));
    return i > 0 ? aumid.left(i) : QString();
}

CardRef makeCardRef(const QString &id, const QString &name, const QString &target)
{
    CardRef c;
    c.id = id;
    c.name = name;
    if (isPackagedTarget(target)) {
        c.packageFamily = packageFamilyOfAumid(packagedAppId(target));
        return c;
    }
    QString exe = target;
    if (exe.endsWith(QLatin1String(".lnk"), Qt::CaseInsensitive))
        exe = resolveShortcut(exe).target;
    if (!exe.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        return c; // a script, .msc or URL: no executable to match, so nothing can be attributed
    c.exePath = QDir::toNativeSeparators(exe);
    const QString dir = QFileInfo(exe).absolutePath();
    if (!isSharedLocation(dir))
        c.installDir = QDir::toNativeSeparators(dir);
    c.publisher = executableInfo(exe).publisher;
    return c;
}

}
