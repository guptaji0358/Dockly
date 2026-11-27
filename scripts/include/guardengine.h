#pragma once

#include "procinspect.h"

#include <QHash>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

// Stage 3.1: read-only inventory facts + DRY-RUN decision engine.
// There is deliberately NO executor in this phase: nothing here terminates, suspends, throttles or
// modifies anything. Decisions are reports ("would do X because Y"); Decision::executed is always false.
// Command lines are never read or stored.
namespace Guard {

enum class Tri { Unknown, Yes, No };

// Per-process facts beyond Stage 2's ProcessRecord. All collected read-only.
struct ProcessFacts {
    quint32 pid = 0;
    quint64 created = 0;          // identity = pid + created
    int sessionId = -1;           // -1 = unreadable
    Tri terminateAccess = Tri::Unknown; // can a handle with PROCESS_TERMINATE be opened? (opened and closed at once; never used)
    bool queryDenied = false;     // even PROCESS_QUERY_LIMITED_INFORMATION was refused
};

enum class Action {
    Keep,      // protected: Main / Essential / Unknown / ambiguous / inaccessible
    Observe,   // Candidate-background, but nothing authorises action
    WouldStop  // dry-run only: every gate passed; this is a PROPOSAL, nothing is executed
};
QString actionLabel(Action a);

// Explicit, per-card policy. Candidate classification alone never authorises anything.
struct CardPolicy {
    QString cardId;
    bool guardOn = true;                 // the card's Background Process Guard switch
    QSet<QString> allowedExePaths;       // normalised lower-case native paths the user explicitly allowed
    int minAgeSeconds = 60;              // a process must be at least this old
};

struct Policy {
    bool masterOn = true;
    int currentSessionId = -1;           // only processes in the user's own session are eligible
    QHash<QString, CardPolicy> cards;    // by card id
};

struct Decision {
    ProcInspect::Verdict verdict;
    ProcessFacts facts;
    Action action = Action::Keep;
    QStringList blockers;                // gates that failed (empty only for WouldStop)
    QString explanation;
    bool executed = false;               // always false in Stage 3.1
};

struct DryRunReport {
    QVector<Decision> decisions;
    int keep = 0, observe = 0, wouldStop = 0;
    bool executedAnything = false;       // always false
};

QString normPath(const QString &p);

// Pure. `facts` may lack entries (treated as unreadable => protected). `nowFiletime` is the current time
// in FILETIME ticks, used for the age gate. Never modifies anything.
DryRunReport dryRun(const ProcInspect::Inspection &insp, const QHash<quint32, ProcessFacts> &facts,
                    const Policy &policy, quint64 nowFiletime);

// ---- Windows collectors (read-only) ----
QHash<quint32, ProcessFacts> collectFacts(const QVector<ProcInspect::ProcessRecord> &procs);
quint64 nowFiletime();
int currentSessionId();

// ---- process-start detection by polling ----
// Identity key: pid + creation time. Processes whose creation time is unreadable (created == 0) are
// reported as `uncertain` and never as reliably new.
struct PollDiff {
    QVector<ProcInspect::ProcessRecord> started, exited;
    int uncertain = 0;
};
PollDiff diffSnapshots(const QVector<ProcInspect::ProcessRecord> &before,
                       const QVector<ProcInspect::ProcessRecord> &after);

}
