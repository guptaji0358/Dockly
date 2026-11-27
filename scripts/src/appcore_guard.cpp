#include "appcore.h"

#include <QDir>
#include <QtConcurrent>

using namespace Guard;

// Background Process Guard glue (Stages 3.2-3.6). Everything is user-triggered; there is no watchdog,
// service or elevated helper. Command lines are never read. Startup values never leave GuardStore/Startup*.

bool AppCore::ensureGuardStore()
{
    if (m_gstore.isOpen())
        return true;
    QString err;
    if (!m_gstore.open(QDir(m_dataDir).filePath(QStringLiteral("guard.db")), dpapiProtector(), &err)) {
        emit message(QStringLiteral("Guard data could not be opened: %1").arg(err));
        return false;
    }
    if (!m_sbe)
        m_sbe = makeSystemBackend();
    return true;
}

QVector<ProcInspect::CardRef> AppCore::softwareCardRefs() const
{
    QVector<ProcInspect::CardRef> cards;
    for (const QString &cid : queryIds(LH::ModeCategory, LH::Software, {}, {})) {
        const Item it = item(cid);
        cards << ProcInspect::makeCardRef(it.id, it.name, it.target);
    }
    return cards;
}

Policy AppCore::buildGuardPolicy() const
{
    Policy po;
    po.masterOn = m_settings->boolean("guardEnabled");
    po.currentSessionId = currentSessionId();
    for (const QString &cid : queryIds(LH::ModeCategory, LH::Software, {}, {})) {
        CardPolicy cp;
        cp.cardId = cid;
        cp.guardOn = item(cid).guard;
        cp.allowedExePaths = m_gstore.allowed(cid, QStringLiteral("closed"));
        po.cards.insert(cid, cp);
    }
    return po;
}

QHash<QString, ClosedPolicy> AppCore::buildClosedPolicies() const
{
    QHash<QString, ClosedPolicy> out;
    for (const QString &cid : m_gstore.cardsWithPolicy()) {
        const auto s = m_gstore.cardSettings(cid);
        out.insert(cid, ClosedPolicy{s.graceMs, s.allowForce});
    }
    return out;
}

QHash<QString, OpenPolicy> AppCore::buildOpenPolicies() const
{
    QHash<QString, OpenPolicy> out;
    for (const QString &cid : m_gstore.cardsWithPolicy()) {
        const auto s = m_gstore.cardSettings(cid);
        OpenPolicy op;
        op.cardId = cid; op.enabled = s.openEnabled; op.forceAcknowledged = s.forceAck;
        for (const QString &e : m_gstore.allowed(cid, QStringLiteral("open-lower"))) op.selected.insert(e, Control::LowerPriority);
        for (const QString &e : m_gstore.allowed(cid, QStringLiteral("open-force"))) op.selected.insert(e, Control::ForceStop);
        out.insert(cid, op);
    }
    return out;
}

QVariantList AppCore::annotateRows(const QString &cardId, const QVariantList &rows)
{
    if (!ensureGuardStore())
        return rows;
    QVector<ProcInspect::ProcessRecord> procs;
    for (const auto &v : m_lastInsp.verdicts) procs << v.proc;
    const auto facts = collectFacts(procs);
    const Policy po = buildGuardPolicy();
    const DryRunReport rep = dryRun(m_lastInsp, facts, po, nowFiletime());
    const auto plan = planOpen(m_lastInsp, facts, po, buildOpenPolicies());
    QHash<quint32, const Decision *> dec;
    for (const auto &d : rep.decisions) dec.insert(d.verdict.proc.pid, &d);
    QHash<quint32, const OpenDecision *> op;
    for (const auto &d : plan) op.insert(d.verdict.proc.pid, &d);
    const bool closed = closedCardsOf(m_lastInsp, {cardId}).contains(cardId);
    QVariantList out;
    for (QVariant rv : rows) {
        QVariantMap r = rv.toMap();
        const quint32 pid = r.value("pid").toUInt();
        if (r.value("related").toBool() && op.contains(pid)) {
            const OpenDecision *o = op.value(pid);
            r["role"] = o->role == Role::Protected ? "protected" : o->role == Role::Eligible ? "eligible" : "observed";
            r["roleLabel"] = roleLabel(o->role);
            r["roleWhy"] = o->why;
            r["control"] = o->control == Control::LowerPriority ? "lower" : o->control == Control::ForceStop ? "force" : "none";
        }
        if (dec.contains(pid)) {
            const Decision *d = dec.value(pid);
            // a proposal, never a claim: nothing was executed to produce this text
            r["proposal"] = closed ? actionLabel(d->action) + ": " + d->explanation
                                   : QStringLiteral("Closed-app policy does not apply while the program is open.");
            r["wouldStop"] = closed && d->action == Action::WouldStop;
            r["canAllow"] = d->verdict.cls == ProcInspect::Class::CandidateBackground && d->verdict.ambiguousCards.isEmpty()
                            && d->verdict.cardId == cardId && !d->verdict.proc.exePath.isEmpty();
        }
        r["allowedClosed"] = m_gstore.allowed(cardId, "closed").contains(normPath(r.value("exePath").toString()));
        r["allowedLower"] = m_gstore.allowed(cardId, "open-lower").contains(normPath(r.value("exePath").toString()));
        r["allowedForce"] = m_gstore.allowed(cardId, "open-force").contains(normPath(r.value("exePath").toString()));
        out << r;
    }
    return out;
}

QVariantMap AppCore::guardInfo(const QString &id)
{
    QVariantMap m;
    if (m_closed || !ensureGuardStore())
        return {{"available", false}};
    const auto s = m_gstore.cardSettings(id);
    m = {{"available", true}, {"master", m_settings->boolean("guardEnabled")}, {"cardOn", item(id).guard},
         {"graceMs", s.graceMs}, {"allowForce", s.allowForce}, {"openEnabled", s.openEnabled}, {"forceAck", s.forceAck},
         {"allowedClosed", int(m_gstore.allowed(id, "closed").size())},
         {"recent", m_gstore.recentActions(20)}, {"pendingSoft", int(m_gstore.pendingSoft().size())},
         {"startupConsent", m_settings->boolean("startupManagement")}};
    return m;
}

void AppCore::guardSetAllowed(const QString &id, const QString &exePath, const QString &mode, bool on)
{
    if (m_closed || exePath.isEmpty() || item(id).category != LH::Software || !ensureGuardStore())
        return;
    if (mode != QLatin1String("closed") && mode != QLatin1String("open-lower") && mode != QLatin1String("open-force"))
        return;
    m_gstore.setAllowed(id, exePath, mode, on);
}

void AppCore::guardSetClosedPolicy(const QString &id, int graceMs, bool allowForce)
{
    if (m_closed || !ensureGuardStore())
        return;
    auto s = m_gstore.cardSettings(id);
    s.graceMs = qBound(1000, graceMs, 60000); s.allowForce = allowForce;
    m_gstore.setCardSettings(id, s);
}

void AppCore::guardSetOpenPolicy(const QString &id, bool enabled, bool forceAcknowledged)
{
    if (m_closed || !ensureGuardStore())
        return;
    auto s = m_gstore.cardSettings(id);
    s.openEnabled = enabled; s.forceAck = forceAcknowledged;
    m_gstore.setCardSettings(id, s);
}

namespace {
QVariantList recordsToList(const QVector<ActionRecord> &rs)
{
    QVariantList l;
    for (const auto &r : rs)
        l << QVariantMap{{"cardId", r.cardId}, {"process", r.processName}, {"pid", r.pid},
                         {"outcome", outcomeLabel(r.outcome)}, {"acted", r.acted}, {"explanation", r.explanation}};
    return l;
}
}

void AppCore::finishGuardRun(const QVector<ActionRecord> &rs)
{
    QVector<ActionRecord> done = rs;
    for (auto &r : done) {
        m_gstore.logAction(r);
        if (r.outcome == Outcome::SoftApplied)
            m_gstore.addSoft(r);
    }
    emit guardActions(m_guardRunId, recordsToList(done));
}

void AppCore::guardRunClosed(const QString &id)
{
    if (m_closed || m_guardWatcher.isRunning() || item(id).category != LH::Software || !ensureGuardStore())
        return;
    auto refuse = [&](const QString &why) { emit guardActions(id, {QVariantMap{{"outcome", "Not run"}, {"acted", false}, {"explanation", why}}}); };
    if (!m_settings->boolean("guardEnabled")) { refuse(QStringLiteral("The master Background Process Guard switch is off. Nothing was done.")); return; }
    if (!item(id).guard) { refuse(QStringLiteral("This program's guard switch is off. Nothing was done.")); return; }
    if (m_lastInspId != id || QDateTime::currentMSecsSinceEpoch() - m_lastInspMs > 60000) {
        refuse(QStringLiteral("Inspect the program first (the inspection is older than a minute). Nothing was done.")); return;
    }
    QVector<ProcInspect::ProcessRecord> procs;
    for (const auto &v : m_lastInsp.verdicts) procs << v.proc;
    const DryRunReport rep = dryRun(m_lastInsp, collectFacts(procs), buildGuardPolicy(), nowFiletime());
    const QSet<QString> closed = closedCardsOf(m_lastInsp, {id});
    const auto pol = buildClosedPolicies();
    m_guardRunId = id;
    m_guardWatcher.setFuture(QtConcurrent::run([rep, closed, pol] { return enforceClosed(rep, closed, pol, systemOps()); }));
}

void AppCore::guardRunOpen(const QString &id)
{
    if (m_closed || m_guardWatcher.isRunning() || item(id).category != LH::Software || !ensureGuardStore())
        return;
    auto refuse = [&](const QString &why) { emit guardActions(id, {QVariantMap{{"outcome", "Not run"}, {"acted", false}, {"explanation", why}}}); };
    if (!m_settings->boolean("guardEnabled") || !item(id).guard) { refuse(QStringLiteral("The master or this card's guard switch is off. Nothing was done.")); return; }
    if (m_lastInspId != id || QDateTime::currentMSecsSinceEpoch() - m_lastInspMs > 60000) {
        refuse(QStringLiteral("Inspect the program first (the inspection is older than a minute). Nothing was done.")); return;
    }
    QVector<ProcInspect::ProcessRecord> procs;
    for (const auto &v : m_lastInsp.verdicts) procs << v.proc;
    auto plan = planOpen(m_lastInsp, collectFacts(procs), buildGuardPolicy(), buildOpenPolicies());
    QVector<OpenDecision> mine;
    for (const auto &d : plan) if (d.verdict.cardId == id) mine << d;
    m_guardRunId = id;
    m_guardWatcher.setFuture(QtConcurrent::run([mine] { return applyOpen(mine, systemOps()); }));
}

QStringList AppCore::guardRecover()
{
    QStringList log;
    if (m_closed || !ensureGuardStore())
        return log;
    StartupManager m(&m_gstore, m_sbe.get());
    log << m.recover();
    revertPendingSoft(m_gstore, systemOps(), &log);
    return log;
}

QVariantList AppCore::startupRows()
{
    QVariantList out;
    if (m_closed || !ensureGuardStore() || !m_sbe)
        return out;
    StartupManager m(&m_gstore, m_sbe.get());
    for (const auto &r : m.rows(softwareCardRefs()))
        out << QVariantMap{{"source", int(r.source)}, {"sourceLabel", sourceKey(r.source)}, {"name", r.name},
                           {"location", r.location}, {"cardId", r.cardId}, {"cardName", r.cardName},
                           {"status", r.status}, {"why", r.why}};
    return out;
}

static QVariantMap resultMap(const StartupResult &r)
{
    return {{"ok", r.ok}, {"message", r.message}, {"conflict", r.conflict}, {"verdict", verdictLabel(r.verdict)}};
}

QVariantMap AppCore::startupDisable(int source, const QString &name)
{
    if (m_closed || !ensureGuardStore() || !m_sbe)
        return {{"ok", false}, {"message", "Guard data is unavailable."}};
    // the consent is the user's separate opt-in setting; turning the guard on never sets it
    const bool consent = m_settings->boolean("startupManagement");
    StartupManager m(&m_gstore, m_sbe.get());
    return resultMap(m.disable(StartupSource(source), name, softwareCardRefs(), consent));
}

QVariantMap AppCore::startupRestore(double entryId)
{
    if (m_closed || !ensureGuardStore() || !m_sbe)
        return {{"ok", false}, {"message", "Guard data is unavailable."}};
    StartupManager m(&m_gstore, m_sbe.get());
    return resultMap(m.restore(qint64(entryId)));
}

QVariantList AppCore::startupRestoreAll()
{
    QVariantList out;
    if (m_closed || !ensureGuardStore() || !m_sbe)
        return out;
    StartupManager m(&m_gstore, m_sbe.get());
    for (const auto &r : m.restoreAll()) out << resultMap(r);
    return out;
}

QVariantList AppCore::startupLedger()
{
    QVariantList out;
    if (m_closed || !ensureGuardStore())
        return out;
    for (const auto &e : m_gstore.entries())
        out << QVariantMap{{"id", double(e.id)}, {"cardId", e.cardId}, {"name", e.name}, {"source", sourceKey(e.source)},
                           {"state", e.state}, {"card", item(e.cardId).name}}; // never the value
    return out;
}

QStringList AppCore::startupCheck()
{
    if (m_closed || !ensureGuardStore() || !m_sbe)
        return {};
    StartupManager m(&m_gstore, m_sbe.get());
    return m.checkConsistency();
}
