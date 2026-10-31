#include "appcore.h"
#include "iconpack.h"
#include "itemmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QTcpSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

class CoreTest : public QObject
{
    Q_OBJECT
    QTemporaryDir m_dir;
    QTemporaryFile m_realFile;
    AppCore *core = nullptr;

private slots:
    void initTestCase()
    {
        QVERIFY(m_dir.isValid());
        qputenv("LH_DATA_DIR", m_dir.path().toUtf8());
        QVERIFY(m_realFile.open());
        core = AppCore::instance();
        core->settings()->set("checkMissingOnStartup", false);
        core->init();
        QVERIFY(QFile::exists(m_dir.path() + "/library.db"));
        QVERIFY(QFile::exists(m_dir.path() + "/catalog.db"));
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::AllCategories, {}, {}).size(), 0);
        // Detected software is never turned into cards on its own.
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::Software, {}, {}).size(), 0);
    }

    void addNormalizesAndDedupes()
    {
        const QString id = core->addItem(LH::Web, {}, "example.com/path");
        QVERIFY(!id.isEmpty());
        QCOMPARE(core->item(id).target, QString("https://example.com/path"));
        QCOMPARE(core->item(id).name, QString("example.com"));
        QCOMPARE(core->addItem(LH::Web, "dup", "https://example.com/path"), id);
        QCOMPARE(core->item(id).status, LH::Normal);
    }

    void statusDetection()
    {
        const QString bad = core->addItem(LH::Web, "bad", "https://");
        QCOMPARE(core->item(bad).status, LH::Warning);
        const QString gone = core->addItem(LH::Document, "gone", m_dir.path() + "/nope.pdf");
        QCOMPARE(core->item(gone).status, LH::Missing);
        QCOMPARE(core->launch(gone), int(LH::TargetMissing));
        QCOMPARE(core->launch(bad), int(LH::TargetMissing));

        // "Locate again": pointing at a real file clears the warning.
        QVERIFY(core->updateItem(gone, "found", m_realFile.fileName()));
        QCOMPARE(core->item(gone).status, LH::Normal);
    }

    void collectionsAndModel()
    {
        const QString a = core->addItem(LH::Document, "a", m_realFile.fileName());
        const QString web = core->addItem(LH::Web, "w", "https://qt.io");
        const QString col = core->createCollection("Work");
        core->addToCollection(col, {a, web, web, col}); // duplicate and self are ignored
        QCOMPARE(core->item(col).memberCount, 2);

        ItemModel members;
        members.setMode(LH::ModeMembers);
        members.setCollectionId(col);
        QCOMPARE(members.count(), 2);

        core->setFavorite(web, true);
        ItemModel favs;
        favs.setMode(LH::ModeFavorites);
        QVERIFY(favs.count() >= 1);

        ItemModel search;
        search.setSearch("qt.io");
        QCOMPARE(search.count(), 1);

        // Deleting an item removes it from collections too.
        core->removeItem(web);
        QCOMPARE(core->item(col).memberCount, 1);
        QCOMPARE(members.count(), 1);
    }

    void scanFillsCatalogOnlyOnDemand()
    {
        QCOMPARE(core->catalogCount(), 0); // nothing is scanned at startup
        QSignalSpy spy(core, &AppCore::catalogChanged);
        core->refreshSoftware();
        QVERIFY(spy.wait(60000));
        QVERIFY(core->catalogCount() > 0);
        QVERIFY(core->catalogSearch({}, 5).size() > 0);
        // ...and the scan never creates cards by itself.
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::Software, {}, {}).size(), 0);
    }

    void catalogIsHiddenUntilPicked()
    {
        QVERIFY(core->catalogSearch({}, 3).size() > 0);

        const int before = core->queryIds(LH::ModeCategory, LH::Software, {}, {}).size();
        const int added = core->addSoftware({QVariantMap{{"name", "Fake App"}, {"target", m_realFile.fileName()}}});
        QCOMPARE(added, 1);
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::Software, {}, {}).size(), before + 1);
        // Adding the same program twice does not duplicate the card.
        QCOMPARE(core->addSoftware({QVariantMap{{"name", "Fake App"}, {"target", m_realFile.fileName()}}}), 0);
    }

    void customIconReplacesOriginalOnlyWhenSet()
    {
        const QString id = core->addItem(LH::Web, "iconed", "https://iconed.example");
        const QPixmap original = core->pixmapFor(id, 64);
        QImage red(32, 32, QImage::Format_RGB32);
        red.fill(Qt::red);
        const QString png = m_dir.path() + "/own.bmp";
        QVERIFY(red.save(png));
        QVERIFY(core->setItemIcon(id, png));
        QCOMPARE(core->pixmapFor(id, 64).toImage().pixel(32, 32), QColor(Qt::red).rgb());
        QCOMPARE(core->pixmapOriginal(id, 64).toImage(), original.toImage()); // original stays reachable
        QVERIFY(!core->setItemIcon(id, m_dir.path() + "/missing.png"));
        core->clearItemIcon(id);
        QCOMPARE(core->pixmapFor(id, 64).toImage(), original.toImage());
        QVERIFY(core->imageFilter().contains("*.png"));
    }

    void backupRestoreRoundTrip()
    {
        core->settings()->set("theme", "light");
        core->settings()->set("maxRecent", 5);
        const QString id = core->addItem(LH::Web, "keep", "https://keep.me");
        const QString path = m_dir.path() + "/backup.json";
        QVERIFY(core->backupTo(path));

        core->removeItem(id);
        core->resetSettings();
        QCOMPARE(core->settings()->text("theme"), QString("dark"));

        QVERIFY(core->restoreFrom(path));
        QVERIFY(core->exists(id));
        QCOMPARE(core->settings()->text("theme"), QString("light"));
        QCOMPARE(core->settings()->integer("maxRecent"), 5);

        QFile bad(m_dir.path() + "/bad.json");
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("{\"hello\":1}");
        bad.close();
        QVERIFY(!core->restoreFrom(bad.fileName()));
        QVERIFY(core->exists(id)); // a bad file leaves data untouched
    }

    void droppedFilesGoToTheRightHub()
    {
        const QString d = m_dir.path();
        auto touch = [&](const QString &name) {
            QFile f(d + "/" + name);
            f.open(QIODevice::WriteOnly);
            f.write("x");
            return QUrl::fromLocalFile(f.fileName()).toString();
        };
        const QStringList drop = {touch("tool.exe"), touch("photo.png"), touch("notes.txt"),
                                  "https://dropped.example/page", d + "/does-not-exist.pdf"};
        QCOMPARE(core->addDropped(drop), 4); // the missing file is skipped
        QCOMPARE(core->addDropped(drop), 0); // dropping the same things again adds nothing new

        auto has = [&](int cat, const QString &needle) {
            return !core->queryIds(LH::ModeCategory, cat, needle, {}).isEmpty();
        };
        QVERIFY(has(LH::Software, "tool.exe"));
        QVERIFY(has(LH::Entertainment, "photo.png"));
        QVERIFY(has(LH::Document, "notes.txt"));
        QVERIFY(has(LH::Web, "dropped.example"));
    }

    void membersCanBeReordered()
    {
        const QString col = core->createCollection("Order");
        QStringList ids;
        for (const QString &n : {"a1", "a2", "a3"})
            ids << core->addItem(LH::Web, n, "https://" + n + ".order.example");
        core->addToCollection(col, ids);
        ItemModel m;
        m.setMode(LH::ModeMembers);
        m.setCollectionId(col);
        QCOMPARE(m.idAt(0), ids[0]);

        core->moveMember(col, ids[2], 0); // last -> first
        QCOMPARE(m.idAt(0), ids[2]);
        QCOMPARE(m.idAt(1), ids[0]);
        QCOMPARE(m.idAt(2), ids[1]);
        core->moveMember(col, ids[2], 99); // clamped to the end
        QCOMPARE(m.idAt(2), ids[2]);
    }

    void quickSearchRanksPrefixFirstAndFavouritesWin()
    {
        const QString contains = core->addItem(LH::Web, "myzqpad notes", "https://zq1.example");
        const QString exact = core->addItem(LH::Web, "zqpad", "https://zq2.example");
        const QString word = core->addItem(LH::Web, "the zqpad tool", "https://zq3.example");
        const QString fav = core->addItem(LH::Web, "zqpad pro", "https://zq4.example");
        const QString plain = core->addItem(LH::Web, "zqpad lite", "https://zq5.example");
        core->setFavorite(fav, true);

        const QVariantList hits = core->quickSearch("zqpad", 10);
        QCOMPARE(hits.size(), 5);
        QCOMPARE(hits[0].toMap()["id"].toString(), exact);    // exact name first
        QCOMPARE(hits[1].toMap()["id"].toString(), fav);      // among prefix matches the favourite comes first ...
        QCOMPARE(hits[2].toMap()["id"].toString(), plain);    // ... then the rest
        QCOMPARE(hits[3].toMap()["id"].toString(), word);     // word prefix before plain "contains"
        QCOMPARE(hits[4].toMap()["id"].toString(), contains);
        QCOMPARE(core->quickSearch("zqpad", 2).size(), 2);    // limit
        QVERIFY(core->quickSearch("no-such-thing-xyz", 5).isEmpty());
        QVERIFY(core->quickSearch({}, 50).toList().size() >= 1); // empty query lists favourites
    }

    void droppedOnCollectionLandsInIt()
    {
        const QString col = core->createCollection("Drop target");
        const QString f = m_dir.path() + "/dropped-in.txt";
        QFile file(f);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("x");
        file.close();
        const QStringList drop = {QUrl::fromLocalFile(f).toString(), "https://into-collection.example", m_dir.path() + "/nope.bin"};
        QCOMPARE(core->addDroppedTo(col, drop), 2); // the missing path is skipped
        QCOMPARE(core->addDroppedTo(col, drop), 2); // already-known items are still (re)filed, no duplicates
        QCOMPARE(core->queryIds(LH::ModeMembers, LH::AllCategories, {}, col).size(), 2);
        QCOMPARE(core->addDroppedTo(m_realFile.fileName(), drop), 0); // not a collection
    }

    void workspaceLaunchSkipsMissingItems()
    {
        // (nothing launchable here on purpose: a test must not open real programs or browser tabs)
        core->settings()->set("workspaceDelay", 400);
        const QString col = core->createCollection("Workspace");
        const QString bad = core->addItem(LH::Web, "ws bad", "https://");
        const QString gone = core->addItem(LH::Document, "ws gone", m_dir.path() + "/ws-missing.pdf");
        core->addToCollection(col, {bad, gone});
        QSignalSpy msg(core, &AppCore::message);
        QCOMPARE(core->launchAll(col), 0); // nothing that can open is queued
        QVERIFY(!msg.isEmpty());
        QVERIFY(msg.last().at(0).toString().contains("2 couldn't be opened"));
        QCOMPARE(core->launchAll("not-a-collection"), 0);
    }

    void quickActionsOnlyApplyToPrograms()
    {
        const QString web = core->addItem(LH::Web, "qa web", "https://qa.example");
        QVERIFY(!core->launchAsAdmin(web));
        QVERIFY(!core->launchWithArgs(web, "--x"));
        core->copyPath(web);
        QCOMPARE(QGuiApplication::clipboard()->text(), QString("https://qa.example"));
    }

    void colourThemesRetintTheSkin()
    {
        auto *s = core->settings();
        s->set("theme", "dark");
        s->set("accent", "");
        s->set("palette", "default");
        const QColor base = core->theme()->bg();
        s->set("palette", "forest");
        QVERIFY(core->theme()->bg() != base);
        QCOMPARE(core->theme()->accent(), QColor("#2fbf71"));
        s->set("accent", "#123456"); // a custom accent still wins over the theme's
        QCOMPARE(core->theme()->accent(), QColor("#123456"));
        s->set("accent", "");
        s->set("palette", "no-such-theme"); // unknown key behaves like "default"
        QCOMPARE(core->theme()->bg(), base);
        s->set("palette", "default");
        QCOMPARE(core->theme()->palettes().size(), 7);
    }

    void iconPackAndLetterTile()
    {
        for (const QString &name : IconPack::names()) {
            const QPixmap pm = IconPack::pixmap(name, 48, Qt::white);
            QVERIFY2(!pm.isNull(), qPrintable(name));
            const QImage img = pm.toImage();
            bool drawn = false;
            for (int y = 0; y < img.height() && !drawn; ++y)
                for (int x = 0; x < img.width(); ++x)
                    if (qAlpha(img.pixel(x, y)) > 0) { drawn = true; break; }
            QVERIFY2(drawn, qPrintable(name + " renders nothing"));
        }
        // A web card without a fetched icon gets a letter tile (never the old globe).
        const QString id = core->addItem(LH::Web, "Zebra", "https://zebra.invalid");
        const QPixmap tile = core->pixmapFor(id, 64);
        QVERIFY(!tile.isNull());
        QCOMPARE(tile.size(), QSize(64, 64));
    }

    void searchIsEscaped()
    {
        core->addItem(LH::Web, "100% legit", "https://pct.example");
        ItemModel m;
        m.setSearch("100%");
        QCOMPARE(m.count(), 1);
        m.setSearch("e_a");
        QCOMPARE(m.count(), 0); // '_' is literal, not a wildcard
    }

    // ---- connectors -------------------------------------------------------------------

    QByteArray httpRequest(int port, const QByteArray &method, const QByteArray &path, const QByteArray &token,
                           const QByteArray &body = {}, const QByteArray &host = "127.0.0.1")
    {
        QTcpSocket sock;
        sock.connectToHost("127.0.0.1", quint16(port));
        for (int i = 0; i < 300 && sock.state() != QAbstractSocket::ConnectedState; ++i)
            QTest::qWait(10);
        QByteArray req = method + " " + path + " HTTP/1.1\r\nHost: " + host + "\r\n";
        if (!token.isEmpty())
            req += "Authorization: Bearer " + token + "\r\n";
        req += "Content-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body;
        sock.write(req);
        QByteArray out;
        for (int i = 0; i < 300 && sock.state() != QAbstractSocket::UnconnectedState; ++i) {
            QTest::qWait(10);
            out += sock.readAll();
        }
        return out + sock.readAll();
    }

    void bridgeOnlyAcceptsPairedHttpLinks()
    {
        Connectors *cn = core->connectors();
        cn->setBridgeEnabled(true);
        QVERIFY(cn->bridgeRunning());
        const QString code = cn->pairingCode();
        const int port = cn->bridgePort();
        QCOMPARE(code.section('.', 0, 0).toInt(), port);
        const QByteArray token = code.section('.', 1).toUtf8();
        QVERIFY(token.size() >= 24);

        QVERIFY(httpRequest(port, "GET", "/ping", {}).contains("401"));
        QVERIFY(httpRequest(port, "GET", "/ping", "wrong").contains("401"));
        QVERIFY(httpRequest(port, "GET", "/ping", token, {}, "evil.example").contains("403"));
        QVERIFY(httpRequest(port, "GET", "/ping", token).contains("\"ok\":true"));

        const QByteArray goodBody = "{\"url\":\"https://bridge.example/page\",\"title\":\"Bridge Page\"}";
        const QByteArray jsBody = "{\"url\":\"javascript:alert(1)\"}";
        const QByteArray fileBody = "{\"url\":\"file:///c:/secret\"}";
        const QByteArray reply = httpRequest(port, "POST", "/add", token, goodBody);
        QVERIFY(reply.contains("\"added\":1"));
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::Web, "bridge.example", {}).size(), 1);

        QVERIFY(httpRequest(port, "POST", "/add", token, jsBody).contains("400"));
        QVERIFY(httpRequest(port, "POST", "/add", token, fileBody).contains("400"));
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::AllCategories, "javascript", {}).size(), 0);

        // a regenerated token invalidates the old one
        cn->regenerateToken();
        QVERIFY(httpRequest(port, "GET", "/ping", token).contains("401"));
        cn->setBridgeEnabled(false);
        QVERIFY(!cn->bridgeRunning());
    }

    void bookmarksAreParsedFromChromeFormat()
    {
        QTemporaryFile f;
        QVERIFY(f.open());
        f.write("{\"roots\":{\"bookmark_bar\":{\"name\":\"Bookmarks bar\",\"type\":\"folder\",\"children\":["
                "{\"name\":\"Qt\",\"type\":\"url\",\"url\":\"https://qt.io\"},"
                "{\"name\":\"Dev\",\"type\":\"folder\",\"children\":["
                "{\"name\":\"Docs\",\"type\":\"url\",\"url\":\"https://doc.qt.io\"},"
                "{\"name\":\"Bad\",\"type\":\"url\",\"url\":\"javascript:alert(1)\"}]}]},"
                "\"other\":{\"name\":\"Other\",\"type\":\"folder\",\"children\":[]}}}");
        f.flush();
        const QVariantList list = core->connectors()->bookmarksFromFile(f.fileName());
        QCOMPARE(list.size(), 2); // the javascript: bookmark is dropped
        QCOMPARE(list[0].toMap()["target"].toString(), QString("https://qt.io"));
        QCOMPARE(list[1].toMap()["sub"].toString(), QString("Bookmarks bar / Dev"));
        QCOMPARE(core->connectors()->addEntries(list), 2);
    }

    static int siteIndex(Sites *s, const QString &id)
    {
        for (const QVariant &v : s->list())
            if (v.toMap()["id"].toString() == id)
                return v.toMap()["index"].toInt();
        return -1;
    }

    void sitesFileLinksAutomatically()
    {
        Sites *s = core->sites();
        const int gpt = siteIndex(s, "chatgpt"), gh = siteIndex(s, "github");
        QVERIFY(gpt >= 0 && gh >= 0); // presets are seeded

        const QString chat = core->addItem(LH::Web, "Chat", "https://chatgpt.com/c/abc");
        const QString repo = core->addItem(LH::Web, "Dockly", "https://github.com/siteowner/dockly");
        const QString bare = core->addItem(LH::Web, "Profile", "https://github.com/siteowner");
        QVERIFY(!chat.isEmpty() && !repo.isEmpty() && !bare.isEmpty());

        // each link landed in its site's group; a link with nothing to group by stays loose
        QCOMPARE(s->viewIds(gpt, {}), QStringList{chat});
        QVERIFY(s->viewIds(gh, {}).contains(repo));
        QVERIFY(!s->viewIds(gh, {}).contains(bare));
        QVERIFY(core->queryIds(LH::ModeCategory, LH::Web, {}, {}).contains(bare));
        QVERIFY(!core->queryIds(LH::ModeCategory, LH::Web, {}, {}).contains(chat));
        QCOMPARE(s->viewIds(gh, "dockly"), QStringList{repo});
        QVERIFY(s->viewIds(gh, "no-such-thing").isEmpty());

        // the group is a site group, so the Collections list hides it
        QStringList names;
        for (const QString &id : core->queryIds(LH::ModeCategory, LH::Collection, {}, {}))
            names << core->item(id).name;
        QVERIFY(!names.contains("ChatGPT"));
        QVERIFY(!names.contains("GitHub · siteowner"));
        QCOMPARE(s->groups(gpt).size(), 1);

        // the dock offers an entry for each site that has links
        bool dockHasChat = false;
        for (const QVariant &v : s->dockEntries())
            dockHasChat |= v.toMap()["cat"].toInt() == LH::SiteViewBase + gpt;
        QVERIFY(dockHasChat);
    }

    void importerPickedGroupIsNotOverridden()
    {
        core->connectors()->addEntriesToNewCollection(
            {QVariantMap{{"name", "Mine"}, {"target", "https://claude.ai/chat/mine1"}}}, "My Claude chats");
        const int claude = siteIndex(core->sites(), "claude");
        const QStringList ids = core->sites()->viewIds(claude, {});
        QCOMPARE(ids.size(), 1); // only in the group the user named, not also in "Claude"
        QCOMPARE(core->sites()->groups(claude).first().toMap()["name"].toString(), QString("My Claude chats"));
    }

    void sitesCanBeDefinedByTheUser()
    {
        Sites *s = core->sites();
        QVERIFY(!s->saveSite({{"name", ""}, {"hosts", "x.example"}}));  // a name is required
        QVERIFY(!s->saveSite({{"name", "X"}, {"hosts", "  "}}));         // and at least one host

        // a host with many loose links is suggested, and accepting it files them
        for (int i = 0; i < 5; ++i)
            core->addItem(LH::Web, {}, QString("https://notes.example/page%1").arg(i));
        bool suggested = false;
        for (const QVariant &v : s->suggestions())
            suggested |= v.toMap()["host"].toString() == "notes.example";
        QVERIFY(suggested);
        QVERIFY(s->acceptSuggestion("notes.example"));
        const int idx = siteIndex(s, s->list().last().toMap()["id"].toString());
        QCOMPARE(s->viewIds(idx, {}).size(), 5);
        QCOMPARE(s->looseCount(), 0);

        // later links on that host are filed on arrival, subdomains included
        const QString extra = core->addItem(LH::Web, {}, "https://www.notes.example/new");
        QVERIFY(s->viewIds(idx, {}).contains(extra));

        // a dismissed host is not suggested again
        for (int i = 0; i < 5; ++i)
            core->addItem(LH::Web, {}, QString("https://dismiss.example/p%1").arg(i));
        s->dismissSuggestion("dismiss.example");
        for (const QVariant &v : s->suggestions())
            QVERIFY(v.toMap()["host"].toString() != "dismiss.example");

        // removing the rule turns its group back into an ordinary collection
        s->removeSite(s->list().at(idx).toMap()["id"].toString());
        QStringList names;
        for (const QString &id : core->queryIds(LH::ModeCategory, LH::Collection, {}, {}))
            names << core->item(id).name;
        QVERIFY(names.contains("Notes"));
    }

    void importersAreSafeToCall()
    {
        Connectors *cn = core->connectors();
        cn->vscodeRecent();             // empty when VS Code isn't installed
        cn->browserBookmarks("nope");   // unknown browser
        QSignalSpy spy(cn, &Connectors::githubReady);
        cn->fetchGithub("not a valid name!");
        QCOMPARE(spy.count(), 1);
        QVERIFY(!spy.first().at(1).toString().isEmpty());
    }
};

QTEST_MAIN(CoreTest)
#include "core_test.moc"
