#include "guardengine.h"

#include <QDir>

namespace Guard {

QString actionLabel(Action a)
{
    switch (a) {
    case Action::Keep: return QStringLiteral("Keep");
    case Action::Observe: return QStringLiteral("Observe");
    case Action::WouldStop: return QStringLiteral("Would stop (dry-run)");
    }
    return {};
}

QString normPath(const QString &p)
{
    return QDir::toNativeSeparators(QDir::cleanPath(p)).toLower();
}

DryRunReport dryRun(const ProcInspect::Inspection &insp, const QHash<quint32, ProcessFacts> &facts,
                    const Policy &policy, quint64 now)
{
    using ProcInspect::Class;
    DryRunReport rep;
    for (const auto &v : insp.verdicts) {
        Decision d;
        d.verdict = v;
        d.facts = facts.value(v.proc.pid);
        const bool haveFacts = facts.contains(v.proc.pid);

        // Anything that is not a clean, attributed Candidate stays protected.
        if (v.cls != Class::CandidateBackground) {
            d.action = Action::Keep;
            switch (v.cls) {
            case Class::Main: d.explanation = QStringLiteral("Main program of its card: protected."); break;
            case Class::Essential: d.explanation = QStringLiteral("Essential ancestor of the card's Main: protected."); break;
            default:
                d.explanation = !v.ambiguousCards.isEmpty()
                    ? QStringLiteral("Ambiguous/shared ownership: protected.")
                    : QStringLiteral("Unknown or unattributed: untouched. %1").arg(v.reason);
            }
            if (!haveFacts || d.facts.queryDenied)
                d.explanation += QStringLiteral(" (not inspectable)");
            ++rep.keep;
            rep.decisions << d;
            continue;
        }

        // Candidate: every gate must pass. Failures are recorded, never skipped.
        auto block = [&](const QString &s) { d.blockers << s; };
        if (!policy.masterOn) block(QStringLiteral("Master switch is off"));
        const auto cp = policy.cards.constFind(v.cardId);
        if (cp == policy.cards.constEnd()) {
            block(QStringLiteral("No explicit policy for this card"));
        } else {
            if (!cp->guardOn) block(QStringLiteral("Card guard switch is off"));
            if (v.proc.exePath.isEmpty() || !cp->allowedExePaths.contains(normPath(v.proc.exePath)))
                block(QStringLiteral("Executable is not on the card's explicit allow-list (Candidate is not authorisation)"));
        }
        if (v.proc.created == 0) block(QStringLiteral("Process identity unverifiable (no creation time)"));
        if (!haveFacts) block(QStringLiteral("No access facts collected"));
        else {
            if (d.facts.queryDenied) block(QStringLiteral("Process cannot be queried"));
            if (d.facts.sessionId < 0) block(QStringLiteral("Session unknown"));
            else if (d.facts.sessionId == 0) block(QStringLiteral("Session 0 (services): never eligible"));
            else if (policy.currentSessionId >= 0 && d.facts.sessionId != policy.currentSessionId)
                block(QStringLiteral("Belongs to another session"));
            if (d.facts.terminateAccess == Tri::No) block(QStringLiteral("Windows denies PROCESS_TERMINATE to this user"));
            else if (d.facts.terminateAccess == Tri::Unknown) block(QStringLiteral("Terminate access not verified"));
        }
        if (cp != policy.cards.constEnd() && v.proc.created && now >= v.proc.created) {
            const qint64 ageS = qint64((now - v.proc.created) / 10000000ULL);
            if (ageS < cp->minAgeSeconds)
                block(QStringLiteral("Younger than the %1 s grace period").arg(cp->minAgeSeconds));
        }

        if (d.blockers.isEmpty()) {
            d.action = Action::WouldStop;
            d.explanation = QStringLiteral("DRY-RUN: all gates pass; would propose stopping it. Nothing is executed.");
            ++rep.wouldStop;
        } else {
            d.action = Action::Observe;
            d.explanation = QStringLiteral("Candidate-background but not actionable: %1").arg(d.blockers.join(QStringLiteral("; ")));
            ++rep.observe;
        }
        rep.decisions << d;
    }
    return rep;
}

PollDiff diffSnapshots(const QVector<ProcInspect::ProcessRecord> &before,
                       const QVector<ProcInspect::ProcessRecord> &after)
{
    PollDiff out;
    auto key = [](const ProcInspect::ProcessRecord &r) {
        return QString::number(r.pid) + QLatin1Char('-') + QString::number(r.created);
    };
    QSet<QString> b, a;
    for (const auto &r : before) b.insert(key(r));
    for (const auto &r : after) a.insert(key(r));
    for (const auto &r : after) {
        if (r.created == 0) { ++out.uncertain; continue; }
        if (!b.contains(key(r))) out.started << r;
    }
    for (const auto &r : before)
        if (r.created != 0 && !a.contains(key(r))) out.exited << r; // PID reuse shows as exit + start
    return out;
}

}
