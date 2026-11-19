#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>
#include <functional>

// Stage 2: READ-ONLY process discovery and attribution.
// Nothing here terminates, blocks, suspends or modifies a process, and nothing touches the registry,
// services or scheduled tasks. Command lines are never read and never stored (they can hold secrets);
// attribution uses only the executable path, install folder, publisher, package identity and the
// parent relationship. A file name alone is never evidence of ownership.
namespace ProcInspect {

enum class Class { Main, Essential, CandidateBackground, Unknown };
QString classKey(Class c);   // main | essential | candidate | unknown
QString classLabel(Class c); // Main | Essential | Candidate-background | Unknown

struct ProcessRecord {
    quint32 pid = 0;
    quint32 ppid = 0;
    quint64 created = 0;     // process creation time (FILETIME ticks); with pid it is the process identity
    QString name;            // file name only, for display
    QString exePath;         // empty when Windows does not let us read it (protected / elevated process)
    QString packageFamily;   // Store/MSIX package family name, when the process is packaged
    bool hasWindow = false;  // owns a visible top-level window
};

// A Software card, reduced to what attribution needs.
struct CardRef {
    QString id;
    QString name;
    QString exePath;       // the card's executable (resolved from a .lnk); empty if the target is not an exe
    QString installDir;    // install folder; empty when it would be a shared location (never used then)
    QString publisher;
    QString packageFamily; // for shell:AppsFolder\<AUMID> targets
};

struct Verdict {
    ProcessRecord proc;
    Class cls = Class::Unknown;
    QString cardId, cardName;      // empty when unattributed
    QStringList evidence;          // independent signals that were found
    QString reason;                // why it was identified, allowed or left Unknown
    QStringList ambiguousCards;    // names of the cards that tie for it (shared / ambiguous ownership)
    bool parentValid = false;      // parent exists and was created before the child (PID-reuse safe)
    QString parentName;
    QString key() const { return QString::number(proc.pid) + QLatin1Char('-') + QString::number(proc.created); }
};

struct Inspection {
    QVector<Verdict> verdicts;
    int processCount = 0;
    qint64 elapsedMs = 0;
    QString error;
};

// --- pure logic (unit-tested with synthetic records) ----------------------------------------------
// `publisherOf` is only called for processes that already sit inside some card's install folder.
Inspection attribute(const QVector<ProcessRecord> &procs, const QVector<CardRef> &cards,
                     const std::function<QString(const QString &)> &publisherOf = {});

// True for folders shared by many programs (Windows, Program Files root, user profile roots...). An
// install folder like that is never used as a signal.
bool isSharedLocation(const QString &dir);
// Package family from an AppUserModelID ("Family!App" -> "Family"); empty if there is none.
QString packageFamilyOfAumid(const QString &aumid);
// Reduces a card to a CardRef (resolves .lnk, reads publisher, rejects shared install folders).
CardRef makeCardRef(const QString &id, const QString &name, const QString &target);

// --- Windows collectors (read-only) -----------------------------------------------------------------
QVector<ProcessRecord> collectProcesses(QString *error = nullptr);
// PID-reuse check: is `pid` still the same process that was created at `created`?
bool stillSameProcess(quint32 pid, quint64 created);
quint64 creationTimeOf(quint32 pid); // 0 when unreadable

// Collect + attribute in one call. Safe on a worker thread.
Inspection inspectNow(const QVector<CardRef> &cards);

}
