#include "guardstore.h"

#include <QDateTime>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#ifdef Q_OS_WIN
#include <windows.h>
#include <wincrypt.h>
#endif

namespace Guard {

namespace {
class DpapiProtector : public ValueProtector
{
public:
    QByteArray protect(const QByteArray &plain, bool *ok) override { return run(plain, true, ok); }
    QByteArray unprotect(const QByteArray &blob, bool *ok) override { return run(blob, false, ok); }
private:
    static QByteArray run(const QByteArray &in, bool enc, bool *ok)
    {
        if (ok) *ok = false;
#ifdef Q_OS_WIN
        static BYTE entropyBytes[] = {'D','o','c','k','l','y','.','G','u','a','r','d'};
        DATA_BLOB e{DWORD(sizeof(entropyBytes)), entropyBytes};
        DATA_BLOB src{DWORD(in.size()), reinterpret_cast<BYTE *>(const_cast<char *>(in.constData()))};
        DATA_BLOB dst{0, nullptr};
        // Current-user scope (no CRYPTPROTECT_LOCAL_MACHINE): only this Windows account can decrypt.
        const BOOL r = enc ? CryptProtectData(&src, L"Dockly guard", &e, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &dst)
                           : CryptUnprotectData(&src, nullptr, &e, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &dst);
        if (!r) return {};
        QByteArray out(reinterpret_cast<const char *>(dst.pbData), int(dst.cbData));
        LocalFree(dst.pbData);
        if (ok) *ok = true;
        return out;
#else
        Q_UNUSED(in) Q_UNUSED(enc)
        return {};
#endif
    }
};
QString nowUtc() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODate); }
}

ValueProtector *dpapiProtector()
{
    static DpapiProtector p;
    return &p;
}

GuardStore::~GuardStore() { close(); }

void GuardStore::close()
{
    if (m_conn.isEmpty()) return;
    if (m_db.isOpen()) m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_conn);
    m_conn.clear();
}

bool GuardStore::open(const QString &dbPath, ValueProtector *protector, QString *error)
{
    close();
    m_prot = protector;
    if (!ensureLedgerSchema(dbPath, error)) return false;
    m_conn = QStringLiteral("guard-store-") + QUuid::createUuid().toString(QUuid::Id128);
    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_conn);
    m_db.setDatabaseName(dbPath);
    if (!m_db.open()) {
        if (error) *error = m_db.lastError().text();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(m_conn);
        m_conn.clear();
        return false;
    }
    exec(QStringLiteral("PRAGMA foreign_keys=ON"));
    exec(QStringLiteral("PRAGMA synchronous=FULL"));
    migratePlaintext();
    return true;
}

bool GuardStore::exec(const QString &sql, const QVariantList &args) const
{
    QSqlQuery q(m_db);
    q.prepare(sql);
    for (const QVariant &a : args) // SQLite columns are NOT NULL: a null QString must be bound as ''
        q.addBindValue(a.metaType().id() == QMetaType::QString && a.toString().isNull() ? QVariant(QString(QLatin1String(""))) : a);
    if (!q.exec()) { m_err = q.lastError().text(); return false; }
    return true;
}

qint64 GuardStore::addPlanned(const LedgerEntry &e, const QString &meta)
{
    bool ok = false;
    const QByteArray blob = m_prot ? m_prot->protect(e.originalValue.toUtf8(), &ok) : QByteArray();
    if (!ok) { m_err = QStringLiteral("could not protect the original value"); return 0; }
    m_db.transaction();
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO recovery_entries(card_id,source,location,name,original_enc,meta,original_hash,"
              "applied_removed,applied_hash,state,created_utc) VALUES(?,?,?,?,?,?,?,?,?,'planned',?)");
    q.addBindValue(e.cardId); q.addBindValue(sourceKey(e.source)); q.addBindValue(e.location.isNull() ? QString(QLatin1String("")) : e.location); q.addBindValue(e.name);
    q.addBindValue(blob); q.addBindValue(meta.isNull() ? QString(QLatin1String("")) : meta); q.addBindValue(e.originalHash);
    q.addBindValue(e.appliedRemoved ? 1 : 0); q.addBindValue(e.appliedHash.isNull() ? QString(QLatin1String("")) : e.appliedHash); q.addBindValue(nowUtc());
    if (!q.exec()) { m_err = q.lastError().text(); m_db.rollback(); return 0; }
    const qint64 id = q.lastInsertId().toLongLong();
    QSqlQuery ev(m_db);
    ev.prepare("INSERT INTO recovery_events(entry_id,ts_utc,kind,detail) VALUES(?,?,?,?)");
    ev.addBindValue(id); ev.addBindValue(nowUtc()); ev.addBindValue(QStringLiteral("planned")); ev.addBindValue(QString(QLatin1String("")));
    if (!ev.exec() || !m_db.commit()) { m_err = ev.lastError().text(); m_db.rollback(); return 0; }
    return id;
}

bool GuardStore::setState(qint64 id, const QString &state, const QString &kind, const QString &detail)
{
    m_db.transaction();
    QString col = state == QLatin1String("applied") ? QStringLiteral(", applied_utc=?")
                : state == QLatin1String("restored") ? QStringLiteral(", restored_utc=?") : QString();
    QSqlQuery q(m_db);
    q.prepare("UPDATE recovery_entries SET state=?" + col + " WHERE id=?");
    q.addBindValue(state);
    if (!col.isEmpty()) q.addBindValue(nowUtc());
    q.addBindValue(id);
    bool ok = q.exec();
    if (ok) ok = event(id, kind, detail);
    if (!ok) { m_err = q.lastError().text(); m_db.rollback(); return false; }
    return m_db.commit();
}

bool GuardStore::event(qint64 id, const QString &kind, const QString &detail)
{
    return exec(QStringLiteral("INSERT INTO recovery_events(entry_id,ts_utc,kind,detail) VALUES(?,?,?,?)"),
                {id, nowUtc(), kind, detail});
}

QVector<LedgerEntry> GuardStore::entries(const QString &cardId, const QString &state) const
{
    QVector<LedgerEntry> out;
    QSqlQuery q(m_db);
    q.prepare("SELECT id,card_id,source,location,name,original_hash,applied_removed,applied_hash,state FROM recovery_entries "
              "WHERE (?='' OR card_id=?) AND (?='' OR state=?) ORDER BY id");
    const QString c = cardId.isNull() ? QString(QLatin1String("")) : cardId, st = state.isNull() ? QString(QLatin1String("")) : state;
    q.addBindValue(c); q.addBindValue(c); q.addBindValue(st); q.addBindValue(st);
    if (!q.exec()) { m_err = q.lastError().text(); return out; }
    while (q.next()) {
        LedgerEntry e;
        e.id = q.value(0).toLongLong(); e.cardId = q.value(1).toString();
        const QString s = q.value(2).toString();
        e.source = s == QLatin1String("startup-folder") ? StartupSource::StartupFolder : StartupSource::RunKeyHkcu;
        e.location = q.value(3).toString(); e.name = q.value(4).toString();
        e.originalHash = q.value(5).toString(); e.appliedRemoved = q.value(6).toInt() != 0;
        e.appliedHash = q.value(7).toString(); e.state = q.value(8).toString();
        out << e;
    }
    return out;
}

bool GuardStore::entryOriginal(qint64 id, QString *value, QString *meta) const
{
    QSqlQuery q(m_db);
    q.prepare("SELECT original_enc,meta,original_hash FROM recovery_entries WHERE id=?");
    q.addBindValue(id);
    if (!q.exec() || !q.next()) return false;
    bool ok = false;
    const QByteArray plain = m_prot ? m_prot->unprotect(q.value(0).toByteArray(), &ok) : QByteArray();
    if (!ok) return false;
    const QString v = QString::fromUtf8(plain);
    if (hashOf(v) != q.value(2).toString()) return false; // protected blob no longer matches its recorded hash
    if (value) *value = v;
    if (meta) *meta = q.value(1).toString();
    return true;
}

int GuardStore::migratePlaintext()
{
    QSqlQuery q(m_db);
    if (!q.exec("SELECT id,original_value FROM recovery_entries WHERE original_value<>'' AND length(original_enc)=0"))
        return 0;
    QVector<QPair<qint64, QString>> rows;
    while (q.next()) rows.append({q.value(0).toLongLong(), q.value(1).toString()});
    int n = 0;
    m_db.transaction();
    for (const auto &r : rows) {
        bool ok = false;
        const QByteArray b = m_prot ? m_prot->protect(r.second.toUtf8(), &ok) : QByteArray();
        if (ok && exec(QStringLiteral("UPDATE recovery_entries SET original_enc=?, original_value='' WHERE id=?"), {b, r.first}))
            ++n;
    }
    m_db.commit();
    return n;
}

void GuardStore::logAction(const ActionRecord &r)
{
    exec(QStringLiteral("INSERT INTO action_log(ts_utc,card_id,process,pid,outcome,acted,explanation) VALUES(?,?,?,?,?,?,?)"),
         {nowUtc(), r.cardId, r.processName, r.pid, outcomeLabel(r.outcome), r.acted ? 1 : 0, r.explanation});
}

QVariantList GuardStore::recentActions(int limit) const
{
    QVariantList out;
    QSqlQuery q(m_db);
    q.prepare("SELECT ts_utc,card_id,process,pid,outcome,acted,explanation FROM action_log ORDER BY id DESC LIMIT ?");
    q.addBindValue(limit);
    if (!q.exec()) return out;
    while (q.next())
        out << QVariantMap{{"ts", q.value(0)}, {"cardId", q.value(1)}, {"process", q.value(2)}, {"pid", q.value(3)},
                           {"outcome", q.value(4)}, {"acted", q.value(5).toInt() != 0}, {"explanation", q.value(6)}};
    return out;
}

void GuardStore::setAllowed(const QString &cardId, const QString &exe, const QString &mode, bool on)
{
    const QString p = normPath(exe);
    if (on) exec(QStringLiteral("INSERT OR IGNORE INTO card_policy(card_id,exe_path,mode) VALUES(?,?,?)"), {cardId, p, mode});
    else exec(QStringLiteral("DELETE FROM card_policy WHERE card_id=? AND exe_path=? AND mode=?"), {cardId, p, mode});
}

QSet<QString> GuardStore::allowed(const QString &cardId, const QString &mode) const
{
    QSet<QString> out;
    QSqlQuery q(m_db);
    q.prepare("SELECT exe_path FROM card_policy WHERE card_id=? AND mode=?");
    q.addBindValue(cardId); q.addBindValue(mode);
    if (q.exec()) while (q.next()) out.insert(q.value(0).toString());
    return out;
}

void GuardStore::setCardSettings(const QString &cardId, const CardSettings &s)
{
    exec(QStringLiteral("INSERT OR REPLACE INTO card_settings(card_id,grace_ms,allow_force,open_enabled,force_ack) VALUES(?,?,?,?,?)"),
         {cardId, qBound(500, s.graceMs, 60000), s.allowForce ? 1 : 0, s.openEnabled ? 1 : 0, s.forceAck ? 1 : 0});
}

GuardStore::CardSettings GuardStore::cardSettings(const QString &cardId) const
{
    CardSettings s;
    QSqlQuery q(m_db);
    q.prepare("SELECT grace_ms,allow_force,open_enabled,force_ack FROM card_settings WHERE card_id=?");
    q.addBindValue(cardId);
    if (q.exec() && q.next()) {
        s.graceMs = q.value(0).toInt(); s.allowForce = q.value(1).toInt() != 0;
        s.openEnabled = q.value(2).toInt() != 0; s.forceAck = q.value(3).toInt() != 0;
    }
    return s;
}

QStringList GuardStore::cardsWithPolicy() const
{
    QStringList out;
    QSqlQuery q(m_db);
    if (q.exec("SELECT DISTINCT card_id FROM card_policy")) while (q.next()) out << q.value(0).toString();
    return out;
}

qint64 GuardStore::addSoft(const ActionRecord &r)
{
    QSqlQuery q(m_db);
    q.prepare("INSERT INTO soft_controls(card_id,pid,created,exe_path,prior_class,applied_utc) VALUES(?,?,?,?,?,?)");
    q.addBindValue(r.cardId); q.addBindValue(r.pid); q.addBindValue(qulonglong(r.created));
    q.addBindValue(r.exePath); q.addBindValue(r.priorClass); q.addBindValue(nowUtc());
    return q.exec() ? q.lastInsertId().toLongLong() : 0;
}

QVector<GuardStore::SoftRow> GuardStore::pendingSoft() const
{
    QVector<SoftRow> out;
    QSqlQuery q(m_db);
    if (q.exec("SELECT id,card_id,pid,created,exe_path,prior_class FROM soft_controls WHERE reverted_utc IS NULL"))
        while (q.next())
            out.append({q.value(0).toLongLong(), q.value(1).toString(), q.value(2).toUInt(), q.value(3).toULongLong(),
                        q.value(4).toString(), q.value(5).toInt()});
    return out;
}

void GuardStore::markSoftReverted(qint64 id)
{
    exec(QStringLiteral("UPDATE soft_controls SET reverted_utc=? WHERE id=?"), {nowUtc(), id});
}

}

namespace Guard {
int revertPendingSoft(GuardStore &store, ProcessOps *ops, QStringList *log)
{
    int n = 0;
    for (const auto &r : store.pendingSoft()) {
        const LiveIdentity id = ops->identity(r.pid);
        if (id.accessDenied || id.unreadable) { // state unknown: keep the record so a later restore can retry
            if (log) *log << QStringLiteral("PID %1 could not be inspected (access denied); record kept").arg(r.pid);
            continue;
        }
        if (id.alive && id.created == r.created && normPath(id.exePath) == normPath(r.exe)) {
            int dummy = 0; QString err;
            if (ops->setPriority(r.pid, r.created, r.prior, &dummy, &err)) { ++n; if (log) *log << QStringLiteral("Priority restored for PID %1").arg(r.pid); }
            else { if (log) *log << QStringLiteral("Could not restore PID %1: %2").arg(r.pid).arg(err); continue; }
        } else if (log) {
            *log << QStringLiteral("PID %1 is gone or was replaced; record closed").arg(r.pid);
        }
        store.markSoftReverted(r.id);
    }
    return n;
}
}
