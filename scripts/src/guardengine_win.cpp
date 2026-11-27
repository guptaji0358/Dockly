#include "guardengine.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace Guard {

quint64 nowFiletime()
{
#ifdef Q_OS_WIN
    FILETIME f;
    GetSystemTimeAsFileTime(&f);
    return (quint64(f.dwHighDateTime) << 32) | f.dwLowDateTime;
#else
    return 0;
#endif
}

int currentSessionId()
{
#ifdef Q_OS_WIN
    DWORD s = 0;
    return ProcessIdToSessionId(GetCurrentProcessId(), &s) ? int(s) : -1;
#else
    return -1;
#endif
}

QHash<quint32, ProcessFacts> collectFacts(const QVector<ProcInspect::ProcessRecord> &procs)
{
    QHash<quint32, ProcessFacts> out;
#ifdef Q_OS_WIN
    for (const auto &p : procs) {
        ProcessFacts f;
        f.pid = p.pid;
        f.created = p.created;
        DWORD s = 0;
        f.sessionId = ProcessIdToSessionId(p.pid, &s) ? int(s) : -1;
        f.queryDenied = p.pid != 0 && p.created == 0 && p.exePath.isEmpty();
        if (p.pid != 0 && !f.queryDenied) {
            // Access PROBE only: the handle is closed immediately and never used for anything.
            if (HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, p.pid)) {
                CloseHandle(h);
                f.terminateAccess = Tri::Yes;
            } else {
                f.terminateAccess = GetLastError() == ERROR_ACCESS_DENIED ? Tri::No : Tri::Unknown;
            }
        } else if (f.queryDenied) {
            f.terminateAccess = Tri::No;
        }
        out.insert(p.pid, f);
    }
#else
    Q_UNUSED(procs);
#endif
    return out;
}

}
