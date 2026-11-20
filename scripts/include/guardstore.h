#pragma once

#include "guardexec.h"
#include "guardledger.h"

#include <QHash>
#include <QSqlDatabase>
#include <QVariantMap>

// Stage 3.3 / 3.6: persistent guard state in one SQLite file (separate from library.db), scoped to the
// current Windows user: original startup values are DPAPI-protected, so the file is useless to another
// account or machine. Multi-row changes run inside transactions.
namespace Guard {

class ValueProtector
{
public:
    virtual ~ValueProtector() = default;
    virtual QByteArray protect(const QByteArray &plain, bool *ok) = 0;
    virtual QByteArray unprotect(const QByteArray &blob, bool *ok) = 0;
};
ValueProtector *dpapiProtector();

class GuardStore;
// Crash recovery for soft controls: puts back the priority of every still-pending record whose process is the
// same one (pid + creation time + exe); records of processes that are gone or replaced are just closed.
int revertPendingSoft(GuardStore &store, ProcessOps *ops, QStringList *log = nullptr);

class GuardStore
{
public:
    GuardStore() = default;
    ~GuardStore();
    GuardStore(const GuardStore &) = delete;
    GuardStore &operator=(const GuardStore &) = delete;

    bool open(const QString &dbPath, ValueProtector *protector, QString *error = nullptr);
    bool isOpen() const { return m_db.isOpen(); }
    void close();

    // recovery entries
    qint64 addPlanned(const LedgerEntry &e, const QString &meta); // encrypts originalValue; state=planned
    bool setState(qint64 id, const QString &state, const QString &eventKind, const QString &detail = {});
    bool event(qint64 id, const QString &kind, const QString &detail = {});
    QVector<LedgerEntry> entries(const QString &cardId = {}, const QString &state = {}) const;
    bool entryOriginal(qint64 id, QString *value, QString *meta) const; // decrypts
    int migratePlaintext(); // v1 rows -> protected; returns rows converted

    // action log (outcomes; no command lines, no startup values)
    void logAction(const ActionRecord &r);
    QVariantList recentActions(int limit = 50) const;

    // per-card policy
    void setAllowed(const QString &cardId, const QString &exe, const QString &mode, bool on);
    QSet<QString> allowed(const QString &cardId, const QString &mode) const; // normalised exe paths
    struct CardSettings { int graceMs = 5000; bool allowForce = false; bool openEnabled = false; bool forceAck = false; };
    void setCardSettings(const QString &cardId, const CardSettings &s);
    CardSettings cardSettings(const QString &cardId) const;
    QStringList cardsWithPolicy() const;

    // soft controls (so a crash never leaves a lowered priority without a record)
    qint64 addSoft(const ActionRecord &r);
    struct SoftRow { qint64 id; QString cardId; quint32 pid; quint64 created; QString exe; int prior; };
    QVector<SoftRow> pendingSoft() const;
    void markSoftReverted(qint64 id);

    QString lastError() const { return m_err; }

private:
    bool exec(const QString &sql, const QVariantList &args = {}) const;
    QSqlDatabase m_db;
    QString m_conn;
    ValueProtector *m_prot = nullptr;
    mutable QString m_err;
};

}
