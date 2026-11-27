#include "guardexec.h"

#include <QDir>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Guard {

#ifdef Q_OS_WIN
namespace {
quint64 ft(const FILETIME &f) { return (quint64(f.dwHighDateTime) << 32) | f.dwLowDateTime; }

struct CloseCtx { DWORD pid; int n; };
BOOL CALLBACK closeProc(HWND h, LPARAM lp)
{
    auto *c = reinterpret_cast<CloseCtx *>(lp);
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid == c->pid && GetWindow(h, GW_OWNER) == nullptr && IsWindowVisible(h)) {
        if (PostMessageW(h, WM_CLOSE, 0, 0)) // asks politely; the app may refuse or show a save prompt
            ++c->n;
    }
    return TRUE;
}

// Opens with `access`, then proves the handle belongs to the process created at `created`.
HANDLE openVerified(quint32 pid, quint64 created, DWORD access, QString *error)
{
    HANDLE h = OpenProcess(access | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) {
        const DWORD err = GetLastError();
        if (error) *error = err == ERROR_ACCESS_DENIED ? QStringLiteral("access denied (error 5)")
                          : err == ERROR_INVALID_PARAMETER ? QStringLiteral("process no longer exists")
                          : QStringLiteral("could not open process (error %1)").arg(err);
        return nullptr;
    }
    FILETIME c, e, k, u;
    if (!GetProcessTimes(h, &c, &e, &k, &u) || ft(c) != created) {
        CloseHandle(h);
        if (error) *error = QStringLiteral("identity changed");
        return nullptr;
    }
    return h;
}

class SysOps : public ProcessOps
{
public:
    LiveIdentity identity(quint32 pid) override
    {
        LiveIdentity id;
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!h) {
            const DWORD err = GetLastError();
            if (err == ERROR_INVALID_PARAMETER) return id;           // no such process: exited
            if (err == ERROR_ACCESS_DENIED) id.accessDenied = true;  // exists but we may not look
            else id.unreadable = true;                               // anything else: unknown, never "exited"
            return id;
        }
        DWORD code = 0;
        FILETIME c, e, k, u;
        if (!GetExitCodeProcess(h, &code)) id.unreadable = true;
        else if (code == STILL_ACTIVE) {
            id.alive = true;
            if (GetProcessTimes(h, &c, &e, &k, &u)) id.created = ft(c);
            wchar_t buf[MAX_PATH * 4];
            DWORD len = DWORD(std::size(buf));
            if (QueryFullProcessImageNameW(h, 0, buf, &len))
                id.exePath = QDir::toNativeSeparators(QString::fromWCharArray(buf, int(len)));
        }
        CloseHandle(h);
        return id;
    }
    int requestClose(quint32 pid, quint64 created) override
    {
        QString err;
        if (HANDLE h = openVerified(pid, created, 0, &err)) CloseHandle(h); else return 0;
        CloseCtx c{pid, 0};
        EnumWindows(closeProc, reinterpret_cast<LPARAM>(&c));
        return c.n;
    }
    bool forceTerminate(quint32 pid, quint64 created, QString *error) override
    {
        HANDLE h = openVerified(pid, created, PROCESS_TERMINATE, error);
        if (!h) return false;
        const bool ok = TerminateProcess(h, 1) != 0;
        if (!ok && error) *error = QStringLiteral("TerminateProcess failed (error %1)").arg(GetLastError());
        CloseHandle(h);
        return ok;
    }
    bool setPriority(quint32 pid, quint64 created, int cls, int *prior, QString *error) override
    {
        HANDLE h = openVerified(pid, created, PROCESS_SET_INFORMATION, error);
        if (!h) return false;
        const DWORD old = GetPriorityClass(h);
        bool ok = old != 0 && SetPriorityClass(h, DWORD(cls)) != 0;
        if (ok && prior) *prior = int(old);
        if (!ok && error) *error = QStringLiteral("SetPriorityClass failed (error %1)").arg(GetLastError());
        CloseHandle(h);
        return ok;
    }
    void sleepMs(int ms) override { QThread::msleep(ulong(ms)); }
};
}

ProcessOps *systemOps()
{
    static SysOps ops;
    return &ops;
}
#else
ProcessOps *systemOps() { return nullptr; }
#endif

}
