#include "procinspect.h"
#include "softwarescanner.h"

#include <QDir>
#include <QElapsedTimer>
#include <QSet>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#include <dwmapi.h>
#endif

namespace ProcInspect {

#ifdef Q_OS_WIN
namespace {
quint64 ft(const FILETIME &f) { return (quint64(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

// Read-only handle: query-limited rights only, never terminate/suspend/write.
HANDLE openLimited(quint32 pid) { return OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid); }

BOOL CALLBACK windowProc(HWND h, LPARAM lp)
{
    auto *set = reinterpret_cast<QSet<quint32> *>(lp);
    if (!IsWindowVisible(h) || GetWindow(h, GW_OWNER) != nullptr)
        return TRUE;
    if (GetWindowLongW(h, GWL_EXSTYLE) & WS_EX_TOOLWINDOW)
        return TRUE;
    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(h, 14 /*DWMWA_CLOAKED*/, &cloaked, sizeof(cloaked))) && cloaked)
        return TRUE;
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid)
        set->insert(pid);
    return TRUE;
}
}
#endif

quint64 creationTimeOf(quint32 pid)
{
#ifdef Q_OS_WIN
    HANDLE h = openLimited(pid);
    if (!h)
        return 0;
    FILETIME c, e, k, u;
    const quint64 r = GetProcessTimes(h, &c, &e, &k, &u) ? ft(c) : 0;
    CloseHandle(h);
    return r;
#else
    Q_UNUSED(pid);
    return 0;
#endif
}

bool stillSameProcess(quint32 pid, quint64 created)
{
    const quint64 now = creationTimeOf(pid);
    return created != 0 && now != 0 && now == created;
}

QVector<ProcessRecord> collectProcesses(QString *error)
{
    QVector<ProcessRecord> out;
#ifdef Q_OS_WIN
    QSet<quint32> windows;
    EnumWindows(windowProc, reinterpret_cast<LPARAM>(&windows));

    using GetPfn = LONG(WINAPI *)(HANDLE, UINT32 *, PWSTR);
    static const GetPfn getFamily = reinterpret_cast<GetPfn>(
        reinterpret_cast<void *>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "GetPackageFamilyName")));

    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) {
        if (error) *error = QStringLiteral("Process snapshot failed (error %1)").arg(GetLastError());
        return out;
    }
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        ProcessRecord r;
        r.pid = pe.th32ProcessID;
        r.ppid = pe.th32ParentProcessID;
        r.name = QString::fromWCharArray(pe.szExeFile);
        r.hasWindow = windows.contains(r.pid);
        if (r.pid != 0) {
            if (HANDLE h = openLimited(r.pid)) {
                FILETIME c, e, k, u;
                if (GetProcessTimes(h, &c, &e, &k, &u))
                    r.created = ft(c);
                wchar_t buf[MAX_PATH * 4];
                DWORD len = DWORD(std::size(buf));
                if (QueryFullProcessImageNameW(h, 0, buf, &len))
                    r.exePath = QDir::toNativeSeparators(QString::fromWCharArray(buf, int(len)));
                if (getFamily) {
                    wchar_t fam[256];
                    UINT32 fl = UINT32(std::size(fam));
                    if (getFamily(h, &fl, fam) == ERROR_SUCCESS && fl > 1)
                        r.packageFamily = QString::fromWCharArray(fam, int(fl) - 1);
                }
                CloseHandle(h);
            }
        }
        out << r;
    }
    CloseHandle(snap);
#else
    if (error) *error = QStringLiteral("Process discovery is only available on Windows");
#endif
    return out;
}

Inspection inspectNow(const QVector<CardRef> &cards)
{
    QElapsedTimer t;
    t.start();
    QString err;
    const QVector<ProcessRecord> procs = collectProcesses(&err);
    Inspection r = attribute(procs, cards, [](const QString &p) { return executableInfo(p).publisher; });
    r.error = err;
    r.elapsedMs = t.elapsed();
    return r;
}

}
