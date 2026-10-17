#include "appcore.h"
#include "iconpack.h"
#include "itemmodel.h"

#include <QApplication>
#include <QFile>
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
};

QTEST_MAIN(CoreTest)
#include "core_test.moc"
