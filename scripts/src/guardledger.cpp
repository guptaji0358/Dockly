#include "guardledger.h"

#include <QCryptographicHash>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

namespace Guard {

QString sourceKey(StartupSource s)
{
    switch (s) {
    case StartupSource::RunKeyHkcu: return QStringLiteral("run-hkcu");
    case StartupSource::RunKeyHklm: return QStringLiteral("run-hklm");
    case StartupSource::StartupFolder: return QStringLiteral("startup-folder");
    case StartupSource::PackagedStartupTask: return QStringLiteral("packaged-task");
    case StartupSource::ScheduledTask: return QStringLiteral("scheduled-task");
    }
    return {};
}

QString hashOf(const QString &value)
{
    return QString::fromLatin1(QCryptographicHash::hash(value.toUtf8(), QCryptographicHash::Sha256).toHex());
}

StartupPlan planStartup(const StartupEntry &e)
{
    StartupPlan p;
    switch (e.source) {
    case StartupSource::PackagedStartupTask:
        p.managedByWindows = true;
        p.reason = QStringLiteral("Packaged app startup is managed by Windows (Settings > Apps > Startup); Dockly does not touch it.");
        break;
    case StartupSource::RunKeyHklm:
        p.reason = QStringLiteral("Machine-wide entry: changing it needs elevation, which Dockly never requests.");
        break;
    case StartupSource::ScheduledTask:
        p.reason = QStringLiteral("Scheduled tasks are not supported.");
        break;
    case StartupSource::RunKeyHkcu:
        p.actionable = e.present;
        p.reason = e.present ? QStringLiteral("User-writable, restorable.") : QStringLiteral("Entry not present.");
        break;
    case StartupSource::StartupFolder:
        p.actionable = e.present;
        p.reason = e.present ? QStringLiteral("User-writable, restorable.") : QStringLiteral("Entry not present.");
        break;
    }
    return p;
}

QString verdictLabel(RestoreVerdict v)
{
    switch (v) {
    case RestoreVerdict::SafeToRestore: return QStringLiteral("safe to restore");
    case RestoreVerdict::AlreadyRestored: return QStringLiteral("already restored");
    case RestoreVerdict::ConflictRecreated: return QStringLiteral("conflict: re-created by user");
    case RestoreVerdict::ConflictModified: return QStringLiteral("conflict: modified by user");
    case RestoreVerdict::ConflictMissing: return QStringLiteral("conflict: removed by user");
    case RestoreVerdict::RefusedManagedByWindows: return QStringLiteral("refused: managed by Windows");
    case RestoreVerdict::RefusedUnsupported: return QStringLiteral("refused: unsupported source");
    case RestoreVerdict::RefusedNotApplied: return QStringLiteral("refused: nothing was applied");
    }
    return {};
}

RestoreVerdict evaluateRestore(const LedgerEntry &e, const StartupEntry &cur)
{
    if (e.source == StartupSource::PackagedStartupTask)
        return RestoreVerdict::RefusedManagedByWindows;
    if (e.source == StartupSource::RunKeyHklm || e.source == StartupSource::ScheduledTask)
        return RestoreVerdict::RefusedUnsupported;
    if (e.state != QLatin1String("applied"))
        return RestoreVerdict::RefusedNotApplied;
    if (cur.present && hashOf(cur.value) == e.originalHash)
        return RestoreVerdict::AlreadyRestored;
    if (e.appliedRemoved)
        return cur.present ? RestoreVerdict::ConflictRecreated : RestoreVerdict::SafeToRestore;
    if (!cur.present)
        return RestoreVerdict::ConflictMissing;
    return hashOf(cur.value) == e.appliedHash ? RestoreVerdict::SafeToRestore : RestoreVerdict::ConflictModified;
}

QStringList ledgerSchema()
{
    return {
        "CREATE TABLE IF NOT EXISTS ledger_meta(key TEXT PRIMARY KEY, value TEXT NOT NULL)",
        "CREATE TABLE IF NOT EXISTS recovery_entries("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " card_id TEXT NOT NULL,"
        " source TEXT NOT NULL CHECK(source IN ('run-hkcu','startup-folder')),"   // only restorable sources can be recorded
        " location TEXT NOT NULL, name TEXT NOT NULL,"
        " original_value TEXT NOT NULL DEFAULT '',"          // legacy v1 plaintext column: always blank from v2 on
        " original_enc BLOB NOT NULL DEFAULT x'',"           // DPAPI (current user) protected original value
        " meta TEXT NOT NULL DEFAULT '', original_hash TEXT NOT NULL,"
        " applied_removed INTEGER NOT NULL CHECK(applied_removed IN (0,1)),"
        " applied_hash TEXT NOT NULL DEFAULT '',"
        " state TEXT NOT NULL CHECK(state IN ('planned','applied','restored','abandoned')),"
        " created_utc TEXT NOT NULL, applied_utc TEXT, restored_utc TEXT)",
        "CREATE INDEX IF NOT EXISTS idx_recovery_card ON recovery_entries(card_id)",
        "CREATE TABLE IF NOT EXISTS recovery_events("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " entry_id INTEGER NOT NULL REFERENCES recovery_entries(id),"
        " ts_utc TEXT NOT NULL, kind TEXT NOT NULL, detail TEXT NOT NULL DEFAULT '')",
        "CREATE TABLE IF NOT EXISTS action_log("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, ts_utc TEXT NOT NULL, card_id TEXT NOT NULL, process TEXT NOT NULL,"
        " pid INTEGER NOT NULL, outcome TEXT NOT NULL, acted INTEGER NOT NULL, explanation TEXT NOT NULL)",
        "CREATE TABLE IF NOT EXISTS card_policy("
        " card_id TEXT NOT NULL, exe_path TEXT NOT NULL, mode TEXT NOT NULL CHECK(mode IN ('closed-stop','open-lower','open-force')),"
        " PRIMARY KEY(card_id, exe_path, mode))",
        "CREATE TABLE IF NOT EXISTS card_settings("
        " card_id TEXT PRIMARY KEY, grace_ms INTEGER NOT NULL DEFAULT 5000, allow_force INTEGER NOT NULL DEFAULT 0,"
        " open_enabled INTEGER NOT NULL DEFAULT 0, force_ack INTEGER NOT NULL DEFAULT 0)",
        "CREATE TABLE IF NOT EXISTS soft_controls("
        " id INTEGER PRIMARY KEY AUTOINCREMENT, card_id TEXT NOT NULL, pid INTEGER NOT NULL, created INTEGER NOT NULL,"
        " exe_path TEXT NOT NULL, prior_class INTEGER NOT NULL, applied_utc TEXT NOT NULL, reverted_utc TEXT)",
    };
}

bool ensureLedgerSchema(const QString &dbPath, QString *error)
{
    const QString conn = QStringLiteral("guard-ledger-") + QUuid::createUuid().toString(QUuid::Id128);
    bool ok = true;
    {
        QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), conn);
        db.setDatabaseName(dbPath);
        if (!db.open()) {
            if (error) *error = db.lastError().text();
            ok = false;
        } else {
            QSqlQuery q(db);
            for (const QString &s : ledgerSchema())
                if (!q.exec(s)) { if (error) *error = q.lastError().text(); ok = false; break; }
            if (ok) { // v1 -> v2: add the protected-value columns to an old table
                bool hasEnc = false;
                q.exec("PRAGMA table_info(recovery_entries)");
                while (q.next()) hasEnc = hasEnc || q.value(1).toString() == QLatin1String("original_enc");
                if (!hasEnc) {
                    ok = q.exec("ALTER TABLE recovery_entries ADD COLUMN original_enc BLOB NOT NULL DEFAULT x''")
                      && q.exec("ALTER TABLE recovery_entries ADD COLUMN meta TEXT NOT NULL DEFAULT ''");
                    if (!ok && error) *error = q.lastError().text();
                }
            }
            if (ok) {
                q.prepare("INSERT OR REPLACE INTO ledger_meta(key,value) VALUES('schema_version',?)");
                q.addBindValue(QString::number(kLedgerSchemaVersion));
                ok = q.exec();
            }
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(conn);
    return ok;
}

}
