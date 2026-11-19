#include "appcore.h"
#include "softwarescanner.h"
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

    void mainExeIsRanked()
    {
        QCOMPARE(pickMainExe("Notepad++", {"notepad++.exe", "updater.exe", "uninstall.exe"}), QString("notepad++.exe"));
        QCOMPARE(pickMainExe("My App", {"crashhandler.exe", "myapp.exe", "unins000.exe"}), QString("myapp.exe"));
        // exact beats partial; a helper only wins when nothing else matches
        QCOMPARE(pickMainExe("Foo", {"foobar.exe", "foo.exe"}), QString("foo.exe"));
        QCOMPARE(pickMainExe("Foo", {"fooupdater.exe", "foolauncher.exe"}), QString("foolauncher.exe"));
        QCOMPARE(pickMainExe("Foo", {"fooupdater.exe"}), QString("fooupdater.exe"));
        QVERIFY(pickMainExe("Foo", {"bar.exe", "unins000.exe"}).isEmpty());
        QVERIFY(pickMainExe({}, {"foo.exe"}).isEmpty());
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
