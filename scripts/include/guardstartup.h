#pragma once

#include "guardstore.h"

#include <memory>

// Stage 3.3: optional, reversible, per-user startup management. Scope: HKCU Run values and the current
// user's Startup folder only. Values are opaque: Dockly never parses or shows launch arguments, and never
// logs a value. Nothing happens unless the caller passes consent=true (a separate opt-in from the guard).
namespace Guard {

class StartupBackend
{
public:
    virtual ~StartupBackend() = default;
    virtual QVector<StartupEntry> list() = 0;                            // current user's entries (value filled, opaque)
    virtual bool read(StartupSource s, const QString &name, StartupEntry *out) = 0; // out->present=false when absent
    virtual bool remove(StartupSource s, const QString &name, QString *err) = 0;
    virtual bool write(StartupSource s, const QString &name, const QString &value, const QString &meta, QString *err) = 0;
};
// Real backend. Paths are injectable so tests run on an isolated temporary registry key and folder.
// Defaults: HKCU\Software\Microsoft\Windows\CurrentVersion\Run and the user's Startup folder.
std::unique_ptr<StartupBackend> makeSystemBackend(const QString &runSubkey = {}, const QString &folder = {});

// Executable path only (in memory); empty when it cannot be determined reliably. Arguments are discarded.
QString exeOfValue(const QString &runValue);

enum class Attribution { None, Unattributed, Ambiguous, Attributed };
struct StartupAttr { Attribution kind = Attribution::None; QString cardId, cardName, why; };
StartupAttr attributeStartup(const StartupEntry &e, const QVector<ProcInspect::CardRef> &cards);

// What the UI shows for one startup entry. The value is NEVER included.
struct StartupRow {
    StartupSource source = StartupSource::RunKeyHkcu;
    QString name, location, cardId, cardName;
    QString status;   // "available" | "managed" | "unattributed" | "ambiguous"
    QString why;
};

struct StartupResult {
    bool ok = false;
    QString message;
    RestoreVerdict verdict = RestoreVerdict::RefusedNotApplied;
    bool conflict = false;
};

class StartupManager
{
public:
    StartupManager(GuardStore *store, StartupBackend *backend) : m_store(store), m_be(backend) {}
    QVector<StartupRow> rows(const QVector<ProcInspect::CardRef> &cards);
    // Disables (removes) one attributed entry after recording its protected original. consent must be true.
    StartupResult disable(StartupSource s, const QString &name, const QVector<ProcInspect::CardRef> &cards, bool consent);
    StartupResult restore(qint64 entryId);
    QVector<StartupResult> restoreAll();
    // Crash recovery: resolves 'planned' rows left by an interrupted disable. Returns what it did.
    QStringList recover();
    // Consistency: every applied row must decrypt and match its hash. Returns human-readable problems.
    QStringList checkConsistency();
private:
    GuardStore *m_store;
    StartupBackend *m_be;
};

}
