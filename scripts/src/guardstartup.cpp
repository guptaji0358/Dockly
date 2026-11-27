#include "guardstartup.h"

namespace Guard {

StartupAttr attributeStartup(const StartupEntry &e, const QVector<ProcInspect::CardRef> &cards)
{
    StartupAttr a;
    a.kind = Attribution::Unattributed;
    if (e.source != StartupSource::RunKeyHkcu && e.source != StartupSource::StartupFolder) {
        a.why = QStringLiteral("Unsupported source."); return a;
    }
    if (e.exeHint.isEmpty()) { a.why = QStringLiteral("The program this entry starts cannot be determined reliably."); return a; }
    const QString exe = normPath(e.exeHint);
    QStringList ids, names;
    for (const auto &c : cards) {
        bool hit = !c.exePath.isEmpty() && normPath(c.exePath) == exe;
        if (!hit && !c.installDir.isEmpty()) { // installDir is already empty for shared locations
            const QString dir = normPath(c.installDir) + QLatin1Char('\\');
            hit = exe.startsWith(dir);
        }
        if (hit) { ids << c.id; names << c.name; }
    }
    if (ids.isEmpty()) { a.why = QStringLiteral("Matches no Software card."); return a; }
    if (ids.size() > 1) {
        a.kind = Attribution::Ambiguous;
        a.why = QStringLiteral("Matches several cards (%1): left untouched.").arg(names.join(QStringLiteral(", ")));
        return a;
    }
    a.kind = Attribution::Attributed; a.cardId = ids[0]; a.cardName = names[0];
    a.why = QStringLiteral("Its executable belongs to this card.");
    return a;
}

QVector<StartupRow> StartupManager::rows(const QVector<ProcInspect::CardRef> &cards)
{
    QSet<QString> managed;
    for (const auto &le : m_store->entries({}, QStringLiteral("applied")))
        managed.insert(sourceKey(le.source) + QLatin1Char('|') + le.name);
    QVector<StartupRow> out;
    for (const StartupEntry &e : m_be->list()) {
        StartupRow r;
        r.source = e.source; r.name = e.name; r.location = e.location;
        const StartupAttr a = attributeStartup(e, cards);
        r.cardId = a.cardId; r.cardName = a.cardName; r.why = a.why;
        r.status = a.kind == Attribution::Attributed ? QStringLiteral("available")
                 : a.kind == Attribution::Ambiguous ? QStringLiteral("ambiguous") : QStringLiteral("unattributed");
        out << r; // note: the value is intentionally not copied
    }
    // applied (removed) entries are no longer live; the caller lists them via the ledger
    return out;
}

StartupResult StartupManager::disable(StartupSource s, const QString &name,
                                      const QVector<ProcInspect::CardRef> &cards, bool consent)
{
    StartupResult r;
    if (!consent) { r.message = QStringLiteral("Startup management is not enabled."); return r; }
    if (s != StartupSource::RunKeyHkcu && s != StartupSource::StartupFolder) {
        r.message = planStartup({s, {}, name, true, {}, {}, {}}).reason; return r;
    }
    StartupEntry cur;
    if (!m_be->read(s, name, &cur) || !cur.present) { r.message = QStringLiteral("The entry is not present."); return r; }
    const StartupAttr a = attributeStartup(cur, cards);
    if (a.kind != Attribution::Attributed) { r.message = QStringLiteral("Not modified: ") + a.why; return r; }

    LedgerEntry e;
    e.cardId = a.cardId; e.source = s; e.location = cur.location; e.name = name;
    e.originalValue = cur.value; e.originalHash = hashOf(cur.value); e.appliedRemoved = true;
    const qint64 id = m_store->addPlanned(e, cur.meta);        // 1) record first (protected), state=planned
    if (!id) { r.message = QStringLiteral("Could not record the original safely, nothing was changed: ") + m_store->lastError(); return r; }
    QString err;
    if (!m_be->remove(s, name, &err)) {                          // 2) change
        m_store->setState(id, QStringLiteral("abandoned"), QStringLiteral("remove-failed"), err);
        r.message = QStringLiteral("Could not disable the entry (%1). Nothing was changed.").arg(err); return r;
    }
    StartupEntry after;
    m_be->read(s, name, &after);
    if (after.present) {                                          // 3) verify
        m_store->setState(id, QStringLiteral("abandoned"), QStringLiteral("verify-failed"));
        r.message = QStringLiteral("The entry is still present after removal; not recorded as disabled."); return r;
    }
    m_store->setState(id, QStringLiteral("applied"), QStringLiteral("applied"));
    r.ok = true; r.message = QStringLiteral("Startup entry disabled for \"%1\". You can restore it at any time.").arg(a.cardName);
    return r;
}

StartupResult StartupManager::restore(qint64 id)
{
    StartupResult r;
    LedgerEntry le;
    bool found = false;
    for (const auto &x : m_store->entries()) if (x.id == id) { le = x; found = true; }
    if (!found) { r.message = QStringLiteral("No such recovery entry."); return r; }
    StartupEntry cur;
    if (!m_be->read(le.source, le.name, &cur)) { r.message = QStringLiteral("The current state could not be read."); return r; }
    r.verdict = evaluateRestore(le, cur);
    switch (r.verdict) {
    case RestoreVerdict::AlreadyRestored:
        m_store->setState(id, QStringLiteral("restored"), QStringLiteral("already-restored"));
        r.ok = true; r.message = QStringLiteral("Already in its original state."); return r;
    case RestoreVerdict::SafeToRestore: {
        QString value, meta, err;
        if (!m_store->entryOriginal(id, &value, &meta)) {
            r.message = QStringLiteral("The protected original cannot be read (different Windows user or damaged). Nothing was changed."); return r;
        }
        if (!m_be->write(le.source, le.name, value, meta, &err)) { r.message = QStringLiteral("Restore failed: ") + err; return r; }
        StartupEntry after;
        m_be->read(le.source, le.name, &after);
        if (!after.present || hashOf(after.value) != le.originalHash) {
            m_store->event(id, QStringLiteral("restore-verify-failed"));
            r.message = QStringLiteral("Restore could not be verified."); return r;
        }
        m_store->setState(id, QStringLiteral("restored"), QStringLiteral("restored"));
        r.ok = true; r.message = QStringLiteral("Restored."); return r;
    }
    default:
        r.conflict = r.verdict == RestoreVerdict::ConflictRecreated || r.verdict == RestoreVerdict::ConflictModified
                  || r.verdict == RestoreVerdict::ConflictMissing;
        if (r.conflict) m_store->event(id, QStringLiteral("conflict"), verdictLabel(r.verdict));
        r.message = QStringLiteral("Not restored: %1. Your newer state was kept.").arg(verdictLabel(r.verdict));
        return r;
    }
}

QVector<StartupResult> StartupManager::restoreAll()
{
    QVector<StartupResult> out;
    for (const auto &e : m_store->entries({}, QStringLiteral("applied")))
        out << restore(e.id);
    return out;
}

QStringList StartupManager::recover()
{
    QStringList log;
    for (const auto &le : m_store->entries({}, QStringLiteral("planned"))) {
        StartupEntry cur;
        if (!m_be->read(le.source, le.name, &cur)) { log << QStringLiteral("%1: state unreadable, left planned").arg(le.name); continue; }
        if (!cur.present) { // we (probably) removed it before the interruption; the protected original is safe
            m_store->setState(le.id, QStringLiteral("applied"), QStringLiteral("recovered-applied"));
            log << QStringLiteral("%1: interrupted disable completed in the ledger").arg(le.name);
        } else {
            m_store->setState(le.id, QStringLiteral("abandoned"), QStringLiteral("recovered-abandoned"));
            log << QStringLiteral("%1: entry still present, interrupted disable abandoned").arg(le.name);
        }
    }
    return log;
}

QStringList StartupManager::checkConsistency()
{
    QStringList problems;
    for (const auto &e : m_store->entries({}, QStringLiteral("applied"))) {
        if (!m_store->entryOriginal(e.id, nullptr, nullptr))
            problems << QStringLiteral("Entry \"%1\": protected original unreadable or does not match its hash").arg(e.name);
        StartupEntry cur;
        if (m_be->read(e.source, e.name, &cur) && cur.present && hashOf(cur.value) != e.originalHash)
            problems << QStringLiteral("Entry \"%1\": changed since Dockly disabled it (restore would be a conflict)").arg(e.name);
    }
    return problems;
}

}
