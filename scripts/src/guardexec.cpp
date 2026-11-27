#include "guardexec.h"

namespace Guard {

QString outcomeLabel(Outcome o)
{
    switch (o) {
    case Outcome::Skipped: return QStringLiteral("Skipped (nothing done)");
    case Outcome::IdentityChanged: return QStringLiteral("Identity changed (nothing done)");
    case Outcome::AlreadyGone: return QStringLiteral("Already exited");
    case Outcome::AccessDenied: return QStringLiteral("NOT DONE: access denied (process left untouched)");
    case Outcome::IdentityUnavailable: return QStringLiteral("NOT DONE: identity unavailable (process left untouched)");
    case Outcome::ClosedGracefully: return QStringLiteral("Closed gracefully");
    case Outcome::NoGracefulPath: return QStringLiteral("No graceful path (left running)");
    case Outcome::StillRunning: return QStringLiteral("Still running after grace period (left running)");
    case Outcome::ForceTerminated: return QStringLiteral("Force-terminated (explicit policy)");
    case Outcome::Failed: return QStringLiteral("Failed");
    case Outcome::SoftApplied: return QStringLiteral("Priority lowered (reversible)");
    case Outcome::SoftReverted: return QStringLiteral("Priority restored");
    }
    return {};
}

bool outcomeActed(Outcome o)
{
    return o == Outcome::ClosedGracefully || o == Outcome::ForceTerminated || o == Outcome::SoftApplied
        || o == Outcome::SoftReverted;
}

QString roleLabel(Role r)
{
    switch (r) {
    case Role::Protected: return QStringLiteral("Protected");
    case Role::Observed: return QStringLiteral("Observed");
    case Role::Eligible: return QStringLiteral("Eligible (user-selected)");
    }
    return {};
}

QSet<QString> closedCardsOf(const ProcInspect::Inspection &insp, const QSet<QString> &cardIds)
{
    QSet<QString> open;
    for (const auto &v : insp.verdicts)
        if (v.cls == ProcInspect::Class::Main && !v.cardId.isEmpty())
            open.insert(v.cardId);
    QSet<QString> closed = cardIds;
    closed.subtract(open);
    return closed;
}

namespace {
enum class IdState { Same, Exited, AccessDenied, Unavailable, Mismatch };

// Same pid, same creation time, same executable as at inspection. Distinct failures are never conflated.
IdState checkIdentity(ProcessOps *ops, const ProcInspect::ProcessRecord &p, QString *why)
{
    const LiveIdentity id = ops->identity(p.pid);
    if (id.accessDenied) { *why = QStringLiteral("Windows denied access to the process, so its identity could not be verified"); return IdState::AccessDenied; }
    if (id.unreadable) { *why = QStringLiteral("the process could not be queried, so its identity could not be verified"); return IdState::Unavailable; }
    if (!id.alive) { *why = QStringLiteral("process exited"); return IdState::Exited; }
    if (p.created == 0 || id.created == 0 || p.exePath.isEmpty() || id.exePath.isEmpty()) {
        *why = QStringLiteral("creation time or executable path is unreadable, so identity could not be verified");
        return IdState::Unavailable;
    }
    if (id.created != p.created) { *why = QStringLiteral("creation time differs (PID reused)"); return IdState::Mismatch; }
    if (normPath(id.exePath) != normPath(p.exePath)) { *why = QStringLiteral("executable differs"); return IdState::Mismatch; }
    return IdState::Same;
}

Outcome outcomeFor(IdState s)
{
    switch (s) {
    case IdState::Exited: return Outcome::AlreadyGone;
    case IdState::AccessDenied: return Outcome::AccessDenied;
    case IdState::Unavailable: return Outcome::IdentityUnavailable;
    default: return Outcome::IdentityChanged;
    }
}

ActionRecord baseRecord(const ProcInspect::Verdict &v)
{
    ActionRecord r;
    r.cardId = v.cardId; r.processName = v.proc.name; r.pid = v.proc.pid; r.created = v.proc.created;
    r.exePath = v.proc.exePath;
    return r;
}
}

QVector<ActionRecord> enforceClosed(const DryRunReport &rep, const QSet<QString> &closedCards,
                                    const QHash<QString, ClosedPolicy> &perCard, ProcessOps *ops)
{
    QVector<ActionRecord> out;
    for (const Decision &d : rep.decisions) {
        if (d.action != Action::WouldStop)
            continue;
        ActionRecord r = baseRecord(d.verdict);
        auto done = [&](Outcome o, const QString &why) {
            r.outcome = o; r.acted = outcomeActed(o); r.explanation = why; out << r;
        };
        if (!closedCards.contains(d.verdict.cardId)) {
            done(Outcome::Skipped, QStringLiteral("The program is open: closed-app policy does not apply."));
            continue;
        }
        const ClosedPolicy cp = perCard.value(d.verdict.cardId);
        QString why;
        if (const IdState s = checkIdentity(ops, d.verdict.proc, &why); s != IdState::Same) {
            done(outcomeFor(s), QStringLiteral("Not acted on: %1.").arg(why));
            continue;
        }
        const int asked = ops->requestClose(d.verdict.proc.pid, d.verdict.proc.created);
        if (asked > 0) {
            int waited = 0;
            bool gone = false, lost = false;
            while (waited < cp.graceMs) {
                ops->sleepMs(100); waited += 100;
                const LiveIdentity id = ops->identity(d.verdict.proc.pid);
                if (id.accessDenied || id.unreadable) { lost = true; break; } // state unknown: never call it exited
                if (!id.alive || id.created != d.verdict.proc.created) { gone = true; break; }
            }
            if (lost) {
                done(Outcome::IdentityUnavailable, QStringLiteral("NOT DONE: asked to close, but the process can no longer be inspected; its state is unknown."));
                continue;
            }
            if (gone) { done(Outcome::ClosedGracefully, QStringLiteral("Asked %1 window(s) to close; it exited.").arg(asked)); continue; }
        }
        if (!cp.allowForce) {
            done(asked > 0 ? Outcome::StillRunning : Outcome::NoGracefulPath,
                 asked > 0 ? QStringLiteral("Asked to close; still running after %1 ms. Forced termination is not enabled for this card.").arg(cp.graceMs)
                           : QStringLiteral("It has no window to ask. Forced termination is not enabled for this card, so it was left running."));
            continue;
        }
        if (const IdState s = checkIdentity(ops, d.verdict.proc, &why); s != IdState::Same) { // re-verify before the forced step
            done(s == IdState::Exited ? Outcome::ClosedGracefully : outcomeFor(s),
                 QStringLiteral("Not force-terminated: %1.").arg(why));
            continue;
        }
        QString err;
        if (ops->forceTerminate(d.verdict.proc.pid, d.verdict.proc.created, &err))
            done(Outcome::ForceTerminated, QStringLiteral("Graceful close did not work; terminated under this card's explicit force policy."));
        else
            done(Outcome::Failed, QStringLiteral("Force termination failed: %1").arg(err));
    }
    return out;
}

QVector<OpenDecision> planOpen(const ProcInspect::Inspection &insp, const QHash<quint32, ProcessFacts> &facts,
                               const Policy &policy, const QHash<QString, OpenPolicy> &perCard, bool)
{
    using ProcInspect::Class;
    QVector<OpenDecision> out;
    for (const auto &v : insp.verdicts) {
        if (v.cardId.isEmpty() && v.ambiguousCards.isEmpty())
            continue; // unrelated processes are not part of any card's report
        OpenDecision d;
        d.verdict = v;
        auto prot = [&](const QString &w) { d.role = Role::Protected; d.why = w; out << d; };
        if (v.cls == Class::Main) { prot(QStringLiteral("Main program: always protected.")); continue; }
        if (v.cls == Class::Essential) { prot(QStringLiteral("Essential ancestor of the main program: protected.")); continue; }
        if (!v.ambiguousCards.isEmpty()) { prot(QStringLiteral("Shared by several programs: protected.")); continue; }
        if (v.cls != Class::CandidateBackground) { prot(QStringLiteral("Unknown role: left untouched.")); continue; }
        if (v.proc.hasWindow) { prot(QStringLiteral("Has a visible window: protected.")); continue; }
        const ProcessFacts f = facts.value(v.proc.pid);
        if (!facts.contains(v.proc.pid) || f.queryDenied || f.sessionId <= 0
            || (policy.currentSessionId >= 0 && f.sessionId != policy.currentSessionId) || v.proc.created == 0) {
            prot(QStringLiteral("Cannot be inspected or is outside your session: protected.")); continue;
        }
        const OpenPolicy op = perCard.value(v.cardId);
        d.role = Role::Observed;
        if (!policy.masterOn || !policy.cards.value(v.cardId).guardOn)
            d.why = QStringLiteral("Guard is off for this card: observation only.");
        else if (!op.enabled)
            d.why = QStringLiteral("Open-app management is off: observation only. Having no window does not make a helper unnecessary.");
        else if (!op.selected.contains(normPath(v.proc.exePath)))
            d.why = QStringLiteral("Not selected by you: observed only.");
        else {
            const Control c = op.selected.value(normPath(v.proc.exePath));
            if (c == Control::ForceStop && !op.forceAcknowledged)
                d.why = QStringLiteral("Force stop was chosen but its warning was not acknowledged: observed only.");
            else if (c == Control::ForceStop && f.terminateAccess != Tri::Yes)
                d.why = QStringLiteral("Windows does not allow terminating it: observed only.");
            else if (c == Control::None)
                d.why = QStringLiteral("No restriction chosen: observed only.");
            else {
                d.role = Role::Eligible; d.control = c;
                d.forceAcknowledged = op.forceAcknowledged;
                d.why = c == Control::LowerPriority
                    ? QStringLiteral("You selected it for a reversible priority reduction.")
                    : QStringLiteral("You selected it for forced stop (may break the program).");
            }
        }
        out << d;
    }
    return out;
}

QVector<ActionRecord> applyOpen(const QVector<OpenDecision> &plan, ProcessOps *ops)
{
    QVector<ActionRecord> out;
    for (const OpenDecision &d : plan) {
        if (d.role != Role::Eligible)
            continue;
        ActionRecord r = baseRecord(d.verdict);
        auto done = [&](Outcome o, const QString &why) {
            r.outcome = o; r.acted = outcomeActed(o); r.explanation = why; out << r;
        };
        QString why, err;
        // The executor validates the acknowledgement itself; a hand-built or stale plan cannot skip it.
        if (d.control == Control::ForceStop && !d.forceAcknowledged) {
            done(Outcome::Skipped, QStringLiteral("Force stop requires the user's acknowledgement; process left untouched."));
            continue;
        }
        if (const IdState s = checkIdentity(ops, d.verdict.proc, &why); s != IdState::Same) {
            done(outcomeFor(s), QStringLiteral("Not acted on: %1.").arg(why));
            continue;
        }
        if (d.control == Control::LowerPriority) {
            int prior = 0;
            if (ops->setPriority(d.verdict.proc.pid, d.verdict.proc.created, 0x4000 /*BELOW_NORMAL*/, &prior, &err)) {
                r.priorClass = prior;
                done(Outcome::SoftApplied, QStringLiteral("Priority lowered; restorable."));
            } else {
                done(Outcome::Failed, QStringLiteral("Could not lower priority: %1").arg(err));
            }
        } else if (d.control == Control::ForceStop) {
            if (ops->forceTerminate(d.verdict.proc.pid, d.verdict.proc.created, &err))
                done(Outcome::ForceTerminated, QStringLiteral("Terminated under your explicit force policy."));
            else
                done(Outcome::Failed, QStringLiteral("Force termination failed: %1").arg(err));
        }
    }
    return out;
}

}
