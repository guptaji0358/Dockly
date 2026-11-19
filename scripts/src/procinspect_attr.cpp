#include "procinspect.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QHash>
#include <QSet>

namespace ProcInspect {

static QString nrm(const QString &p) { return QDir::toNativeSeparators(QDir::cleanPath(p)).toLower(); }

static bool under(const QString &path, const QString &dir)
{
    if (path.isEmpty() || dir.isEmpty())
        return false;
    const QString d = nrm(dir);
    return nrm(path).startsWith(d.endsWith(QLatin1Char('\\')) ? d : d + QLatin1Char('\\'));
}

namespace {
struct Match {
    int score = 0;
    bool strong = false; // exact executable or package identity
    bool exact = false;
    QStringList evidence;
};
}

Inspection attribute(const QVector<ProcessRecord> &procs, const QVector<CardRef> &cards,
                     const std::function<QString(const QString &)> &publisherOf)
{
    QElapsedTimer t;
    t.start();
    Inspection out;
    out.processCount = procs.size();
    const int n = procs.size();
    QHash<quint32, int> byPid;
    for (int i = 0; i < n; ++i)
        byPid.insert(procs[i].pid, i);

    out.verdicts.resize(n);
    QVector<int> parent(n, -1); // index of the valid parent, else -1 (detached)
    for (int i = 0; i < n; ++i) {
        Verdict &v = out.verdicts[i];
        v.proc = procs[i];
        const auto it = byPid.constFind(procs[i].ppid);
        if (it != byPid.constEnd() && *it != i) {
            const ProcessRecord &p = procs[*it];
            // PID reuse: a real parent was created no later than its child
            if (p.created != 0 && procs[i].created != 0 && p.created <= procs[i].created) {
                parent[i] = *it;
                v.parentValid = true;
                v.parentName = p.name;
            }
        }
    }

    QHash<QString, QString> pubCache;
    auto publisher = [&](const QString &path) {
        if (!publisherOf)
            return QString();
        auto it = pubCache.find(path);
        if (it == pubCache.end())
            it = pubCache.insert(path, publisherOf(path));
        return *it;
    };

    // direct signals per process and card
    QVector<QVector<Match>> direct(n, QVector<Match>(cards.size()));
    for (int i = 0; i < n; ++i) {
        const ProcessRecord &p = procs[i];
        for (int c = 0; c < cards.size(); ++c) {
            const CardRef &cd = cards[c];
            Match &m = direct[i][c];
            if (!p.exePath.isEmpty() && !cd.exePath.isEmpty() && nrm(p.exePath) == nrm(cd.exePath)) {
                m.score += 3; m.strong = m.exact = true;
                m.evidence << QStringLiteral("Executable path equals the card's target (%1)").arg(QDir::toNativeSeparators(p.exePath));
            }
            if (!p.packageFamily.isEmpty() && !cd.packageFamily.isEmpty()
                && p.packageFamily.compare(cd.packageFamily, Qt::CaseInsensitive) == 0) {
                m.score += 3; m.strong = true;
                m.evidence << QStringLiteral("Package identity matches (%1)").arg(p.packageFamily);
            }
            if (!m.strong && under(p.exePath, cd.installDir)) {
                m.score += 2;
                m.evidence << QStringLiteral("Runs from the card's install folder (%1)").arg(cd.installDir);
                const QString pub = publisher(p.exePath);
                if (!pub.isEmpty() && !cd.publisher.isEmpty() && pub.compare(cd.publisher, Qt::CaseInsensitive) == 0) {
                    m.score += 1;
                    m.evidence << QStringLiteral("Same publisher (%1)").arg(pub);
                }
            }
        }
    }

    // owner[i]: card index or -1; qualifies with a strong signal, or install folder + publisher (score >= 3)
    QVector<int> owner(n, -1);
    QVector<QStringList> tied(n);
    QVector<QStringList> evid(n);
    auto decide = [&](int i, const QVector<Match> &ms) {
        int bestTier = 0, best = -1;
        bool tie = false;
        QStringList tieNames;
        for (int c = 0; c < ms.size(); ++c) {
            if (ms[c].score < 3)
                continue;
            const int tier = ms[c].strong ? 2 : 1;
            const int rank = tier * 100 + ms[c].score;
            if (rank > bestTier) { bestTier = rank; best = c; tie = false; tieNames = {cards[c].name}; }
            else if (rank == bestTier) { tie = true; tieNames << cards[c].name; }
        }
        if (best < 0)
            return;
        if (tie) { tied[i] = tieNames; return; }
        owner[i] = best;
        evid[i] = ms[best].evidence;
    };
    for (int i = 0; i < n; ++i)
        decide(i, direct[i]);

    // parent relationship is only a supporting signal: it can complete a single weak signal, never stand alone
    for (int pass = 0; pass < 4; ++pass) {
        bool changed = false;
        for (int i = 0; i < n; ++i) {
            if (owner[i] >= 0 || !tied[i].isEmpty() || parent[i] < 0 || owner[parent[i]] < 0)
                continue;
            const int c = owner[parent[i]];
            Match m = direct[i][c];
            if (m.score >= 2) {
                m.score += 1;
                m.evidence << QStringLiteral("Parent process %1 (PID %2) belongs to this card")
                                  .arg(procs[parent[i]].name).arg(procs[parent[i]].pid);
                QVector<Match> only(cards.size());
                only[c] = m;
                decide(i, only);
                changed = changed || owner[i] >= 0;
            }
        }
        if (!changed)
            break;
    }

    // classification
    auto isService = [&](int i) {
        if (parent[i] < 0)
            return false;
        const QString pp = nrm(procs[parent[i]].exePath);
        return pp.endsWith(QLatin1String("\\system32\\services.exe"));
    };
    // Classification is deliberately conservative. A window or a service parent is NOT proof that a process is
    // essential; they only prove it must not be called a background candidate. Essential is reserved for a
    // process the card's Main process is demonstrably descended from.
    QVector<Class> cls(n, Class::Unknown);
    QVector<bool> keepOut(n, false);   // attributed, but role unproven: stays Unknown
    QVector<bool> cardHasMain(cards.size(), false);
    for (int i = 0; i < n; ++i) {
        if (owner[i] < 0)
            continue;
        const bool pkg = !cards[owner[i]].packageFamily.isEmpty();
        const bool exact = direct[i][owner[i]].exact;
        if ((exact && !pkg) || (pkg && procs[i].hasWindow && direct[i][owner[i]].strong)) {
            cls[i] = Class::Main;
            cardHasMain[owner[i]] = true;
        }
    }
    for (int i = 0; i < n; ++i) {
        if (owner[i] < 0 || cls[i] == Class::Main)
            continue;
        if (procs[i].hasWindow || isService(i))
            keepOut[i] = true;
    }
    // descendants of a live Main process (renderers, GPU/utility children...) are part of the running app, and a
    // system component running from a shared location (brokers, dllhost) acts for many apps: neither is a candidate
    QVector<QString> why(n);
    for (int i = 0; i < n; ++i) {
        if (owner[i] < 0 || cls[i] == Class::Main)
            continue;
        int a = parent[i];
        for (int depth = 0; a >= 0 && depth < 32; ++depth, a = parent[a])
            if (owner[a] == owner[i] && cls[a] == Class::Main) {
                keepOut[i] = true;
                why[i] = QStringLiteral("It is a descendant of the card's live Main process (PID %1); children are not assumed unnecessary, so it is left Unknown.").arg(procs[a].pid);
                break;
            }
        if (!keepOut[i] && !procs[i].exePath.isEmpty() && isSharedLocation(QFileInfo(procs[i].exePath).absolutePath())) {
            keepOut[i] = true;
            why[i] = QStringLiteral("It runs from a shared system location (%1), so it may serve other programs too; left Unknown.")
                         .arg(QDir::toNativeSeparators(QFileInfo(procs[i].exePath).absolutePath()));
        }
    }
    // an ancestor of a window owner is kept out of the candidates too (something visible hangs below it)
    for (int i = 0; i < n; ++i) {
        if (owner[i] < 0 || !procs[i].hasWindow)
            continue;
        int a = parent[i];
        for (int depth = 0; a >= 0 && depth < 32; ++depth, a = parent[a])
            if (owner[a] == owner[i])
                keepOut[a] = true;
    }
    // Essential: a same-card process that is a valid ancestor of that card's Main process
    for (int i = 0; i < n; ++i) {
        if (cls[i] != Class::Main)
            continue;
        int a = parent[i];
        for (int depth = 0; a >= 0 && depth < 32; ++depth, a = parent[a])
            if (owner[a] == owner[i] && cls[a] != Class::Main)
                cls[a] = Class::Essential;
    }
    for (int i = 0; i < n; ++i) {
        if (owner[i] < 0 || cls[i] != Class::Unknown || keepOut[i])
            continue;
        // packaged apps have no exact "main exe": without a windowed process to anchor on, stay Unknown
        if (!cards[owner[i]].packageFamily.isEmpty() && !cardHasMain[owner[i]])
            continue;
        cls[i] = Class::CandidateBackground;
    }

    for (int i = 0; i < n; ++i) {
        Verdict &v = out.verdicts[i];
        v.cls = cls[i];
        if (owner[i] >= 0) {
            v.cardId = cards[owner[i]].id;
            v.cardName = cards[owner[i]].name;
            v.evidence = evid[i];
            if (procs[i].hasWindow)
                v.evidence << QStringLiteral("Owns a visible window");
            if (isService(i))
                v.evidence << QStringLiteral("Started by the Service Control Manager");
            if (!v.parentValid)
                v.evidence << QStringLiteral("Detached: parent PID %1 is gone or was reused").arg(procs[i].ppid);
            switch (v.cls) {
            case Class::Main: v.reason = QStringLiteral("The card's own program; never touched."); break;
            case Class::Essential:
                v.reason = QStringLiteral("The card's Main process was started through it, so the app depends on it; never touched.");
                break;
            case Class::CandidateBackground:
                v.reason = QStringLiteral("Attributed by independent signals, no window, not a service child and nothing visible below it. Candidate only: Stage 2 takes no action.");
                break;
            default:
                v.reason = procs[i].hasWindow ? QStringLiteral("Belongs to this card but shows a window; its role is not proven, so it is left Unknown.")
                           : isService(i) ? QStringLiteral("Belongs to this card but runs under the service manager; services are never assumed unneeded or essential, so it is left Unknown.")
                           : !why[i].isEmpty() ? why[i]
                           : keepOut[i] ? QStringLiteral("Belongs to this card but a visible window runs below it; left Unknown.")
                                        : QStringLiteral("Belongs to this card, but there is no main window to anchor on yet; left Unknown.");
            }
        } else if (!tied[i].isEmpty()) {
            v.ambiguousCards = tied[i];
            v.reason = QStringLiteral("Ambiguous ownership: equally matches %1. Left untouched.").arg(tied[i].join(QStringLiteral(", ")));
            v.evidence << QStringLiteral("Equal evidence for several cards");
        } else if (procs[i].exePath.isEmpty()) {
            v.reason = QStringLiteral("Executable path not readable (protected process). Left untouched, no elevation attempted.");
        } else {
            // note a lone weak signal so the user can see why it was not attributed
            for (int c = 0; c < cards.size(); ++c)
                if (direct[i][c].score > 0)
                    v.evidence << direct[i][c].evidence << QStringLiteral("...but only one weak signal for %1").arg(cards[c].name);
            v.reason = v.evidence.isEmpty() ? QStringLiteral("No signal links it to any card. Left untouched.")
                                            : QStringLiteral("Not enough independent signals to name an owner. Left untouched.");
        }
    }
    out.elapsedMs = t.elapsed();
    return out;
}

}
