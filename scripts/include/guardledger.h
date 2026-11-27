#pragma once

#include <QString>
#include <QStringList>

// Stage 3.1: recovery-ledger DESIGN and read-only logic. Nothing here touches Windows startup settings,
// and the application never opens a ledger on the user's profile in this phase (no entries are created).
// The ledger exists so a future restore can prove that nothing changed behind its back.
//
// Note: the ledger would hold a startup entry's own value (needed to restore it). That is not a process
// command line, which is never read or stored.
namespace Guard {

enum class StartupSource {
    RunKeyHkcu,          // HKCU\...\Run value (documented; user-writable)
    RunKeyHklm,          // needs elevation: never touched
    StartupFolder,       // shortcut in the user's Startup folder
    PackagedStartupTask, // MSIX/Store startup task: managed by Windows (Settings > Apps > Startup)
    ScheduledTask        // not supported
};
QString sourceKey(StartupSource s);

struct StartupEntry {
    StartupSource source = StartupSource::RunKeyHkcu;
    QString location;   // key path or folder
    QString name;       // value name / file name
    bool present = true;
    QString value;      // OPAQUE value data (Run value, or base64 of a shortcut file). Never parsed for arguments, logged or displayed.
    QString meta;       // e.g. registry type, file modification time
    QString exeHint;    // executable path only, in memory only, used for attribution; never stored or logged
};

// What a future action on a startup entry could be. Stage 3.1 only reports it.
struct StartupPlan {
    bool actionable = false;
    bool managedByWindows = false;
    QString reason;
};
StartupPlan planStartup(const StartupEntry &e);

QString hashOf(const QString &value);

// One ledger row (mirrors table recovery_entries).
struct LedgerEntry {
    qint64 id = 0;
    QString cardId;
    StartupSource source = StartupSource::RunKeyHkcu;
    QString location, name;
    QString originalValue, originalHash;
    bool appliedRemoved = true;   // we removed it (true) or rewrote it (false)
    QString appliedHash;          // hash expected to be present after our change; empty when removed
    QString state = QStringLiteral("applied"); // planned | applied | restored | abandoned
};

enum class RestoreVerdict {
    SafeToRestore,
    AlreadyRestored,        // current state already equals the original
    ConflictRecreated,      // we removed it, but something is there now that is not the original
    ConflictModified,       // we rewrote it, and the value is no longer what we wrote
    ConflictMissing,        // we rewrote it, and it has since vanished
    RefusedManagedByWindows,
    RefusedUnsupported,
    RefusedNotApplied
};
QString verdictLabel(RestoreVerdict v);

// Pure: compares the live state with what the ledger expects. Any difference is a conflict: the user's
// later change always wins and a restore must not overwrite it.
RestoreVerdict evaluateRestore(const LedgerEntry &e, const StartupEntry &current);

// Schema (SQLite). Creating it is the only write this module can do, and only on a path the caller gives.
QStringList ledgerSchema();
bool ensureLedgerSchema(const QString &dbPath, QString *error = nullptr);
constexpr int kLedgerSchemaVersion = 2; // v2: original values are stored protected (DPAPI), never as plaintext

}
