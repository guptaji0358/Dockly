#include "guardengine.h"
#include "guardledger.h"
#include "procinspect.h"

#include <QElapsedTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>
#include <windows.h>

using namespace Guard;
using ProcInspect::CardRef;
using ProcInspect::Class;
using ProcInspect::ProcessRecord;

namespace {
const quint64 kSec = 10000000ULL;
const quint64 kNow = 1000000ULL * kSec;

ProcessRecord rec(quint32 pid, quint32 ppid, const QString &exe, quint64 ageSec = 600, bool win = false)
{
    ProcessRecord r;
    r.pid = pid; r.ppid = ppid; r.exePath = exe;
    r.name = exe.section('\\', -1);
    r.created = ageSec ? kNow - ageSec * kSec : 0;
    r.hasWindow = win;
    return r;
}
CardRef card(const QString &id, const QString &exe, const QString &dir)
{
    CardRef c; c.id = id; c.name = id; c.exePath = exe; c.installDir = dir; c.publisher = "Pub";
    return c;
}
ProcessFacts okFacts(quint32 pid, int session = 1)
{
    ProcessFacts f; f.pid = pid; f.sessionId = session; f.terminateAccess = Tri::Yes; return f;
}
const QString kMain = "C:\\Apps\\Foo\\foo.exe";
const QString kHelper = "C:\\Apps\\Foo\\foohelper.exe";
}

class GuardTest : public QObject
{
    Q_OBJECT

    // Main (pid 10, windowed) + helper (pid 20, detached, no window) for card "foo"
    ProcInspect::Inspection scene(const QVector<ProcessRecord> &extra = {})
    {
        QVector<ProcessRecord> p{rec(10, 1, kMain, 900, true), rec(20, 1, kHelper, 900)};
        p += extra;
        return ProcInspect::attribute(p, {card("foo", kMain, "C:\\Apps\\Foo")},
                                      [](const QString &) { return QStringLiteral("Pub"); });
    }
    Policy policy(bool allowHelper = true)
    {
        Policy po; po.currentSessionId = 1;
        CardPolicy cp; cp.cardId = "foo";
        if (allowHelper) cp.allowedExePaths.insert(normPath(kHelper));
        po.cards.insert("foo", cp);
        return po;
    }
    static const Decision &D(const DryRunReport &r, quint32 pid)
    {
        for (const auto &d : r.decisions) if (d.verdict.proc.pid == pid) return d;
        static Decision none; return none;
    }

private slots:
    void candidateWithAllGatesIsOnlyProposed()
    {
        const auto insp = scene();
        QCOMPARE(insp.verdicts[1].cls, Class::CandidateBackground);
        QHash<quint32, ProcessFacts> f{{10, okFacts(10)}, {20, okFacts(20)}};
        const auto r = dryRun(insp, f, policy(), kNow);
        QCOMPARE(D(r, 20).action, Action::WouldStop);
        QVERIFY(!D(r, 20).executed);
        QVERIFY(!r.executedAnything);
        QCOMPARE(D(r, 10).action, Action::Keep);
        qInfo() << "EXAMPLE" << D(r, 20).explanation;
    }
    void candidateIsNotAuthorisation()
    {
        QHash<quint32, ProcessFacts> f{{20, okFacts(20)}};
        const auto r = dryRun(scene(), f, policy(false), kNow);
        QCOMPARE(D(r, 20).action, Action::Observe);
        QVERIFY(D(r, 20).blockers.join(' ').contains("allow-list"));
        Policy none; none.currentSessionId = 1; // no policy for the card at all
        QCOMPARE(D(dryRun(scene(), f, none, kNow), 20).action, Action::Observe);
        Policy off = policy(); off.cards["foo"].guardOn = false;
        QCOMPARE(D(dryRun(scene(), f, off, kNow), 20).action, Action::Observe);
        Policy master = policy(); master.masterOn = false;
        QCOMPARE(D(dryRun(scene(), f, master, kNow), 20).action, Action::Observe);
        qInfo() << "EXAMPLE" << D(r, 20).explanation;
    }
    void terminateDeniedBlocks()
    {
        ProcessFacts f = okFacts(20); f.terminateAccess = Tri::No;
        const auto r = dryRun(scene(), {{20, f}}, policy(), kNow);
        QCOMPARE(D(r, 20).action, Action::Observe);
        QVERIFY(D(r, 20).blockers.join(' ').contains("PROCESS_TERMINATE"));
        qInfo() << "EXAMPLE" << D(r, 20).explanation;
        f.terminateAccess = Tri::Unknown; // unverified is also protected
        QCOMPARE(D(dryRun(scene(), {{20, f}}, policy(), kNow), 20).action, Action::Observe);
    }
    void session0AndOtherSessionsAndMissingFactsAreProtected()
    {
        QCOMPARE(D(dryRun(scene(), {{20, okFacts(20, 0)}}, policy(), kNow), 20).action, Action::Observe);
        QCOMPARE(D(dryRun(scene(), {{20, okFacts(20, 3)}}, policy(), kNow), 20).action, Action::Observe);
        QCOMPARE(D(dryRun(scene(), {{20, okFacts(20, -1)}}, policy(), kNow), 20).action, Action::Observe);
        QCOMPARE(D(dryRun(scene(), {}, policy(), kNow), 20).action, Action::Observe);
        ProcessFacts denied; denied.pid = 20; denied.queryDenied = true; denied.sessionId = 1;
        QCOMPARE(D(dryRun(scene(), {{20, denied}}, policy(), kNow), 20).action, Action::Observe);
    }
    void inaccessibleProcessWithoutPathStaysUntouched()
    {
        auto p = rec(30, 4, QString(), 0); // protected: no path, no creation time
        const auto insp = scene({p});
        const auto r = dryRun(insp, {}, policy(), kNow);
        QCOMPARE(D(r, 30).action, Action::Keep);
        QVERIFY(D(r, 30).explanation.contains("not inspectable"));
    }
    void unknownProcessesRemainUntouched()
    {
        const auto insp = scene({rec(40, 1, "C:\\Other\\random.exe")});
        const auto r = dryRun(insp, {{40, okFacts(40)}}, policy(), kNow);
        QCOMPARE(D(r, 40).action, Action::Keep);
        QCOMPARE(D(r, 40).verdict.cls, Class::Unknown);
    }
    void ambiguousSharedAttributionIsProtected()
    {
        const QString exe = "C:\\Shared\\tool.exe";
        QVector<ProcessRecord> p{rec(50, 1, exe)};
        const auto insp = ProcInspect::attribute(p, {card("a", exe, ""), card("b", exe, "")});
        QVERIFY(!insp.verdicts[0].ambiguousCards.isEmpty());
        Policy po; po.currentSessionId = 1;
        for (auto id : {"a", "b"}) { CardPolicy c; c.cardId = id; c.allowedExePaths.insert(normPath(exe)); po.cards.insert(id, c); }
        const auto r = dryRun(insp, {{50, okFacts(50)}}, po, kNow);
        QCOMPARE(r.decisions[0].action, Action::Keep);
        QCOMPARE(r.wouldStop, 0);
    }
    void gracePeriodAndIdentity()
    {
        auto young = scene({});
        young.verdicts[1].proc.created = kNow - 5 * kSec;
        QCOMPARE(D(dryRun(young, {{20, okFacts(20)}}, policy(), kNow), 20).action, Action::Observe);
        auto noId = scene({});
        noId.verdicts[1].proc.created = 0;
        QCOMPARE(D(dryRun(noId, {{20, okFacts(20)}}, policy(), kNow), 20).action, Action::Observe);
    }
    void pidReuseIsDetected()
    {
        const QVector<ProcessRecord> before{rec(20, 1, kHelper, 900)};
        const QVector<ProcessRecord> after{rec(20, 1, "C:\\Other\\x.exe", 5)}; // same PID, new creation time
        const auto d = diffSnapshots(before, after);
        QCOMPARE(d.started.size(), 1);
        QCOMPARE(d.exited.size(), 1);
        // a stale identity must not match the new process
        QVERIFY(!ProcInspect::stillSameProcess(0xFFFFFFF0u, 1));
        QVERIFY(ProcInspect::stillSameProcess(GetCurrentProcessId(), ProcInspect::creationTimeOf(GetCurrentProcessId())));
        QVERIFY(!ProcInspect::stillSameProcess(GetCurrentProcessId(), ProcInspect::creationTimeOf(GetCurrentProcessId()) + 1));
        // unreadable creation time is uncertain, never "new"
        const auto u = diffSnapshots({}, {rec(60, 1, "x", 0)});
        QCOMPARE(u.started.size(), 0);
        QCOMPARE(u.uncertain, 1);
    }

    // ---- ledger ----
    static LedgerEntry removedEntry()
    {
        LedgerEntry e; e.cardId = "foo"; e.source = StartupSource::RunKeyHkcu; e.name = "Foo";
        e.originalValue = QStringLiteral("\"foo.exe\" /bg"); e.originalHash = hashOf(e.originalValue);
        e.appliedRemoved = true; e.state = "applied";
        return e;
    }
    void restoreConflictDetection()
    {
        LedgerEntry e = removedEntry();
        StartupEntry cur; cur.source = e.source; cur.name = e.name;
        cur.present = false;
        QCOMPARE(evaluateRestore(e, cur), RestoreVerdict::SafeToRestore);
        cur.present = true; cur.value = "C:\Elsewhere\new.exe"; // user re-created with other value
        QCOMPARE(evaluateRestore(e, cur), RestoreVerdict::ConflictRecreated);
        cur.value = e.originalValue; // user already restored it
        QCOMPARE(evaluateRestore(e, cur), RestoreVerdict::AlreadyRestored);

        LedgerEntry m = removedEntry(); // rewritten case
        m.appliedRemoved = false; m.appliedHash = hashOf("changed-by-dockly");
        cur.value = "changed-by-dockly";
        QCOMPARE(evaluateRestore(m, cur), RestoreVerdict::SafeToRestore);
        cur.value = "user-edited";
        QCOMPARE(evaluateRestore(m, cur), RestoreVerdict::ConflictModified);
        cur.present = false;
        QCOMPARE(evaluateRestore(m, cur), RestoreVerdict::ConflictMissing);
        LedgerEntry n = removedEntry(); n.state = "planned";
        QCOMPARE(evaluateRestore(n, cur), RestoreVerdict::RefusedNotApplied);
    }
    void packagedStartupIsManagedByWindows()
    {
        StartupEntry e; e.source = StartupSource::PackagedStartupTask; e.name = "Store app";
        const auto p = planStartup(e);
        QVERIFY(p.managedByWindows);
        QVERIFY(!p.actionable);
        QVERIFY(p.reason.contains("managed by Windows"));
        LedgerEntry le = removedEntry(); le.source = e.source;
        QCOMPARE(evaluateRestore(le, e), RestoreVerdict::RefusedManagedByWindows);
        StartupEntry hklm; hklm.source = StartupSource::RunKeyHklm;
        QVERIFY(!planStartup(hklm).actionable);
        StartupEntry hk; hk.source = StartupSource::RunKeyHkcu;
        QVERIFY(planStartup(hk).actionable);
    }
    void ledgerSchemaCreatesAndRejectsUnsafeSources()
    {
        QTemporaryDir dir;
        const QString path = dir.path() + "/ledger.db";
        QString err;
        QVERIFY2(ensureLedgerSchema(path, &err), qPrintable(err));
        QVERIFY(ensureLedgerSchema(path, &err)); // idempotent
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", "lt");
            db.setDatabaseName(path); QVERIFY(db.open());
            QSqlQuery q(db);
            QVERIFY(q.exec("SELECT value FROM ledger_meta WHERE key='schema_version'") && q.next());
            QCOMPARE(q.value(0).toInt(), kLedgerSchemaVersion);
            QVERIFY(q.exec("SELECT COUNT(*) FROM recovery_entries") && q.next());
            QCOMPARE(q.value(0).toInt(), 0); // no entries are ever created in this phase
            const char *ins = "INSERT INTO recovery_entries(card_id,source,location,name,original_value,original_hash,applied_removed,state,created_utc) VALUES('c','%1','l','n','v','h',1,'applied','t')";
            QVERIFY(!q.exec(QString(ins).arg("packaged-task"))); // CHECK constraint
            QVERIFY(!q.exec(QString(ins).arg("run-hklm")));
            QVERIFY(q.exec(QString(ins).arg("run-hkcu")));
            db.close();
        }
        QSqlDatabase::removeDatabase("lt");
    }

    // ---- real machine: polling cost and dry-run ----
    void pollingOverhead()
    {
        QElapsedTimer t;
        QVector<qint64> times;
        QString err;
        auto prev = ProcInspect::collectProcesses(&err);
        QVERIFY(prev.size() > 10);
        FILETIME c, e, k0, u0, k1, u1;
        GetProcessTimes(GetCurrentProcess(), &c, &e, &k0, &u0);
        QElapsedTimer wall; wall.start();
        const int N = 50;
        int started = 0;
        for (int i = 0; i < N; ++i) {
            t.start();
            auto now = ProcInspect::collectProcesses(&err);
            auto d = diffSnapshots(prev, now);
            started += d.started.size();
            times << t.nsecsElapsed() / 1000;
            prev = now;
        }
        GetProcessTimes(GetCurrentProcess(), &c, &e, &k1, &u1);
        auto tk = [](FILETIME f) { return (quint64(f.dwHighDateTime) << 32 | f.dwLowDateTime) / 10000.0; };
        const double cpuMs = tk(k1) + tk(u1) - tk(k0) - tk(u0);
        std::sort(times.begin(), times.end());
        qInfo().nospace() << "POLL processes=" << prev.size() << " polls=" << N
                          << " median_us=" << times[N / 2] << " p95_us=" << times[N * 95 / 100]
                          << " max_us=" << times.last() << " cpu_ms_per_poll=" << cpuMs / N
                          << " wall_ms=" << wall.elapsed() << " started_seen=" << started;
        QVERIFY(times[N / 2] < 500000);
    }
    void realMachineDryRunTouchesNothing()
    {
        const auto procs = ProcInspect::collectProcesses();
        QElapsedTimer t; t.start();
        const auto facts = collectFacts(procs);
        const qint64 factsMs = t.elapsed();
        qInfo() << "FACTS ms" << factsMs << "count" << facts.size();
        int denied = 0, s0 = 0;
        for (const auto &f : facts) { denied += f.terminateAccess == Tri::No; s0 += f.sessionId == 0; }
        qInfo() << "terminate-denied" << denied << "session0" << s0;
        // a card for this very test executable; every other process must stay Keep
        const QString me = QCoreApplication::applicationFilePath();
        auto insp = ProcInspect::attribute(procs, {ProcInspect::makeCardRef("me", "guard_test", me)});
        Policy po; po.currentSessionId = currentSessionId();
        const auto r = dryRun(insp, facts, po, nowFiletime());
        QCOMPARE(r.wouldStop, 0); // no policy => nothing proposed
        QVERIFY(!r.executedAnything);
        for (const auto &d : r.decisions) QVERIFY(!d.executed);
        QVERIFY(QCoreApplication::applicationPid() > 0); // still alive
    }
};

QTEST_MAIN(GuardTest)
#include "guard_test.moc"
