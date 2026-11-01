#include "runningapps.h"

#include <iterator>

#include <QDir>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

RunningApps::RunningApps(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(2000);
    connect(&m_timer, &QTimer::timeout, this, &RunningApps::refresh);
}

void RunningApps::setEnabled(bool on)
{
    if (m_enabled == on)
        return;
    m_enabled = on;
    if (on) {
        m_timer.start();
        refresh();
    } else {
        m_timer.stop();
        const bool had = !m_running.isEmpty();
        m_running.clear();
        m_windows.clear();
        if (had)
            emit changed();
    }
    emit enabledChanged();
}

QString RunningApps::exeKey(const QString &target)
{
    const auto cached = m_resolved.constFind(target);
    if (cached != m_resolved.cend())
        return cached.value();
    QString path = target;
    const QFileInfo fi(target);
    if (fi.suffix().compare(QLatin1String("lnk"), Qt::CaseInsensitive) == 0)
        path = fi.symLinkTarget();
    QString key;
    if (path.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive))
        key = QDir::toNativeSeparators(QDir::cleanPath(path)).toLower();
    m_resolved.insert(target, key);
    return key;
}

void RunningApps::setTargets(const QHash<QString, QString> &targets)
{
    m_keys.clear();
    for (auto it = targets.cbegin(); it != targets.cend(); ++it) {
        const QString key = exeKey(it.value());
        if (!key.isEmpty())
            m_keys.insert(it.key(), key);
    }
    if (m_enabled)
        refresh();
}

#ifdef Q_OS_WIN
namespace {
struct Scan {
    QHash<DWORD, QString> exeOfPid;
    QHash<QString, QList<quintptr>> windowsOfExe; // lower-case exe path -> windows, top-most first
};

QString imagePath(DWORD pid)
{
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h)
        return {};
    wchar_t buf[MAX_PATH * 2];
    DWORD len = DWORD(std::size(buf));
    QString out;
    if (QueryFullProcessImageNameW(h, 0, buf, &len))
        out = QDir::toNativeSeparators(QString::fromWCharArray(buf, int(len))).toLower();
    CloseHandle(h);
    return out;
}

// an "Alt+Tab" window: visible, top-level, not a tool window, not hidden by the shell (other virtual desktop / suspended app)
bool isAppWindow(HWND h)
{
    if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER) != nullptr || GetWindowTextLengthW(h) == 0)
        return false;
    if (GetWindowLongW(h, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return false;
    BOOL cloaked = FALSE;
    if (SUCCEEDED(DwmGetWindowAttribute(h, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked)
        return false;
    return true;
}

BOOL CALLBACK collect(HWND h, LPARAM lp)
{
    auto *scan = reinterpret_cast<Scan *>(lp);
    if (!isAppWindow(h))
        return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (!pid)
        return TRUE;
    auto it = scan->exeOfPid.find(pid);
    if (it == scan->exeOfPid.end())
        it = scan->exeOfPid.insert(pid, imagePath(pid));
    if (!it.value().isEmpty())
        scan->windowsOfExe[it.value()].append(reinterpret_cast<quintptr>(h));
    return TRUE;
}
} // namespace
#endif

void RunningApps::refresh()
{
    if (!m_enabled)
        return;
    QSet<QString> running;
    QHash<QString, QList<quintptr>> windows;
#ifdef Q_OS_WIN
    Scan scan;
    EnumWindows(collect, reinterpret_cast<LPARAM>(&scan)); // front-to-back
    for (auto it = m_keys.cbegin(); it != m_keys.cend(); ++it) {
        const auto w = scan.windowsOfExe.constFind(it.value());
        if (w != scan.windowsOfExe.cend()) {
            running.insert(it.key());
            windows.insert(it.key(), w.value());
        }
    }
#endif
    m_windows = windows;
    if (running != m_running) {
        m_running = running;
        emit changed();
    }
}

bool RunningApps::focus(const QString &id)
{
    if (!m_enabled)
        return false;
    refresh(); // the list may be up to two seconds old
#ifdef Q_OS_WIN
    const QList<quintptr> wins = m_windows.value(id);
    if (wins.isEmpty())
        return false;
    // already in front with other windows of the same program behind it: go to the next one
    const HWND fg = GetForegroundWindow();
    int pick = 0;
    const int at = wins.indexOf(reinterpret_cast<quintptr>(fg));
    if (at >= 0 && wins.size() > 1)
        pick = (at + 1) % wins.size();
    const HWND h = reinterpret_cast<HWND>(wins.at(pick));
    if (!IsWindow(h))
        return false;
    if (IsIconic(h))
        ShowWindow(h, SW_RESTORE);
    // Windows only lets the foreground thread hand out focus: attach to it for the moment
    const DWORD fgThread = GetWindowThreadProcessId(fg, nullptr);
    const DWORD me = GetCurrentThreadId();
    if (fgThread && fgThread != me)
        AttachThreadInput(me, fgThread, TRUE);
    SetForegroundWindow(h);
    BringWindowToTop(h);
    if (fgThread && fgThread != me)
        AttachThreadInput(me, fgThread, FALSE);
    if (GetForegroundWindow() != h)
        FlashWindow(h, TRUE); // refused (e.g. an elevated program): at least make its taskbar button blink
    return true;
#else
    Q_UNUSED(id)
    return false;
#endif
}
