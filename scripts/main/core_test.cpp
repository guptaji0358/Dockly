#include "appcore.h"
#include "appsettings.h"
#include "database.h"
#include "softwarescanner.h"
#include "iconpack.h"
#include "procinspect.h"
#include "itemmodel.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QScopeGuard>
#include <QSettings>
#include <QTcpSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QtTest>

class CoreTest : public QObject
{
    Q_OBJECT
    QTemporaryDir m_dir;
    QTemporaryFile m_realFile;
    AppCore *core = nullptr;
    QVariant m_autostart; // the real Windows autostart entry, read before AppCore starts

    static QSettings runKey()
    {
        return QSettings(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                         QSettings::NativeFormat);
    }

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

    void registryValueParsing()
    {
        QCOMPARE(exeFromRegistryValue("\"C:\\Apps\\A\\a.exe\",0"), QString("C:/Apps/A/a.exe"));
        QCOMPARE(exeFromRegistryValue("C:\\Apps\\A\\a.exe,-3"), QString("C:/Apps/A/a.exe"));
        QCOMPARE(exeFromRegistryValue("  C:\\Apps\\A\\a.EXE "), QString("C:/Apps/A/a.EXE"));
        QVERIFY(exeFromRegistryValue("C:\\Apps\\A\\a.ico").isEmpty());
        QVERIFY(exeFromRegistryValue("C:\\Windows\\system32\\shell32.dll,-3").isEmpty());
        QVERIFY(exeFromRegistryValue({}).isEmpty());
    }

    void registryValueExpandsEnvironmentVariables()
    {
        qputenv("DOCKLY_TEST_DIR", "C:\\Apps");
        QCOMPARE(exeFromRegistryValue("%DOCKLY_TEST_DIR%\\A\\a.exe,0"), QString("C:/Apps/A/a.exe"));
        QCOMPARE(exeFromRegistryValue("\"%DOCKLY_TEST_DIR%\\a.exe\""), QString("C:/Apps/a.exe"));
        qputenv("DOCKLY_TEST_LOOP", "%DOCKLY_TEST_LOOP%");
        exeFromRegistryValue("%DOCKLY_TEST_LOOP%\\a.exe"); // must terminate
        qunsetenv("DOCKLY_TEST_DIR");
        qunsetenv("DOCKLY_TEST_LOOP");
    }

    void noiseIsMatchedByWholeWord()
    {
        QVERIFY(looksLikeNoise("Foo Help", "C:/x/foo.exe"));
        QVERIFY(looksLikeNoise("Foo Uninstaller", "C:/x/foo.exe"));
        QVERIFY(looksLikeNoise("Product Setup", "C:/x/foo.exe"));
        QVERIFY(!looksLikeNoise("Helpdesk Pro", "C:/x/foo.exe"));
        QVERIFY(!looksLikeNoise("Manualist", "C:/x/foo.exe"));
        QVERIFY(!looksLikeNoise("Setuptools GUI Studio", "C:/x/foo.exe"));
    }

    void packagedTargetsAreRecognised()
    {
        QVERIFY(isPackagedTarget("shell:AppsFolder\\Microsoft.WindowsCalculator_8wekyb3d8bbwe!App"));
        QVERIFY(isPackagedTarget("SHELL:appsfolder/Foo_123!App")); // after cleanPath-style separators
        QVERIFY(!isPackagedTarget("C:\\Apps\\a.exe"));
        QVERIFY(!isPackagedTarget("shell:AppsFolder\\"));
        QVERIFY(!isPackagedTarget("https://example.com"));
        QCOMPARE(packagedAppId("shell:AppsFolder\\Foo_123!App"), QString("Foo_123!App"));
        QVERIFY(packagedAppId("C:\\Apps\\a.exe").isEmpty());
        QVERIFY(!packagedAppExists("shell:AppsFolder\\Not.A.Real.Package_0000000000000!App"));

        QVector<RawRecord> in;
        in.append({AppSource::Packaged, "Calculator", "shell:AppsFolder\\Microsoft.WindowsCalculator_8wekyb3d8bbwe!App", {}});
        in.append({AppSource::Packaged, "Calculator again", "SHELL:AppsFolder\\microsoft.windowscalculator_8wekyb3d8bbwe!app", {}});
        const QVector<ScannedApp> out = mergeRecords(in, true);
        QCOMPARE(out.size(), 1); // same AUMID in another case is the same program
        QCOMPARE(out[0].target, QString("shell:AppsFolder\\Microsoft.WindowsCalculator_8wekyb3d8bbwe!App"));
        QCOMPARE(out[0].sources, 1 << int(AppSource::Packaged));
        QVERIFY(out[0].installDir.isEmpty() && out[0].publisher.isEmpty());
    }

    void noiseRules()
    {
        QVERIFY(looksLikeNoise("Uninstall Foo", "C:/x/foo.exe"));
        QVERIFY(looksLikeNoise("Foo", "C:/x/unins000.exe"));
        QVERIFY(looksLikeNoise("Foo", "C:/Windows/System32/msiexec.exe"));
        QVERIFY(!looksLikeNoise("Visual Studio Code", "C:/x/Code.exe"));
    }

    void mergeKeepsFirstRecordPerExe()
    {
        QVector<RawRecord> in;
        in.append({AppSource::StartMenu, "Alpha", "C:/Apps/al.exe", {}});
        in.append({AppSource::Uninstall, "Alpha Duplicate", "c:/apps/AL.exe", {}});
        in.append({AppSource::Uninstall, "Uninstall Beta", "C:/Apps/beta.exe", {}});
        RawRecord lazy{AppSource::AppPaths, "gamma", "C:/Apps/gamma.exe", [] { return QString("Gamma Pro"); }};
        in.append(lazy);
        in.append({AppSource::AppPaths, "x", "C:/Apps/x.exe", [] { return QString(); }}); // unresolvable name
        const QVector<ScannedApp> out = mergeRecords(in);
        QCOMPARE(out.size(), 2);
        QCOMPARE(out[0].name, QString("Alpha"));
        QCOMPARE(out[0].target, QDir::toNativeSeparators("C:/Apps/al.exe"));
        QCOMPARE(out[1].name, QString("Gamma Pro"));
    }

    void mergeFillsMissingMetadataFromLaterSources()
    {
        RawRecord a{AppSource::StartMenu, "Alpha", "C:/Apps/al.exe", {}};
        RawRecord b{AppSource::Uninstall, "Alpha Reg", "c:/apps/al.exe", {}};
        b.publisher = " Alpha Corp ";
        b.version = "1.2.3";
        b.installDir = "C:/Apps";
        RawRecord c{AppSource::AppPaths, "alpha", "C:/Apps/al.exe", {}};
        c.publisher = "Someone Else"; // never overrides a value that is already set
        const QVector<ScannedApp> out = mergeRecords({a, b, c});
        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].name, QString("Alpha"));
        QCOMPARE(out[0].publisher, QString("Alpha Corp"));
        QCOMPARE(out[0].version, QString("1.2.3"));
        QCOMPARE(out[0].installDir, QString("C:/Apps"));
        QCOMPARE(out[0].sources, (1 << int(AppSource::StartMenu)) | (1 << int(AppSource::Uninstall)) | (1 << int(AppSource::AppPaths)));
    }

    // ---- synthetic fixtures: no real software, no vendor names that the code could special-case ----------

    void nameRankingIsGeneric()
    {
        // an exe-like name loses to a human-readable one from another source, whatever the vendor
        QCOMPARE(rankNames({{"contosostudio", 20}, {"Contoso Studio", 39}}, "C:/x/contosostudio.exe"), QString("Contoso Studio"));
        QCOMPARE(rankNames({{"Fabrikam Notes", 48}, {"fabrikamnotes.exe", 20}}, "C:/x/fabrikamnotes.exe"), QString("Fabrikam Notes"));
        // only file-name-like candidates: no name at all rather than a wrong one
        QVERIFY(rankNames({{"zz.exe", 20}}, "C:/x/zz.exe").isEmpty());
        // version/arch decoration is stripped from the winner
        QCOMPARE(stripVersionDecoration("Northwind Editor 12.4.1 (x64)"), QString("Northwind Editor"));
        QCOMPARE(stripVersionDecoration("7-Zip 23.01 (x64 edition)"), QString("7-Zip 23.01 (x64 edition)")); // not a pure suffix: untouched
        QCOMPARE(stripVersionDecoration("Windows 11 Helper"), QString("Windows 11 Helper"));
        QCOMPARE(stripVersionDecoration("Qt 6.12.0 (MinGW 64-bit)"), QString("Qt 6.12.0 (MinGW 64-bit)")); // brackets never left half-open
        QCOMPARE(stripVersionDecoration("Tailspin Racer x64"), QString("Tailspin Racer"));
        QCOMPARE(stripVersionDecoration("Wingtip Mail v3.2"), QString("Wingtip Mail"));
        QVERIFY(nameQuality("Proper Name") > nameQuality("proper_name.exe"));
        QVERIFY(nameQuality("{1E87508D-89C2-42F0-8A7E-645A0F50CA58}") < 0);
        // two sources agreeing beat one outlier of the same prior
        QCOMPARE(rankNames({{"Tailspin Toys Racer", 48}, {"Tailspin Toys Racer Pro", 48}, {"Tailspin Toys Racer", 50}}), QString("Tailspin Toys Racer"));
    }

    void registryValuesKeepArgumentsAndRejectUnresolvedVariables()
    {
        QString args;
        QCOMPARE(exeFromRegistryValue("\"C:\\Apps\\A\\a.exe\" --flag \"x y\"", &args), QString("C:/Apps/A/a.exe"));
        QCOMPARE(args, QString("--flag \"x y\""));
        QCOMPARE(exeFromRegistryValue("C:\\Program Files\\Vendor App\\v.exe /silent", &args), QString("C:/Program Files/Vendor App/v.exe"));
        QCOMPARE(args, QString("/silent"));
        QCOMPARE(exeFromRegistryValue("C:\\Apps\\a.exe,0", &args), QString("C:/Apps/a.exe"));
        QVERIFY(args.isEmpty()); // an icon index is not an argument
        qunsetenv("DOCKLY_TEST_UNDEFINED");
        QVERIFY(exeFromRegistryValue("%DOCKLY_TEST_UNDEFINED%\\a.exe").isEmpty()); // never a wrong half-path
        bool ok = true;
        expandEnvironment("%DOCKLY_TEST_UNDEFINED%\\x", &ok);
        QVERIFY(!ok);
        qputenv("DOCKLY_TEST_A", "%DOCKLY_TEST_B%");
        qputenv("DOCKLY_TEST_B", "C:\\Loop");
        QCOMPARE(expandEnvironment("%DOCKLY_TEST_A%", &ok), QString("%DOCKLY_TEST_B%")); // single pass, no recursion
        qunsetenv("DOCKLY_TEST_A");
        qunsetenv("DOCKLY_TEST_B");
    }

    void installersAreFilteredByNameAndLocation()
    {
        for (const char *n : {"C:/x/setup.exe", "C:/x/Setup_v2.exe", "C:/x/vc_redist.x64.exe", "C:/x/unins000.exe",
                              "C:/x/install.exe", "C:/x/foo-installer.exe", "C:/Windows/System32/msiexec.exe",
                              "C:/Windows/Installer/{AB}/icon.exe"})
            QVERIFY2(looksLikeInstallerExe(n), n);
        for (const char *n : {"C:/x/Studio.exe", "C:/x/settings.exe", "C:/x/installed-apps.exe", "C:/x/Notes.exe"})
            QVERIFY2(!looksLikeInstallerExe(n), n);
    }

    void mainExeNeverChosenBySizeAlone()
    {
        QTemporaryDir d;
        QVERIFY(d.isValid());
        auto make = [&](const QString &rel, int size) {
            QDir().mkpath(QFileInfo(d.filePath(rel)).absolutePath());
            QFile f(d.filePath(rel));
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(QByteArray(size, 'x'));
        };
        make("zzbig.exe", 200000);          // largest, but unrelated to the product
        make("setup.exe", 100000);
        make("Aurora Notes.exe", 10);       // small, but named like the product
        make("aurora-updater.exe", 5000);
        make("bin/helper.exe", 90000);
        QCOMPARE(QFileInfo(findMainExeInDir(d.path(), "Aurora Notes")).fileName(), QString("Aurora Notes.exe"));
        QCOMPARE(QFileInfo(findMainExeInDir(d.path(), "Aurora", "Aurora Corp")).fileName(), QString("Aurora Notes.exe")); // partial name beats updater
        QVERIFY(findMainExeInDir(d.path(), "Unrelated Product").isEmpty()); // big files never win without evidence
        QVERIFY(findMainExeInDir(d.path() + "/missing", "Aurora").isEmpty());
        // one level down is searched, top level preferred when equal
        make("tools/Zephyr.exe", 10);
        QCOMPARE(findMainExeInDir(d.path(), "Zephyr"), d.filePath("tools/Zephyr.exe"));
    }

    void mergeReportsRejectionReasons()
    {
        QVector<RawRecord> in;
        in.append({AppSource::StartMenu, "Uninstall Foo", "C:/Apps/foo.exe", {}});
        in.append({AppSource::StartMenu, "Foo", "C:/Apps/unins000.exe", {}});
        in.append({AppSource::AppPaths, "x", "C:/Apps/x.exe", [] { return QString(); }});
        in.append({AppSource::Uninstall, "Good Tool", "", {}});
        in.append({AppSource::StartMenu, "Fine Program", "C:/Apps/fine.exe", {}});
        QVector<Rejection> rej;
        const QVector<ScannedApp> out = mergeRecords(in, false, &rej);
        QCOMPARE(out.size(), 1);
        QCOMPARE(rej.size(), 4);
        QStringList reasons;
        for (const Rejection &r : rej) {
            QVERIFY(!r.reason.isEmpty());
            reasons << r.reason;
        }
        reasons.sort();
        QCOMPARE(reasons, QStringList({"no-launch-target", "noise-name", "noise-target", "unresolved-name"}));
    }

    void argumentsAndNonExeTargetsSurvive()
    {
        QVector<RawRecord> in;
        RawRecord a{AppSource::StartMenu, "Console Tool", "C:/Apps/console.exe", {}};
        RawRecord b{AppSource::StartMenu, "Console Tool Debug", "C:/Apps/console.exe", {}};
        a.args = "--profile work";
        b.args = "--debug";
        RawRecord c{AppSource::StartMenu, "Batch Runner", "C:/Apps/run all.bat", {}};
        in << a << b << c;
        const QVector<ScannedApp> out = mergeRecords(in);
        QCOMPARE(out.size(), 3); // same exe, different arguments = different programs; .bat is not dropped
        QCOMPARE(out[0].args, QString("--profile work"));
        QCOMPARE(out[1].args, QString("--debug"));
        QVERIFY(isLaunchableTarget("C:/Apps/run all.bat") && isLaunchableTarget("x.msc") && isLaunchableTarget("x.lnk"));
        QVERIFY(!isLaunchableTarget("C:/Docs/readme.txt") && !isLaunchableTarget("C:/Docs"));

        // the card keeps the arguments, and they take part in identity
        const QString t = m_realFile.fileName();
        QCOMPARE(core->addSoftware({QVariantMap{{"name", "Args One"}, {"target", t}, {"args", "--one"}}}), 1);
        QCOMPARE(core->addSoftware({QVariantMap{{"name", "Args Two"}, {"target", t}, {"args", "--two"}}}), 1);
        QCOMPARE(core->addSoftware({QVariantMap{{"name", "Args One again"}, {"target", t}, {"args", "--one"}}}), 0);
        bool found = false;
        for (const QString &id : core->queryIds(LH::ModeCategory, LH::Software, {}, {}))
            if (core->item(id).name == "Args One") {
                QCOMPARE(core->item(id).args, QString("--one"));
                found = true;
            }
        QVERIFY(found);
        for (const QString &id : core->queryIds(LH::ModeCategory, LH::Software, {}, {})) // leave no cards behind
            if (core->item(id).name.startsWith("Args "))
                core->removeItem(id);
    }

    void aumidIdentityIsCaseAndSeparatorStable()
    {
        QVector<RawRecord> in;
        in.append({AppSource::AppsFolder, "Stable App", "shell:AppsFolder\\Contoso.Stable_8abc!App", {}});
        in.append({AppSource::Packaged, "Stable App", "SHELL:appsfolder/contoso.stable_8abc!app", {}});
        const QVector<ScannedApp> out = mergeRecords(in);
        QCOMPARE(out.size(), 1);
        QCOMPARE(out[0].sources, (1 << int(AppSource::AppsFolder)) | (1 << int(AppSource::Packaged)));
        QCOMPARE(packagedAppId(out[0].target), QString("Contoso.Stable_8abc!App")); // first spelling is kept
    }

    // ---- duplicates and stale data ------------------------------------------------------------------------

    static QString touch(const QTemporaryDir &d, const QString &rel)
    {
        QDir().mkpath(QFileInfo(d.filePath(rel)).absolutePath());
        QFile f(d.filePath(rel));
        f.open(QIODevice::WriteOnly);
        f.write("x");
        return d.filePath(rel);
    }

    void crossSourceDuplicatesMerge()
    {
        QTemporaryDir d;
        QVERIFY(d.isValid());
        const QString exe = touch(d, "Contoso/Studio/studio.exe");
        const QString exe2 = touch(d, "Contoso/Studio/bin/studio64.exe");
        const QString other = touch(d, "Fabrikam/Studio/studio.exe"); // same name, separate install: must stay
        RawRecord shell{AppSource::AppsFolder, "Contoso Studio", "shell:AppsFolder\\Contoso.Studio", {}};
        RawRecord sm{AppSource::StartMenu, "Contoso Studio", exe, {}};
        RawRecord reg{AppSource::Uninstall, "Contoso Studio", exe2, {}};
        RawRecord paths{AppSource::AppPaths, "Contoso Studio", QString(exe).replace("/Contoso/", "/Contoso/./"), {}}; // other spelling
        RawRecord sep{AppSource::StartMenu, "Contoso Studio", other, {}};
        const QVector<ScannedApp> out = mergeRecords({shell, sm, reg, paths, sep});
        QCOMPARE(out.size(), 2);
        QVERIFY(isPackagedTarget(out[0].target)); // first (primary-source) record survives
        QCOMPARE(out[0].sources, (1 << int(AppSource::AppsFolder)) | (1 << int(AppSource::StartMenu))
                                     | (1 << int(AppSource::Uninstall)) | (1 << int(AppSource::AppPaths)));
        QCOMPARE(out[1].target, QDir::toNativeSeparators(other));
    }

    void registryEntryJoinsArgumentShortcutsButArgsStayDistinct()
    {
        QTemporaryDir d;
        const QString exe = touch(d, "Tool/tool.exe");
        RawRecord a{AppSource::StartMenu, "Tool Work", exe, {}};
        a.args = "--profile work";
        RawRecord b{AppSource::StartMenu, "Tool Debug", exe, {}};
        b.args = "--debug";
        RawRecord reg{AppSource::Uninstall, "Tool Work", exe, {}};
        const QVector<ScannedApp> out = mergeRecords({a, b, reg});
        QCOMPARE(out.size(), 2); // registry record merged, the two argument variants kept apart
        QVERIFY(out[0].sources & (1 << int(AppSource::Uninstall)));
        QCOMPARE(out[1].args, QString("--debug"));
        // a shortcut-backed AUMID pairs with its exe+arguments entry (same name, unique AUMID)
        RawRecord au{AppSource::AppsFolder, "Tool Shell", "shell:AppsFolder\\Vendor.Auto.{1}", {}};
        RawRecord ex{AppSource::StartMenu, "Tool Shell", exe, {}};
        ex.args = "/K setup.bat";
        QCOMPARE(mergeRecords({au, ex}).size(), 1);
        // two packaged apps sharing a name are never merged with an exe
        RawRecord p1{AppSource::AppsFolder, "Photos", "shell:AppsFolder\\A.Photos!App", {}};
        RawRecord p2{AppSource::AppsFolder, "Photos", "shell:AppsFolder\\B.Photos!App", {}};
        RawRecord e{AppSource::Uninstall, "Photos", touch(d, "Other/photos.exe"), {}};
        QCOMPARE(mergeRecords({p1, p2, e}).size(), 3);
    }

    void catalogRefreshReflectsCurrentState()
    {
        QTemporaryDir d;
        const QString v1 = touch(d, "App/app.exe");
        const QString gone = touch(d, "Gone/gone.exe");
        Database db;
        QVERIFY(db.open(d.filePath("data")) || (QDir().mkpath(d.filePath("data")) && db.open(d.filePath("data"))));
        auto scan = [&] {
            QVector<RawRecord> in;
            in.append({AppSource::StartMenu, "App Pro", v1, {}});
            in.append({AppSource::StartMenu, "Gone Tool", gone, {}});
            return mergeRecords(in);
        };
        QCOMPARE(db.replaceCatalog(scan()), 2);              // first scan
        QCOMPARE(db.replaceCatalog(scan()), 2);              // repeated scan: identical, no growth
        QVERIFY(QFile::remove(gone));                        // uninstall
        QCOMPARE(db.pruneCatalog([](const QString &t) { return QFileInfo::exists(t); }), 1); // startup prune
        QCOMPARE(db.catalogCount(), 1);
        QVector<RawRecord> in;                               // update: same program, new publisher/version, rescan
        RawRecord u{AppSource::StartMenu, "App Pro", v1, {}};
        u.version = "2.0";
        in.append(u);
        QCOMPARE(db.replaceCatalog(mergeRecords(in)), 1);
        const QVariantList rows = db.catalog({}, 10);
        QCOMPARE(rows.size(), 1);
        QCOMPARE(rows[0].toMap()["version"].toString(), QString("2.0"));
        QCOMPARE(rows[0].toMap()["name"].toString(), QString("App Pro"));
        db.close();
    }

    void scanFillsCatalogOnlyOnDemand()
    {
        QCOMPARE(core->catalogCount(), 0); // nothing is scanned at startup
        QSignalSpy spy(core, &AppCore::catalogChanged);
        core->refreshSoftware();
        QVERIFY(spy.wait(60000));
        QVERIFY(core->catalogCount() > 0);
        const QVariantList some = core->catalogSearch({}, 5);
        QVERIFY(some.size() > 0);
        for (const QVariant &v : some) { // schema v2 columns are returned and filled where the exe has version info
            const QVariantMap m = v.toMap();
            QVERIFY(m.contains("publisher") && m.contains("version") && m.contains("installDir") && m.contains("sources"));
            QVERIFY(m["sources"].toInt() != 0);
            QVERIFY(!m["installDir"].toString().isEmpty() || isPackagedTarget(m["target"].toString()));
        }
        // Store/MSIX apps (present on any Windows 10/11 install) come through the shell's Apps folder.
        int packaged = 0;
        for (const QVariant &v : core->catalogSearch({}, 5000)) {
            const QString target = v.toMap()["target"].toString();
            if (!isPackagedTarget(target))
                continue;
            if (packaged++ == 0) {
                QVERIFY2(packagedAppExists(target), qPrintable(target));
                QVERIFY2(!packagedAppIcon(target, 48).isNull(), qPrintable(target));
            }
        }
        qInfo() << "packaged apps in catalog:" << packaged;
        QVERIFY(packaged > 0);
        // ...and the scan never creates cards by itself.
        QCOMPARE(core->queryIds(LH::ModeCategory, LH::Software, {}, {}).size(), 0);
    }

    void catalogRefreshNeverTouchesUserCards()
    {
        QTemporaryDir d;
        const QString exe = touch(d, "Keep/keep.exe");
        QCOMPARE(core->addSoftware({QVariantMap{{"name", "My Keep Card"}, {"target", exe}}}), 1);
        const int before = core->queryIds(LH::ModeCategory, LH::Software, {}, {}).size();
        QVERIFY(QFile::remove(exe)); // the program disappears from disk
        core->refreshSoftware();
        QSignalSpy spy(core, &AppCore::catalogChanged);
        QVERIFY(spy.wait(60000));
        const QStringList ids = core->queryIds(LH::ModeCategory, LH::Software, {}, {});
        QCOMPARE(ids.size(), before); // cards survive catalog refresh and uninstall
        QString id;
        for (const QString &i : ids)
            if (core->item(i).name == "My Keep Card")
                id = i;
        QVERIFY(!id.isEmpty());
        core->removeItem(id);
    }

    // ---- real machine: runs after the scan above; asserts invariants, not a nonzero count ----------------

    void realMachineCoverageAndNaming()
    {
        const ScanReport &rep = core->lastScanReport();
        QVERIFY(rep.shellEntries > 0);
        QVERIFY(rep.apps.size() == core->catalogCount());

        // 1. every rejected entry says why
        QHash<QString, int> byReason;
        for (const Rejection &r : rep.rejected) {
            QVERIFY2(!r.reason.isEmpty(), qPrintable(r.name + " | " + r.launch));
            byReason[r.reason]++;
        }
        // 2. coverage of the shell's Apps folder (what Start shows): kept + rejected-with-reason + merged duplicates
        int shellRejected = 0;
        for (const Rejection &r : rep.rejected)
            if (r.source == AppSource::AppsFolder)
                ++shellRejected;
        const int unaccounted = rep.shellEntries - rep.shellEntriesKept - shellRejected;
        qInfo().noquote() << QStringLiteral("COVERAGE shell=%1 kept=%2 rejected=%3 unaccounted(dup/merged)=%4 catalog=%5")
                                 .arg(rep.shellEntries).arg(rep.shellEntriesKept).arg(shellRejected).arg(unaccounted).arg(rep.apps.size());
        QVERIFY2(unaccounted >= 0, "more outcomes than entries");
        QVERIFY2(rep.shellEntriesKept + shellRejected >= rep.shellEntries * 0.9, "over 10% of Apps-folder entries vanished without a reason");

        // 3. naming accuracy: names that still look like file names / GUIDs / versions
        int bad = 0;
        QStringList badNames;
        for (const ScannedApp &a : rep.apps) {
            const QString exe = isPackagedTarget(a.target) ? QString() : a.target;
            if (nameQuality(a.name, exe) <= -50 || a.name.endsWith(".exe", Qt::CaseInsensitive)) { // same bar rankNames uses for "unusable"
                ++bad;
                badNames << a.name + " <- " + a.target;
            }
        }
        const double badPct = rep.apps.isEmpty() ? 100.0 : 100.0 * bad / rep.apps.size();
        qInfo().noquote() << QStringLiteral("NAMING weak=%1 of %2 (%3%)").arg(bad).arg(rep.apps.size()).arg(badPct, 0, 'f', 1);
        for (const QString &n : badNames)
            qInfo().noquote() << "  weak name:" << n;
        QVERIFY2(badPct < 5.0, "too many catalog names still look like file names");

        // 4. no duplicate launch identities, no installer targets, targets resolvable
        QSet<QString> seen;
        int args = 0, nonExe = 0;
        for (const ScannedApp &a : rep.apps) {
            const QString key = Item::normalizedLaunch(a.target, a.args);
            QVERIFY2(!seen.contains(key), qPrintable(a.target));
            seen.insert(key);
            if (!a.args.isEmpty())
                ++args;
            if (!isPackagedTarget(a.target) && !a.target.endsWith(".exe", Qt::CaseInsensitive))
                ++nonExe;
            if (a.target.endsWith(".exe", Qt::CaseInsensitive))
                QVERIFY2(!looksLikeInstallerExe(a.target), qPrintable(a.target));
        }
        qInfo().noquote() << QStringLiteral("SHORTCUT args kept on %1 entries; non-exe launch targets: %2").arg(args).arg(nonExe);
        for (auto it = byReason.constBegin(); it != byReason.constEnd(); ++it)
            qInfo().noquote() << QStringLiteral("REJECTED %1: %2").arg(it.key()).arg(it.value());
        QVERIFY(QFile::exists(m_dir.path() + "/scan-report.txt"));
        if (const QByteArray dump = qgetenv("DOCKLY_SCAN_DUMP"); !dump.isEmpty()) { // manual review of the real catalog
            QFile out(QString::fromLocal8Bit(dump));
            if (out.open(QIODevice::WriteOnly | QIODevice::Text)) {
                for (const ScannedApp &a : rep.apps)
                    out.write((QStringLiteral("%1 | %2 | args=%3 | src=%4").arg(a.name, a.target, a.args).arg(a.sources) + QLatin1Char('\n')).toUtf8());
                out.write("\n");
                out.write(rep.toText().toUtf8());
            }
        }
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
            if (f.open(QIODevice::WriteOnly))
                f.write("x");
            f.close(); // flush before the path is handed to addDropped
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

    void hashListsFiltersNotItems()
    {
        const QString t = core->addItem(LH::Web, "taggedsite", "https://tagged.example");
        core->setItemTags(t, "#Work, client_a  work");
        QCOMPARE(core->item(t).tags, QStringList({"client-a", "work"})); // lower-case, deduped, sorted

        // a bare "#" offers the categories (and tags), never the items themselves
        const QVariantList all = core->quickSearch("#", 50);
        QVERIFY(all.size() >= 7);
        for (const QVariant &v : all)
            QVERIFY(v.toMap()["filter"].toBool());
        QCOMPARE(all[0].toMap()["token"].toString(), QString("#software "));
        QVERIFY(core->quickSearch("#we", 50).size() >= 1);
        QCOMPARE(core->quickSearch("#we", 50)[0].toMap()["token"].toString(), QString("#web "));
        bool sawTag = false;
        for (const QVariant &v : core->quickSearch("#wor", 50))
            sawTag = sawTag || v.toMap()["token"].toString() == "#work ";
        QVERIFY(sawTag);

        // after the space the filter lists items
        const QVariantList web = core->quickSearch("#web ", 200);
        QVERIFY(!web.isEmpty());
        for (const QVariant &v : web)
            QVERIFY(!v.toMap()["filter"].toBool());
        const QVariantList byTag = core->quickSearch("#work ", 200);
        QCOMPARE(byTag.size(), 1);
        QCOMPARE(byTag[0].toMap()["id"].toString(), t);
        QCOMPARE(core->quickSearch("#work tagged", 10).size(), 1);
        QVERIFY(core->quickSearch("#work nothing-like-this", 10).isEmpty());
        QVERIFY(core->quickSearch("#nosuchfilter x", 10).isEmpty());

        // the model filter, the tag list and the backup keep tags
        ItemModel m;
        m.setTag("client-a");
        QCOMPARE(m.count(), 1);
        QVERIFY(core->tagList().toList().size() >= 2);
        QCOMPARE(core->tagList(LH::Document).toList().size(), 0);
        const QString backup = m_dir.path() + "/tags-backup.json";
        QVERIFY(core->backupTo(backup));
        core->setItemTags(t, "");
        QVERIFY(core->item(t).tags.isEmpty());
        QVERIFY(core->restoreFrom(backup));
        QCOMPARE(core->item(t).tags, QStringList({"client-a", "work"}));
    }

    void runningAppsTracksTheTestProcess()
    {
#ifdef Q_OS_WIN
        // this test program owns no window, so it must not be reported - and scanning must be safe
        RunningApps r;
        r.setTargets({{"x", QCoreApplication::applicationFilePath()}});
        r.setEnabled(true);
        r.refresh();
        QVERIFY(!r.isRunning("x"));
        QVERIFY(!r.focus("x"));
        r.setEnabled(false);
        QVERIFY(!r.isRunning("x"));
#endif
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

    void backgroundGuardSwitchDefaultsOnAndSurvivesBackup()
    {
        const QString id = core->addItem(LH::Software, "guard test", m_realFile.fileName());
        QVERIFY(!id.isEmpty());
        QVERIFY(core->item(id).guard); // new cards start with the guard on
        QCOMPARE(core->itemInfo(id).value("guardState").toString(), QString("notEnforced")); // never claims protection yet

        core->setGuard(id, false);
        QVERIFY(!core->item(id).guard);
        QCOMPARE(core->itemInfo(id).value("guardState").toString(), QString("off"));

        const QString path = m_dir.path() + "/guard-backup.json";
        QVERIFY(core->backupTo(path));
        core->setGuard(id, true);
        QVERIFY(core->restoreFrom(path));
        QVERIFY(!core->item(id).guard); // the saved off state is restored
        core->setGuard(id, true);

        core->settings()->set("guardEnabled", false); // master switch overrides the card
        QCOMPARE(core->itemInfo(id).value("guardState").toString(), QString("off"));
        core->settings()->set("guardEnabled", true);

        // older backups carry no guard key: on; only program cards have the switch
        QVERIFY(Item::fromJson(QJsonObject{{"id", "old"}}).guard);
        const QString web = core->addItem(LH::Web, "guard web", "https://guard.example");
        core->setGuard(web, false);
        QVERIFY(core->item(web).guard);
        QCOMPARE(core->itemInfo(web).value("guardState").toString(), QString("none"));
    }

    // Real migration path: a library.db made by the previous schema (no guard column) is opened by Database::open.
    void guardColumnMigrationKeepsExistingData()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString conn = "guard_migration_old_schema";
        {
            QSqlDatabase old = QSqlDatabase::addDatabase("QSQLITE", conn);
            old.setDatabaseName(dir.path() + "/library.db");
            QVERIFY(old.open());
            QSqlQuery q(old);
            QVERIFY(q.exec("CREATE TABLE items (id TEXT PRIMARY KEY, category INTEGER NOT NULL, name TEXT NOT NULL,"
                           " target TEXT NOT NULL DEFAULT '', norm TEXT NOT NULL DEFAULT '', favorite INTEGER NOT NULL DEFAULT 0,"
                           " last_opened TEXT, status INTEGER NOT NULL DEFAULT 0,"
                           " icon TEXT NOT NULL DEFAULT '', tags TEXT NOT NULL DEFAULT '', args TEXT NOT NULL DEFAULT '')"));
            QVERIFY(q.exec("CREATE TABLE members (collection_id TEXT NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
                           " item_id TEXT NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
                           " pos INTEGER NOT NULL, PRIMARY KEY(collection_id, item_id))"));
            QVERIFY(q.exec("INSERT INTO items(id,category,name,target,norm,favorite,tags,args) VALUES"
                           "('sw1',0,'Old Program','C:/Apps/old.exe','c:/apps/old.exe',1,'work,tools','--fast'),"
                           "('sw2',0,'Second Program','C:/Apps/two.exe','c:/apps/two.exe',0,'',''),"
                           "('web1',3,'Old Site','https://old.example','https://old.example',0,'',''),"
                           "('doc1',1,'Old Doc','C:/Docs/a.pdf','c:/docs/a.pdf',0,'',''),"
                           "('col1',4,'Old Collection','','',0,'','')"));
            QVERIFY(q.exec("INSERT INTO members(collection_id,item_id,pos) VALUES ('col1','sw1',0),('col1','web1',1)"));
            QVERIFY(!q.exec("SELECT guard FROM items")); // really the old schema
        }
        QSqlDatabase::removeDatabase(conn);

        auto verifyData = [](Database &db) {
            QCOMPARE(db.itemCount(), 5);
            Item a;
            QVERIFY(db.getItem("sw1", a));
            QCOMPARE(a.name, QString("Old Program"));
            QCOMPARE(a.target, QString("C:/Apps/old.exe"));
            QCOMPARE(a.args, QString("--fast"));
            QVERIFY(a.favorite);
            QCOMPARE(a.tags, QStringList({"tools", "work"})); // tags are read back sorted
            Item w;
            QVERIFY(db.getItem("web1", w));
            QCOMPARE(w.name, QString("Old Site"));
            QCOMPARE(w.target, QString("https://old.example"));
            QCOMPARE(db.members("col1"), QStringList({"sw1", "web1"}));
        };

        {
            Database db;
            QVERIFY(db.open(dir.path())); // runs the real migration
            QVERIFY(!db.libraryIsNew());
            Item sw1, sw2;
            QVERIFY(db.getItem("sw1", sw1));
            QVERIFY(db.getItem("sw2", sw2));
            QVERIFY(sw1.guard); // existing Software cards get guard = 1
            QVERIFY(sw2.guard);
            verifyData(db);

            // a card the user switches off must keep that choice
            db.setGuard("sw2", false);
            // new rows after migration also work
            Item added;
            added.id = "new1";
            added.category = LH::Software;
            added.name = "New Program";
            added.target = "C:/Apps/new.exe";
            QVERIFY(db.insertItem(added));
            Item got;
            QVERIFY(db.getItem("new1", got));
            QVERIFY(got.guard);
        }
        // repeated migration: reopen twice; nothing fails, nothing is reset or corrupted
        for (int round = 0; round < 2; ++round) {
            Database db;
            QVERIFY(db.open(dir.path()));
            QCOMPARE(db.itemCount(), 6);
            Item sw1, sw2;
            QVERIFY(db.getItem("sw1", sw1));
            QVERIFY(db.getItem("sw2", sw2));
            QVERIFY(sw1.guard);
            QVERIFY(!sw2.guard);
            Item a;
            QVERIFY(db.getItem("sw1", a));
            QCOMPARE(a.args, QString("--fast"));
            QCOMPARE(db.members("col1"), QStringList({"sw1", "web1"}));
        }
        {
            const QString c2 = "guard_migration_check";
            {
                QSqlDatabase chk = QSqlDatabase::addDatabase("QSQLITE", c2);
                chk.setDatabaseName(dir.path() + "/library.db");
                QVERIFY(chk.open());
                QSqlQuery q(chk);
                QVERIFY(q.exec("PRAGMA integrity_check") && q.next());
                QCOMPARE(q.value(0).toString(), QString("ok"));
                QVERIFY(q.exec("SELECT COUNT(*) FROM pragma_table_info('items') WHERE name='guard'") && q.next());
                QCOMPARE(q.value(0).toInt(), 1); // the column was added exactly once
            }
            QSqlDatabase::removeDatabase(c2);
        }
    }

    void guardMasterSwitchPersists()
    {
        QCOMPARE(AppSettings::defaults().value("guardEnabled").toBool(), true); // fresh installation
        QCOMPARE(AppSettings::defaults().value("guardEnforce").toBool(), false);
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.path() + "/settings.json";

        AppSettings fresh;
        QVERIFY(fresh.boolean("guardEnabled"));
        QVERIFY(!AppSettings().load(path)); // nothing saved yet: a fresh install keeps the default

        for (bool value : {false, true, false}) {
            AppSettings a;
            a.set("guardEnabled", value);
            QVERIFY(a.save(path));
            AppSettings b; // a new object, as after a restart
            QVERIFY(b.load(path));
            QCOMPARE(b.boolean("guardEnabled"), value);
        }
        // a settings file from before the switch existed falls back to the default (on)
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly | QIODevice::Truncate));
        f.write("{\"theme\":\"light\"}");
        f.close();
        AppSettings old;
        QVERIFY(old.load(path));
        QVERIFY(old.boolean("guardEnabled"));
    }

    void guardCardStatesAreIndependentOfMasterAndBackedUp()
    {
        QFile one(m_dir.path() + "/indep-on.exe"), two(m_dir.path() + "/indep-off.exe");
        QVERIFY(one.open(QIODevice::WriteOnly) && two.open(QIODevice::WriteOnly));
        one.close();
        two.close();
        const QString on = core->addItem(LH::Software, "guard indep on", one.fileName());
        const QString off = core->addItem(LH::Software, "guard indep off", two.fileName());
        QVERIFY(!on.isEmpty());
        QVERIFY(!off.isEmpty());
        QVERIFY(on != off);
        QVERIFY(core->item(on).guard);
        core->setGuard(off, false);

        core->settings()->set("guardEnabled", false); // master switch must not rewrite the cards
        QVERIFY(core->item(on).guard);
        QVERIFY(!core->item(off).guard);
        core->settings()->set("guardEnabled", true);
        QVERIFY(core->item(on).guard);
        QVERIFY(!core->item(off).guard);

        const QString path = m_dir.path() + "/guard-indep-backup.json";
        QVERIFY(core->backupTo(path));
        core->setGuard(on, false);
        core->setGuard(off, true);
        QVERIFY(core->restoreFrom(path));
        QVERIFY(core->item(on).guard);
        QVERIFY(!core->item(off).guard);

        // only Software cards have a switch: a Document stays on and reports no guard state
        const QString doc = core->addItem(LH::Document, "guard doc", m_realFile.fileName());
        core->setGuard(doc, false);
        QVERIFY(core->item(doc).guard);
        QCOMPARE(core->itemInfo(doc).value("guardState").toString(), QString("none"));
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

    // ---- Stage 2: read-only process attribution (synthetic records) ----
    static ProcInspect::ProcessRecord P(quint32 pid, quint32 ppid, quint64 created, const QString &path, bool win = false,
                                        const QString &pkg = {})
    {
        ProcInspect::ProcessRecord r;
        r.pid = pid; r.ppid = ppid; r.created = created; r.exePath = path; r.hasWindow = win; r.packageFamily = pkg;
        r.name = path.section('\\', -1);
        return r;
    }
    static const ProcInspect::Verdict &V(const ProcInspect::Inspection &i, quint32 pid)
    {
        for (const auto &v : i.verdicts) if (v.proc.pid == pid) return v;
        static ProcInspect::Verdict none; return none;
    }
    static std::function<QString(const QString &)> pub(const QString &p)
    {
        return [p](const QString &) { return p; };
    }

    void procMultipleExesOfOneApp()
    {
        using namespace ProcInspect;
        CardRef c{"a", "Acme", "D:\\Acme\\acme.exe", "D:\\Acme", "Acme Inc", {}};
        auto r = attribute({P(10, 4, 100, "D:\\Acme\\acme.exe", true), P(11, 10, 110, "D:\\Acme\\helper.exe"),
                            P(12, 10, 120, "D:\\Acme\\render.exe", true), P(13, 4, 90, "D:\\Other\\acme.exe")}, {c}, pub("Acme Inc"));
        QCOMPARE(V(r, 10).cls, Class::Main);
        QCOMPARE(V(r, 11).cls, Class::Unknown);              // child of the live Main: not assumed unnecessary
        QVERIFY(V(r, 11).reason.contains("descendant"));
        QCOMPARE(V(r, 12).cls, Class::Unknown);              // window alone does not make it Essential
        QCOMPARE(V(r, 12).cardId, QString("a"));             // ...but it is still attributed to the card
        QCOMPARE(V(r, 11).cardId, QString("a"));
        // a same-card process that is NOT below Main (detached leftover) is the only kind that becomes a candidate
        auto rd = attribute({P(10, 4, 100, "D:\\Acme\\acme.exe", true), P(14, 4, 95, "D:\\Acme\\sync.exe")}, {c}, pub("Acme Inc"));
        QCOMPARE(V(rd, 14).cls, Class::CandidateBackground);
        QCOMPARE(V(r, 13).cls, Class::Unknown); // same file name elsewhere: filename alone is not evidence
        QVERIFY(V(r, 13).cardId.isEmpty());
    }

    void procInstallDirAloneIsNotEnough()
    {
        using namespace ProcInspect;
        CardRef c{"a", "Acme", "D:\\Acme\\acme.exe", "D:\\Acme", "Acme Inc", {}};
        auto r = attribute({P(20, 1, 100, "D:\\Acme\\tool.exe")}, {c}, pub("Someone Else"));
        QCOMPARE(V(r, 20).cls, Class::Unknown);
        QVERIFY(!V(r, 20).reason.isEmpty());
    }

    void procSharedHostIsAmbiguous()
    {
        using namespace ProcInspect;
        CardRef a{"a", "Suite Writer", "D:\\Suite\\writer.exe", "D:\\Suite", "Suite Co", {}};
        CardRef b{"b", "Suite Sheet", "D:\\Suite\\sheet.exe", "D:\\Suite", "Suite Co", {}};
        auto r = attribute({P(30, 1, 100, "D:\\Suite\\shared_host.exe")}, {a, b}, pub("Suite Co"));
        QCOMPARE(V(r, 30).cls, Class::Unknown);
        QCOMPARE(V(r, 30).ambiguousCards.size(), 2);
        QVERIFY(V(r, 30).reason.contains("Ambiguous"));
        auto r2 = attribute({P(31, 1, 100, "C:\\Program Files\\Common Files\\host.exe")}, {a, b}, pub("Suite Co"));
        QCOMPARE(V(r2, 31).cls, Class::Unknown);
        auto r3 = attribute({P(32, 1, 100, "D:\\Suite\\writer.exe", true)}, {a, b}, pub("Suite Co"));
        QCOMPARE(V(r3, 32).cardId, QString("a"));
        QCOMPARE(V(r3, 32).cls, Class::Main);
    }

    void procDetachedProcessKeepsOwnership()
    {
        using namespace ProcInspect;
        CardRef c{"a", "Acme", "D:\\Acme\\acme.exe", "D:\\Acme", "Acme Inc", {}};
        auto r = attribute({P(40, 9999, 100, "D:\\Acme\\updater.exe")}, {c}, pub("Acme Inc"));
        QVERIFY(!V(r, 40).parentValid);
        QCOMPARE(V(r, 40).cls, Class::CandidateBackground);
        QVERIFY(V(r, 40).evidence.join(" ").contains("Detached"));
    }

    void procPackagedAppUsesPackageIdentity()
    {
        using namespace ProcInspect;
        QCOMPARE(packageFamilyOfAumid("Contoso.Calc_8wekyb3d8bbwe!App"), QString("Contoso.Calc_8wekyb3d8bbwe"));
        CardRef c;
        c.id = "p"; c.name = "Calc"; c.packageFamily = "Contoso.Calc_8wekyb3d8bbwe";
        auto r = attribute({P(50, 1, 100, "C:\\WindowsApps\\Contoso\\Calc.exe", true, "contoso.calc_8wekyb3d8bbwe"),
                            P(51, 1, 110, "C:\\WindowsApps\\Contoso\\Bg.exe", false, "Contoso.Calc_8wekyb3d8bbwe"),
                            P(53, 50, 120, "C:\\WindowsApps\\Contoso\\Child.exe", false, "Contoso.Calc_8wekyb3d8bbwe"),
                            P(52, 1, 100, "C:\\WindowsApps\\Other\\Calc.exe", true, "Other.Thing_abc")}, {c});
        QCOMPARE(V(r, 50).cls, Class::Main);
        QCOMPARE(V(r, 51).cls, Class::CandidateBackground);
        QCOMPARE(V(r, 53).cls, Class::Unknown);               // child of Main
        QCOMPARE(V(r, 52).cls, Class::Unknown);
    }

    void procPidReuseIsDetectedByCreationTime()
    {
        using namespace ProcInspect;
        CardRef c{"a", "Acme", "D:\\Acme\\acme.exe", "D:\\Acme", "Acme Inc", {}};
        auto r = attribute({P(10, 1, 500, "D:\\Acme\\acme.exe", true), P(11, 10, 100, "D:\\Acme\\helper.exe")}, {c}, pub("Acme Inc"));
        QVERIFY(!V(r, 11).parentValid);
        QCOMPARE(V(r, 11).cls, Class::CandidateBackground);
        const quint32 me = quint32(QCoreApplication::applicationPid());
        const quint64 t = creationTimeOf(me);
        QVERIFY(t != 0);
        QVERIFY(stillSameProcess(me, t));
        QVERIFY(!stillSameProcess(me, t + 12345));
    }

    void procServicesAndSharedFolders()
    {
        using namespace ProcInspect;
        CardRef c{"a", "Acme", "D:\\Acme\\acme.exe", "D:\\Acme", "Acme Inc", {}};
        auto r = attribute({P(60, 1, 10, "C:\\Windows\\System32\\services.exe"), P(61, 60, 100, "D:\\Acme\\svc.exe")}, {c}, pub("Acme Inc"));
        QCOMPARE(V(r, 61).cls, Class::Unknown);              // service child alone: neither essential nor candidate
        QCOMPARE(V(r, 61).cardId, QString("a"));
        QVERIFY(isSharedLocation("C:\\Windows\\System32"));
        QVERIFY(isSharedLocation("C:\\"));
        QVERIFY(!isSharedLocation("D:\\Acme"));
        CardRef cmd = makeCardRef("x", "Command Prompt", "C:\\Windows\\System32\\cmd.exe");
        QVERIFY(cmd.installDir.isEmpty());
    }

    void procEssentialNeedsDependencyEvidence()
    {
        using namespace ProcInspect;
        CardRef c{"a", "Acme", "D:\\Acme\\acme.exe", "D:\\Acme", "Acme Inc", {}};
        // launcher (no window) started the Main process: the launcher is Essential; its sibling helper is a candidate
        auto r = attribute({P(70, 1, 100, "D:\\Acme\\launcher.exe"), P(71, 70, 110, "D:\\Acme\\acme.exe", true),
                            P(72, 1, 105, "D:\\Acme\\sync.exe")}, {c}, pub("Acme Inc"));
        QCOMPARE(V(r, 71).cls, Class::Main);
        QCOMPARE(V(r, 70).cls, Class::Essential);
        QCOMPARE(V(r, 72).cls, Class::CandidateBackground);
        // an ancestor of a non-Main window owner is not a candidate (something visible hangs below it)
        auto r2 = attribute({P(80, 1, 100, "D:\\Acme\\host.exe"), P(81, 80, 110, "D:\\Acme\\view.exe", true)}, {c}, pub("Acme Inc"));
        QCOMPARE(V(r2, 80).cls, Class::Unknown);
        QCOMPARE(V(r2, 81).cls, Class::Unknown);
        // packaged app without a windowed process: nothing to anchor on, everything stays Unknown
        CardRef p; p.id = "p"; p.name = "Pkg"; p.packageFamily = "X.Y_abc";
        auto r3 = attribute({P(90, 1, 100, "C:\\WindowsApps\\X\\bg.exe", false, "X.Y_abc")}, {p});
        QCOMPARE(V(r3, 90).cardId, QString("p"));
        QCOMPARE(V(r3, 90).cls, Class::Unknown);
    }

    void procRealPackagedApps()
    {
        using namespace ProcInspect;
        const ScanReport rep = scanInstalledSoftwareReport();
        QVector<CardRef> cards;
        for (const ScannedApp &a : rep.apps)
            if (isPackagedTarget(a.target))
                cards << makeCardRef(a.target, a.name, a.target);
        const auto procs = collectProcesses();
        const auto r = attribute(procs, cards);
        int pkgProcs = 0, attributed = 0, noWindow = 0;
        QSet<QString> cardsRunning;
        for (const auto &v : r.verdicts) {
            if (v.proc.packageFamily.isEmpty())
                continue;
            ++pkgProcs;
            if (!v.cardId.isEmpty()) {
                ++attributed;
                cardsRunning.insert(v.cardId);
                // the family on the process must equal the family of the card it was attributed to
                for (const CardRef &c : cards)
                    if (c.id == v.cardId)
                        QCOMPARE(c.packageFamily.toLower(), v.proc.packageFamily.toLower());
            }
            qInfo().noquote() << "PKG" << v.proc.pid << v.proc.name << v.proc.packageFamily << "->"
                              << (v.cardName.isEmpty() ? QString("(no card)") : v.cardName) << classLabel(v.cls)
                              << (v.proc.hasWindow ? "window" : "no-window");
        }
        for (const QString &cid : cardsRunning) {
            bool win = false;
            QString nm;
            for (const auto &v : r.verdicts)
                if (v.cardId == cid) { nm = v.cardName; win = win || v.proc.hasWindow; }
            if (!win)
                ++noWindow, qInfo().noquote() << "PKG-NOWINDOW" << nm << "has processes but no visible window";
        }
        qInfo() << "packaged cards" << cards.size() << "packaged processes" << pkgProcs << "attributed" << attributed
                << "cards running" << cardsRunning.size() << "running without window" << noWindow;
    }

    void procRealMachineDiscovery()
    {
        using namespace ProcInspect;
        QString err;
        QElapsedTimer t;
        t.start();
        const auto procs = collectProcesses(&err);
        QVERIFY2(procs.size() > 10, qPrintable(err));
        const quint32 me = quint32(QCoreApplication::applicationPid());
        bool found = false;
        for (const auto &p : procs)
            if (p.pid == me) { found = true; QVERIFY(!p.exePath.isEmpty()); QVERIFY(p.created != 0); }
        QVERIFY(found);
        CardRef self = makeCardRef("me", "core_test", QCoreApplication::applicationFilePath());
        const auto r = inspectNow({self});
        qInfo() << "processes" << r.processCount << "in" << t.elapsed() << "ms";
        QCOMPARE(V(r, me).cardId, QString("me"));
        QCOMPARE(V(r, me).cls, Class::Main);
        QVERIFY2(t.elapsed() < 3000, "process inspection should take well under 3 s");
    }
};

QTEST_MAIN(CoreTest)
#include "core_test.moc"
