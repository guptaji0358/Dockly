#pragma once

#include "guardengine.h"

#include <QSet>
#include <QString>
#include <QStringList>
#include <QVector>

// Stage 3.2 / 3.4: the executor. It only ever acts on a Decision that the pure engine marked WouldStop
// (closed-app) or on a process the user explicitly selected (open-app soft control). All OS access goes
// through ProcessOps, so tests use a fake adapter and never touch a real process.
// Command lines are never read. Nothing here restarts, blocks or prevents relaunches.
namespace Guard {

// What the adapter reports about a process right now.
struct LiveIdentity {
    bool alive = false;
    bool accessDenied = false; // the OS refused to open it: NOT the same as exited; identity is unknown
    bool unreadable = false;   // could not be queried for another reason (state unknown, not "exited")
    quint64 created = 0;     // 0 = unreadable
    QString exePath;         // empty = unreadable
};

class ProcessOps
{
public:
    virtual ~ProcessOps() = default;
    virtual LiveIdentity identity(quint32 pid) = 0;
    // Ask the process's top-level windows to close (WM_CLOSE). Returns how many windows were asked; 0 = no
    // graceful path exists (the caller must NOT silently fall back to force).
    virtual int requestClose(quint32 pid, quint64 created) = 0;
    // Terminates only if the handle's own creation time still equals `created` (re-verified on the handle).
    virtual bool forceTerminate(quint32 pid, quint64 created, QString *error) = 0;
    // Soft, reversible control. `prior` receives the previous priority class so it can be put back.
    virtual bool setPriority(quint32 pid, quint64 created, int priorityClass, int *prior, QString *error) = 0;
    virtual void sleepMs(int ms) = 0;
};

ProcessOps *systemOps(); // real Windows adapter (process-lifetime singleton)

enum class Outcome {
    Skipped,            // a gate failed at execution time; nothing was done
    IdentityChanged,    // pid/creation time/exe differ from the inspection: fail safe, nothing done
    AlreadyGone,        // exited before we acted
    AccessDenied,       // Windows refused access to the process: nothing done, state unknown (NOT "exited")
    IdentityUnavailable,// process is there but creation time / executable cannot be read: nothing done
    ClosedGracefully,   // exited within the grace period after WM_CLOSE
    NoGracefulPath,     // no window to ask and force not permitted: left running
    StillRunning,       // asked to close, still alive after grace, force not permitted: left running
    ForceTerminated,    // separate explicit force policy was set and used
    Failed,
    SoftApplied,        // open-app: priority lowered (reversible)
    SoftReverted
};
QString outcomeLabel(Outcome o);
bool outcomeActed(Outcome o); // true only when a process really changed state

struct ClosedPolicy {
    int graceMs = 5000;
    bool allowForce = false;   // separate, explicit; never implied by guardOn or by the allow-list
};

struct ActionRecord {
    QString cardId, processName;
    quint32 pid = 0;
    quint64 created = 0;
    Outcome outcome = Outcome::Skipped;
    bool acted = false;       // the process really changed state
    bool dryRunOnly = false;  // true when this record is a proposal, never claimed as done
    int priorClass = 0;       // SoftApplied: priority class to put back
    QString exePath;          // for soft-control revert identity checks
    QString explanation;
};

// Closed-app enforcement. `closedCards` = cards with no running Main process. Only decisions that are
// WouldStop AND belong to a closed card are eligible. Re-verifies identity per process right before acting.
QVector<ActionRecord> enforceClosed(const DryRunReport &rep, const QSet<QString> &closedCards,
                                    const QHash<QString, ClosedPolicy> &perCard, ProcessOps *ops);
// Cards that have no Main process in this inspection.
QSet<QString> closedCardsOf(const ProcInspect::Inspection &insp, const QSet<QString> &cardIds);

// ---- Stage 3.4 open-app ----
enum class Role { Protected, Observed, Eligible };
enum class Control { None, LowerPriority, ForceStop };

struct OpenPolicy {
    QString cardId;
    bool enabled = false;                   // default: observation and reporting only
    QHash<QString, Control> selected;       // normalised exe path -> control the USER picked
    bool forceAcknowledged = false;         // user accepted the warning for ForceStop
};
struct OpenDecision {
    ProcInspect::Verdict verdict;
    Role role = Role::Observed;
    Control control = Control::None;        // only meaningful when Eligible
    QString why;
    bool forceAcknowledged = false;         // copied from the policy by planOpen; applyOpen re-checks it itself
};
QString roleLabel(Role r);
QVector<OpenDecision> planOpen(const ProcInspect::Inspection &insp, const QHash<quint32, ProcessFacts> &facts,
                               const Policy &policy, const QHash<QString, OpenPolicy> &perCard, bool runningCardsOnly = true);
QVector<ActionRecord> applyOpen(const QVector<OpenDecision> &plan, ProcessOps *ops);

}
