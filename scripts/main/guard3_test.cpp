#include "guardstartup.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <windows.h>

using namespace Guard;
using ProcInspect::CardRef;
using ProcInspect::ProcessRecord;

namespace {
const quint64 kSec = 10000000ULL;
const quint64 kNow = 1000000ULL * kSec;
const QString kMain = "C:\\Apps\\Foo\\foo.exe";
const QString kHelper = "C:\\Apps\\Foo\\foohelper.exe";

ProcessRecord rec(quint32 pid, const QString &exe, bool win = false)
{
    ProcessRecord r; r.pid = pid; r.ppid = 1; r.exePath = exe; r.name = exe.section('\\', -1);
    r.created = kNow - 900 * kSec; r.hasWindow = win;
    return r;
}
CardRef card(const QString &id, const QString &exe, const QString &dir)
{
    CardRef c; c.id = id; c.name = id; c.exePath = exe; c.installDir = dir; c.publisher = "Pub";
    return c;
}
ProcessFacts okFacts(quint32 pid)
{
    ProcessFacts f; f.pid = pid; f.sessionId = 1; f.terminateAccess = Tri::Yes; return f;
}

// Fake OS: nothing real is ever touched.
class FakeOps : public ProcessOps
{
public:
    QHash<quint32, LiveIdentity> live;
    int windows = 1;            // windows requestClose reports
    int exitAfterSleeps = -1;   // -1 = never exits on its own after WM_CLOSE
    bool closeAsked = false;
    int forced = 0, sleeps = 0, priorityCalls = 0;
    bool forceOk = true;
    LiveIdentity identity(quint32 pid) override { return live.value(pid); }
    int requestClose(quint32 pid, quint64) override { closeAsked = true; return windows; }
    bool forceTerminate(quint32 pid, quint64, QString *) override
    {
        ++forced; live[pid].alive = false; return forceOk;
    }
    bool setPriority(quint32, quint64, int, int *prior, QString *) override { ++priorityCalls; *prior = 0x20; return true; }
    void sleepMs(int) override
    {
        ++sleeps;
        if (closeAsked && exitAfterSleeps >= 0 && sleeps >= exitAfterSleeps)
            for (auto it = live.begin(); it != live.end(); ++it) it->alive = false;
    }
};

class XorProt : public ValueProtector
{
public:
    explicit XorProt(char k) : key(k) {}
    char key;
    QByteArray protect(const QByteArray &p, bool *ok) override
    {
        QByteArray o("P"); o.append(key);
        for (char c : p) o.append(char(c ^ key));
        *ok = true; return o;
    }
    QByteArray unprotect(const QByteArray &b, bool *ok) override
    {
        if (b.size() < 2 || b[0] != 'P' || b[1] != key) { *ok = false; return {}; }
        QByteArray o;
        for (int i = 2; i < b.size(); ++i) o.append(char(b[i] ^ key));
        *ok = true; return o;
    }
};

class FakeBackend : public StartupBackend
{
public:
    QMap<QString, StartupEntry> m; // key = name (Run values only)
    bool failRemove = false;
    void put(const QString &name, const QString &value)
    {
        StartupEntry e; e.source = StartupSource::RunKeyHkcu; e.location = "HKCU\\Run"; e.name = name;
        e.value = value; e.meta = "type=1"; e.exeHint = exeOfValue(value); e.present = true; m[name] = e;
    }
    QVector<StartupEntry> list() override { return m.values().toVector(); }
    bool read(StartupSource s, const QString &n, StartupEntry *out) override
    {
        *out = m.value(n); out->source = s; out->name = n; out->present = m.contains(n); return true;
    }
    bool remove(StartupSource, const QString &n, QString *err) override
    {
        if (failRemove) { *err = "denied"; return false; }
        m.remove(n); return true;
    }
    bool write(StartupSource, const QString &n, const QString &v, const QString &, QString *) override
    {
        put(n, v); return true;
    }
};
}

struct Env {
    QTemporaryDir dir; GuardStore store; XorProt prot{'k'}; FakeBackend be; QVector<CardRef> cards;
    QString db() const { return dir.path() + "/guard.db"; }
    Env() { cards << card("foo", kMain, "C:\\Apps\\Foo") << card("a", "C:\\S\\t.exe", "") << card("b", "C:\\S\\t.exe", ""); }
};

class Guard3Test : public QObject
{
    Q_OBJECT

    ProcInspect::Inspection scene(bool mainRunning = false, bool helperWindow = false)
    {
        QVector<ProcessRecord> p{rec(20, kHelper, helperWindow)};
        if (mainRunning) p.prepend(rec(10, kMain, true));
        return ProcInspect::attribute(p, {card("foo", kMain, "C:\\Apps\\Foo")},
                                      [](const QString &) { return QStringLiteral("Pub"); });
    }
    Policy policy()
    {
        Policy po; po.currentSessionId = 1;
        CardPolicy cp; cp.cardId = "foo"; cp.allowedExePaths.insert(normPath(kHelper));
        po.cards.insert("foo", cp);
        return po;
    }
    static FakeOps liveHelper()
    {
        FakeOps o;
        LiveIdentity id; id.alive = true; id.created = kNow - 900 * kSec; id.exePath = kHelper;
        o.live.insert(20, id);
        return o;
    }
    DryRunReport report(bool mainRunning = false)
    {
        return dryRun(scene(mainRunning), {{10, okFacts(10)}, {20, okFacts(20)}}, policy(), kNow);
    }
    QSet<QString> closed(bool mainRunning = false)
    {
        return closedCardsOf(scene(mainRunning), {"foo"});
    }

private slots:
    // ---- Stage 3.2 ----
    void gracefulCloseNeverForces()
    {
        auto ops = liveHelper(); ops.exitAfterSleeps = 2;
        QHash<QString, ClosedPolicy> pc{{"foo", {3000, false}}};
        const auto r = enforceClosed(report(), closed(), pc, &ops);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].outcome, Outcome::ClosedGracefully);
        QVERIFY(r[0].acted);
        QCOMPARE(ops.forced, 0);
    }
    void noSilentFallbackToForce()
    {
        auto ops = liveHelper(); // ignores WM_CLOSE
        QHash<QString, ClosedPolicy> pc{{"foo", {500, false}}};
        auto r = enforceClosed(report(), closed(), pc, &ops);
        QCOMPARE(r[0].outcome, Outcome::StillRunning);
        QVERIFY(!r[0].acted);
        QCOMPARE(ops.forced, 0);
        QVERIFY(ops.live[20].alive);
        auto ops2 = liveHelper(); ops2.windows = 0; // nothing to ask
        r = enforceClosed(report(), closed(), pc, &ops2);
        QCOMPARE(r[0].outcome, Outcome::NoGracefulPath);
        QCOMPARE(ops2.forced, 0);
    }
    void forceOnlyWithExplicitPolicyAfterGrace()
    {
        auto ops = liveHelper();
        QHash<QString, ClosedPolicy> pc{{"foo", {300, true}}};
        const auto r = enforceClosed(report(), closed(), pc, &ops);
        QCOMPARE(r[0].outcome, Outcome::ForceTerminated);
        QVERIFY(ops.closeAsked);
        QCOMPARE(ops.forced, 1);
        QVERIFY(ops.sleeps >= 3); // the grace period was honoured
    }
    void identityChangeFailsSafe()
    {
        QHash<QString, ClosedPolicy> pc{{"foo", {300, true}}};
        auto reused = liveHelper(); reused.live[20].created += 5 * kSec; // PID reused
        auto r = enforceClosed(report(), closed(), pc, &reused);
        QCOMPARE(r[0].outcome, Outcome::IdentityChanged);
        QVERIFY(!reused.closeAsked); QCOMPARE(reused.forced, 0);
        auto other = liveHelper(); other.live[20].exePath = "C:\\Other\\x.exe";
        r = enforceClosed(report(), closed(), pc, &other);
        QCOMPARE(r[0].outcome, Outcome::IdentityChanged);
        QVERIFY(!other.closeAsked);
        FakeOps gone; // process vanished
        r = enforceClosed(report(), closed(), pc, &gone);
        QCOMPARE(r[0].outcome, Outcome::AlreadyGone);
        auto unreadable = liveHelper(); unreadable.live[20].exePath.clear();
        r = enforceClosed(report(), closed(), pc, &unreadable);
        QCOMPARE(r[0].outcome, Outcome::IdentityUnavailable);
    }
    void openProgramAndDisabledSwitchesAreNeverActedOn()
    {
        QHash<QString, ClosedPolicy> pc{{"foo", {300, true}}};
        auto ops = liveHelper();
        auto r = enforceClosed(report(true), closed(true), pc, &ops); // main is running
        for (const auto &a : r) QVERIFY(!a.acted);
        QVERIFY(!ops.closeAsked); QCOMPARE(ops.forced, 0);
        Policy off = policy(); off.cards["foo"].guardOn = false;
        auto rep = dryRun(scene(), {{20, okFacts(20)}}, off, kNow);
        QCOMPARE(enforceClosed(rep, closed(), pc, &ops).size(), 0);
        off = policy(); off.masterOn = false;
        rep = dryRun(scene(), {{20, okFacts(20)}}, off, kNow);
        QCOMPARE(enforceClosed(rep, closed(), pc, &ops).size(), 0);
        QVERIFY(!ops.closeAsked);
    }
    void unknownAndSharedProcessesAreNotTouched()
    {
        auto insp = ProcInspect::attribute({rec(50, "C:\\Other\\random.exe"), rec(20, kHelper)},
                                           {card("foo", kMain, "C:\\Apps\\Foo")});
        auto rep = dryRun(insp, {{50, okFacts(50)}, {20, okFacts(20)}}, policy(), kNow);
        FakeOps ops; LiveIdentity id; id.alive = true; id.created = kNow - 900 * kSec; id.exePath = "C:\\Other\\random.exe";
        ops.live.insert(50, id);
        QHash<QString, ClosedPolicy> pc{{"foo", {200, true}}};
        for (const auto &a : enforceClosed(rep, closedCardsOf(insp, {"foo"}), pc, &ops)) QVERIFY(a.pid != 50);
        QVERIFY(ops.live[50].alive);
    }

    // ---- Stage 3.4 ----
    void openAppDefaultsToObservation()
    {
        auto insp = scene(true);
        QHash<quint32, ProcessFacts> f{{10, okFacts(10)}, {20, okFacts(20)}};
        auto plan = planOpen(insp, f, policy(), {});
        for (const auto &d : plan) {
            QVERIFY(d.role != Role::Eligible);
            QVERIFY(!d.why.isEmpty());
        }
        QCOMPARE(plan[0].role, Role::Protected); // main
        QCOMPARE(plan[1].role, Role::Observed);  // unselected helper
        FakeOps ops = liveHelper();
        QCOMPARE(applyOpen(plan, &ops).size(), 0);
        QCOMPARE(ops.priorityCalls, 0);
    }
    void openAppOnlyUserSelectedAndReversible()
    {
        auto insp = scene(true);
        QHash<quint32, ProcessFacts> f{{10, okFacts(10)}, {20, okFacts(20)}};
        OpenPolicy op; op.cardId = "foo"; op.enabled = true;
        op.selected.insert(normPath(kHelper), Control::LowerPriority);
        auto plan = planOpen(insp, f, policy(), {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Eligible);
        FakeOps ops = liveHelper();
        auto r = applyOpen(plan, &ops);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].outcome, Outcome::SoftApplied);
        QCOMPARE(r[0].priorClass, 0x20);
        QCOMPARE(ops.forced, 0);
        FakeOps reused = liveHelper(); reused.live[20].created += kSec;
        QCOMPARE(applyOpen(plan, &reused)[0].outcome, Outcome::IdentityChanged);
        QCOMPARE(reused.priorityCalls, 0);
    }
    void forceStopNeedsAcknowledgementAndWindowedHelpersStayProtected()
    {
        QHash<quint32, ProcessFacts> f{{10, okFacts(10)}, {20, okFacts(20)}};
        OpenPolicy op; op.cardId = "foo"; op.enabled = true;
        op.selected.insert(normPath(kHelper), Control::ForceStop);
        auto plan = planOpen(scene(true), f, policy(), {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Observed);
        op.forceAcknowledged = true;
        plan = planOpen(scene(true), f, policy(), {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Eligible);
        plan = planOpen(scene(true, true), f, policy(), {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Protected);
        Policy off = policy(); off.cards["foo"].guardOn = false;
        plan = planOpen(scene(true), f, off, {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Observed);
    }

    // ---- Hardening: distinct identity outcomes ----
    void inaccessibleProcessIsNotReportedAsExited()
    {
        QHash<QString, ClosedPolicy> pc{{"foo", {300, true}}};
        // 1. exited
        FakeOps gone;
        auto r = enforceClosed(report(), closed(), pc, &gone);
        QCOMPARE(r[0].outcome, Outcome::AlreadyGone);
        // 2. access denied
        auto denied = liveHelper(); denied.live[20] = LiveIdentity{}; denied.live[20].accessDenied = true;
        r = enforceClosed(report(), closed(), pc, &denied);
        QCOMPARE(r[0].outcome, Outcome::AccessDenied);
        QVERIFY(!r[0].acted);
        QVERIFY(outcomeLabel(r[0].outcome).startsWith("NOT DONE"));
        QVERIFY(!r[0].explanation.contains("exited"));
        QVERIFY(!denied.closeAsked); QCOMPARE(denied.forced, 0);
        // 3. identity unavailable (alive, unreadable creation time / path / query failure)
        auto noTime = liveHelper(); noTime.live[20].created = 0;
        r = enforceClosed(report(), closed(), pc, &noTime);
        QCOMPARE(r[0].outcome, Outcome::IdentityUnavailable);
        QVERIFY(!noTime.closeAsked); QCOMPARE(noTime.forced, 0);
        auto noPath = liveHelper(); noPath.live[20].exePath.clear();
        QCOMPARE(enforceClosed(report(), closed(), pc, &noPath)[0].outcome, Outcome::IdentityUnavailable);
        auto unq = liveHelper(); unq.live[20] = LiveIdentity{}; unq.live[20].unreadable = true;
        r = enforceClosed(report(), closed(), pc, &unq);
        QCOMPARE(r[0].outcome, Outcome::IdentityUnavailable);
        QVERIFY(!unq.closeAsked); QCOMPARE(unq.forced, 0);
        // 4. mismatch
        auto reused = liveHelper(); reused.live[20].created += kSec;
        QCOMPARE(enforceClosed(report(), closed(), pc, &reused)[0].outcome, Outcome::IdentityChanged);
        auto other = liveHelper(); other.live[20].exePath = "C:\\Other\\x.exe";
        QCOMPARE(enforceClosed(report(), closed(), pc, &other)[0].outcome, Outcome::IdentityChanged);
        QCOMPARE(other.forced, 0);
    }
    void openAppOutcomesAreDistinctAndUntouched()
    {
        QHash<quint32, ProcessFacts> f{{10, okFacts(10)}, {20, okFacts(20)}};
        OpenPolicy op; op.cardId = "foo"; op.enabled = true;
        op.selected.insert(normPath(kHelper), Control::LowerPriority);
        const auto plan = planOpen(scene(true), f, policy(), {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Eligible);
        FakeOps gone;
        QCOMPARE(applyOpen(plan, &gone)[0].outcome, Outcome::AlreadyGone);
        auto denied = liveHelper(); denied.live[20] = LiveIdentity{}; denied.live[20].accessDenied = true;
        QCOMPARE(applyOpen(plan, &denied)[0].outcome, Outcome::AccessDenied);
        auto none = liveHelper(); none.live[20].created = 0;
        QCOMPARE(applyOpen(plan, &none)[0].outcome, Outcome::IdentityUnavailable);
        auto reused = liveHelper(); reused.live[20].created += kSec;
        QCOMPARE(applyOpen(plan, &reused)[0].outcome, Outcome::IdentityChanged);
        QCOMPARE(denied.priorityCalls + none.priorityCalls + reused.priorityCalls + gone.priorityCalls, 0);
    }
    void gracefulWaitDoesNotMistakeDeniedAccessForExit()
    {
        QHash<QString, ClosedPolicy> pc{{"foo", {1000, true}}};
        struct Flaky : FakeOps {
            LiveIdentity identity(quint32 pid) override
            {
                if (closeAsked) { LiveIdentity d; d.accessDenied = true; return d; }
                return FakeOps::identity(pid);
            }
        } ops;
        ops.live = liveHelper().live;
        const auto r = enforceClosed(report(), closed(), pc, &ops);
        QCOMPARE(r[0].outcome, Outcome::IdentityUnavailable);
        QVERIFY(!r[0].acted);
        QCOMPARE(ops.forced, 0);
    }

    // ---- Hardening: force-stop acknowledgement is validated by the executor itself ----
    void applyOpenValidatesForceAckIndependently()
    {
        QHash<quint32, ProcessFacts> f{{10, okFacts(10)}, {20, okFacts(20)}};
        OpenPolicy op; op.cardId = "foo"; op.enabled = true;
        op.selected.insert(normPath(kHelper), Control::ForceStop);
        // plan -> execution, acknowledged: allowed
        op.forceAcknowledged = true;
        auto plan = planOpen(scene(true), f, policy(), {{"foo", op}});
        QCOMPARE(plan[1].role, Role::Eligible);
        QVERIFY(plan[1].forceAcknowledged);
        auto ops = liveHelper();
        auto r = applyOpen(plan, &ops);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].outcome, Outcome::ForceTerminated);
        QCOMPARE(ops.forced, 1);
        // plan -> execution, not acknowledged: never Eligible, nothing runs
        op.forceAcknowledged = false;
        plan = planOpen(scene(true), f, policy(), {{"foo", op}});
        auto ops2 = liveHelper();
        QCOMPARE(applyOpen(plan, &ops2).size(), 0);
        QCOMPARE(ops2.forced, 0);
        QVERIFY(ops2.live[20].alive);
        // direct call: an Eligible/ForceStop decision forged without acknowledgement is rejected
        auto forged = planOpen(scene(true), f, policy(), {{"foo", [&] { auto o = op; o.forceAcknowledged = true; return o; }()}});
        forged[1].forceAcknowledged = false;
        auto ops3 = liveHelper();
        r = applyOpen(forged, &ops3);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].outcome, Outcome::Skipped);
        QVERIFY(!r[0].acted);
        QCOMPARE(ops3.forced, 0);
        QVERIFY(ops3.live[20].alive);
        // acknowledged but identity fails: still untouched
        forged[1].forceAcknowledged = true;
        auto ops4 = liveHelper(); ops4.live[20].created += kSec;
        QCOMPARE(applyOpen(forged, &ops4)[0].outcome, Outcome::IdentityChanged);
        QCOMPARE(ops4.forced, 0);
        auto ops5 = liveHelper(); ops5.live[20] = LiveIdentity{}; ops5.live[20].accessDenied = true;
        QCOMPARE(applyOpen(forged, &ops5)[0].outcome, Outcome::AccessDenied);
        QCOMPARE(ops5.forced, 0);
        // lowering priority never needs the force acknowledgement
        forged[1].control = Control::LowerPriority; forged[1].forceAcknowledged = false;
        auto ops6 = liveHelper();
        QCOMPARE(applyOpen(forged, &ops6)[0].outcome, Outcome::SoftApplied);
    }

    // ---- Stage 3.3 / 3.6 ----
    void startupRequiresConsentAndRefusesAmbiguous()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        e.be.put("Foo", "\"C:\\Apps\\Foo\\foo.exe\" --secret-arg=42");
        e.be.put("Shared", "C:\\S\\t.exe -x");
        e.be.put("Other", "C:\\Nowhere\\z.exe");
        StartupManager m(&e.store, &e.be);
        QVERIFY(!m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, false).ok);
        QVERIFY(e.be.m.contains("Foo"));
        QVERIFY(!m.disable(StartupSource::RunKeyHkcu, "Shared", e.cards, true).ok);
        QVERIFY(!m.disable(StartupSource::RunKeyHkcu, "Other", e.cards, true).ok);
        QVERIFY(!m.disable(StartupSource::RunKeyHklm, "Foo", e.cards, true).ok);
        QCOMPARE(e.be.m.size(), 3);
        for (const auto &row : m.rows(e.cards)) {
            QVERIFY(!row.why.contains("secret")); QVERIFY(!row.name.contains("secret"));
        }
    }
    void disableAndRestoreRoundTrip()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        const QString v = "\"C:\\Apps\\Foo\\foo.exe\" --secret-arg=42";
        e.be.put("Foo", v);
        StartupManager m(&e.store, &e.be);
        auto r = m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, true);
        QVERIFY2(r.ok, qPrintable(r.message));
        QVERIFY(!e.be.m.contains("Foo"));
        QVERIFY(!r.message.contains("secret"));
        const auto id = e.store.entries({}, "applied").value(0).id;
        QVERIFY(id > 0);
        r = m.restore(id);
        QVERIFY(r.ok);
        QCOMPARE(e.be.m["Foo"].value, v);
        QCOMPARE(e.store.entries({}, "restored").size(), 1);
    }
    void originalIsProtectedAtRest()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        e.be.put("Foo", "C:\\Apps\\Foo\\foo.exe --plainmarker");
        StartupManager m(&e.store, &e.be);
        QVERIFY(m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, true).ok);
        e.store.close();
        QFile f(e.db()); QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(!f.readAll().contains("plainmarker"));
        f.close();
        GuardStore other; XorProt wrong('z');
        QVERIFY(other.open(e.db(), &wrong));
        QVERIFY(!other.entryOriginal(other.entries().value(0).id, nullptr, nullptr));
    }
    void conflictsPreserveNewerState()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        e.be.put("Foo", "C:\\Apps\\Foo\\foo.exe --a");
        StartupManager m(&e.store, &e.be);
        QVERIFY(m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, true).ok);
        const auto id = e.store.entries({}, "applied").value(0).id;
        e.be.put("Foo", "C:\\Apps\\Foo\\foo.exe --user-recreated");
        QVERIFY(!m.checkConsistency().isEmpty());
        auto r = m.restore(id);
        QVERIFY(!r.ok); QVERIFY(r.conflict);
        QCOMPARE(e.be.m["Foo"].value, QString("C:\\Apps\\Foo\\foo.exe --user-recreated"));
        QCOMPARE(e.store.entries({}, "applied").size(), 1);
        QVERIFY(!r.message.contains("user-recreated"));
        e.be.m.remove("Foo");
        QVERIFY(m.restore(id).ok);
    }
    void restoreAllAndDisableFailure()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        e.be.put("Foo", "C:\\Apps\\Foo\\foo.exe");
        StartupManager m(&e.store, &e.be);
        e.be.failRemove = true;
        QVERIFY(!m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, true).ok);
        QVERIFY(e.be.m.contains("Foo"));
        QCOMPARE(e.store.entries({}, "applied").size(), 0);
        e.be.failRemove = false;
        QVERIFY(m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, true).ok);
        const auto rs = m.restoreAll();
        QCOMPARE(rs.size(), 1); QVERIFY(rs[0].ok);
        QVERIFY(e.be.m.contains("Foo"));
    }
    void crashRecoveryResolvesPlannedRows()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        StartupManager m(&e.store, &e.be);
        LedgerEntry le; le.cardId = "foo"; le.source = StartupSource::RunKeyHkcu; le.name = "Gone";
        le.originalValue = "C:\\Apps\\Foo\\foo.exe"; le.originalHash = hashOf(le.originalValue);
        LedgerEntry le2 = le; le2.name = "Still";
        QVERIFY(e.store.addPlanned(le, "type=1") > 0);
        QVERIFY(e.store.addPlanned(le2, "type=1") > 0);
        e.be.put("Still", le.originalValue);
        QCOMPARE(m.recover().size(), 2);
        QCOMPARE(e.store.entries({}, "applied").size(), 1);
        QCOMPARE(e.store.entries({}, "abandoned").size(), 1);
        QCOMPARE(e.store.entries({}, "planned").size(), 0);
        ActionRecord a; a.cardId = "foo"; a.pid = 20; a.created = 5; a.exePath = kHelper; a.priorClass = 0x20;
        const qint64 sid = e.store.addSoft(a);
        QCOMPARE(e.store.pendingSoft().size(), 1);
        e.store.markSoftReverted(sid);
        QCOMPARE(e.store.pendingSoft().size(), 0);
    }
    void actionLogHoldsNoStartupValuesOrCommandLines()
    {
        Env e; QVERIFY(e.store.open(e.db(), &e.prot));
        e.be.put("Foo", "C:\\Apps\\Foo\\foo.exe --secret-arg=42");
        StartupManager m(&e.store, &e.be);
        m.disable(StartupSource::RunKeyHkcu, "Foo", e.cards, true);
        auto ops = liveHelper();
        QHash<QString, ClosedPolicy> pc{{"foo", {200, false}}};
        for (const auto &a : enforceClosed(report(), closed(), pc, &ops)) e.store.logAction(a);
        QVERIFY(!e.store.recentActions().isEmpty());
        e.store.close();
        QFile f(e.db()); QVERIFY(f.open(QIODevice::ReadOnly));
        QVERIFY(!f.readAll().contains("secret-arg"));
    }
    void realIsolatedRegistryAndFolderBackend()
    {
        const QString sub = QStringLiteral("Software\\DocklyGuardTest_%1").arg(GetCurrentProcessId());
        QTemporaryDir dir;
        auto be = makeSystemBackend(sub, dir.path());
        QString err;
        QVERIFY(be->write(StartupSource::RunKeyHkcu, "T", "\"C:\\Apps\\Foo\\foo.exe\" /x", "type=1", &err));
        StartupEntry en; QVERIFY(be->read(StartupSource::RunKeyHkcu, "T", &en));
        QVERIFY(en.present);
        QCOMPARE(en.exeHint, QString("C:\\Apps\\Foo\\foo.exe"));
        QVERIFY(be->remove(StartupSource::RunKeyHkcu, "T", &err));
        QVERIFY(be->read(StartupSource::RunKeyHkcu, "T", &en)); QVERIFY(!en.present);
        QVERIFY(be->write(StartupSource::StartupFolder, "x.lnk", QString::fromLatin1(QByteArray("abc").toBase64()), "mtime=1000", &err));
        QVERIFY(be->read(StartupSource::StartupFolder, "x.lnk", &en)); QVERIFY(en.present);
        QVERIFY(!be->write(StartupSource::StartupFolder, "x.lnk", "", "", &err));
        QVERIFY(!be->write(StartupSource::StartupFolder, "..\\evil.lnk", "", "", &err));
        QVERIFY(be->remove(StartupSource::StartupFolder, "x.lnk", &err));
        RegDeleteKeyW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(sub.utf16()));
    }
};

QTEST_MAIN(Guard3Test)
#include "guard3_test.moc"
