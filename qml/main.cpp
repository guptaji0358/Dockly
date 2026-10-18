#include "appcore.h"
#include "iconimageprovider.h"
#include "itemimageprovider.h"
#include "itemmodel.h"
#include "tray.h"

#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QTimer>
#include <QtQml>

int main(int argc, char *argv[])
{
    // The graphics API is a user setting (Settings > Performance); it has to be known before the
    // first window exists, so read it straight from settings.json. LH_NO_GL=1 forces Qt's default.
    QCoreApplication::setOrganizationName("Robin Gupta Studios");
    QCoreApplication::setApplicationName("Dockly");
    if (qEnvironmentVariableIsEmpty("LH_NO_GL")) {
        QString dir = qEnvironmentVariable("LH_DATA_DIR");
        if (dir.isEmpty())
            dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QFile f(dir + "/settings.json");
        QString api = QStringLiteral("opengl");
        if (f.open(QIODevice::ReadOnly)) {
            const QString v = QJsonDocument::fromJson(f.readAll()).object().value("graphicsApi").toString();
            if (!v.isEmpty())
                api = v;
        }
        if (api == "software") {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
        } else if (api == "d3d11") {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
        } else {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
            QSurfaceFormat fmt;
            fmt.setSamples(4);
            QSurfaceFormat::setDefaultFormat(fmt);
        }
    }

    QApplication app(argc, argv);
    QApplication::setOrganizationName("Robin Gupta Studios");
    QApplication::setApplicationName("Dockly");
    QApplication::setApplicationVersion(LH_VERSION);
    QApplication::setQuitOnLastWindowClosed(false); // the tray keeps the app alive
    QQuickStyle::setStyle("Basic");

    QCommandLineParser cli;
    QCommandLineOption minimized("minimized", "Start hidden in the system tray.");
    QCommandLineOption shot("screenshot", "Save a screenshot of the window and exit (diagnostics).", "file");
    QCommandLineOption page("page", "Open this tab index first (diagnostics).", "index");
    QCommandLineOption ring("ring", "Open the radial menu at startup (diagnostics).");
    QCommandLineOption demo("demo", "Fill an empty library with sample cards (diagnostics).");
    QCommandLineOption dialog("dialog", "Open a dialog at startup: software (diagnostics).", "name");
    cli.addOptions({minimized, shot, page, ring, demo, dialog});
    cli.process(app);

    AppCore *core = AppCore::instance();
    core->init();
    if (cli.isSet(demo) && core->queryIds(LH::ModeCategory, LH::AllCategories, QString(), QString()).isEmpty()) {
        const QString win = qEnvironmentVariable("windir", "C:/Windows");
        const QList<QPair<QString, QString>> soft = {
            {"Notepad", win + "/System32/notepad.exe"}, {"Calculator", win + "/System32/calc.exe"},
            {"Paint", win + "/System32/mspaint.exe"}, {"Command Prompt", win + "/System32/cmd.exe"},
            {"Task Manager", win + "/System32/Taskmgr.exe"}, {"Explorer", win + "/explorer.exe"}};
        for (const auto &s : soft)
            core->addItem(LH::Software, s.first, s.second);
        const QList<QPair<QString, QString>> web = {
            {"GitHub", "https://github.com"}, {"Qt Docs", "https://doc.qt.io"}, {"YouTube", "https://youtube.com"},
            {"Wikipedia", "https://wikipedia.org"}, {"Claude", "https://claude.ai"}};
        for (const auto &w : web)
            core->addItem(LH::Web, w.first, w.second);
        const QStringList softIds = core->queryIds(LH::ModeCategory, LH::Software, QString(), QString());
        for (const QString &id : softIds.mid(0, 4))
            core->setFavorite(id, true);
        const QString dev = core->createCollection("Dev Tools");
        core->addToCollection(dev, softIds.mid(0, 4));
        core->createCollection("Morning");
    }
    TrayController tray(core);
    QObject::connect(&tray, &TrayController::quitRequested, &app, &QApplication::quit);

    qmlRegisterUncreatableMetaObject(LH::staticMetaObject, "LH", 1, 0, "LH", "Enums only");
    qmlRegisterType<ItemModel>("LH", 1, 0, "ItemModel");

    QQmlApplicationEngine engine;
    engine.addImageProvider("item", new ItemImageProvider);
    engine.addImageProvider("icon", new IconImageProvider);
    engine.rootContext()->setContextProperty("core", core);
    engine.rootContext()->setContextProperty("tray", &tray);
    engine.rootContext()->setContextProperty("startRing", cli.isSet(ring));
    engine.rootContext()->setContextProperty("startDialog", cli.value(dialog));
    engine.rootContext()->setContextProperty("startPage", cli.isSet(page) ? cli.value(page).toInt() : -1);
    engine.loadFromModule("DocklyUI", "Main");
    if (engine.rootObjects().isEmpty())
        return 1;

    auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    win->setIcon(AppCore::appIcon());
    QObject::connect(&tray, &TrayController::showRequested, win, [win] {
        win->showNormal();
        win->raise();
        win->requestActivate();
    });

    if (cli.isSet(minimized) && tray.isActive())
        ; // stay hidden; the tray icon brings the window back
    else
        win->show();

    if (cli.isSet(shot)) {
        const QString path = cli.value(shot);
        QTimer::singleShot(2500, &app, [win, path, &app] {
            win->grabWindow().save(path);
            app.quit();
        });
    }
    return app.exec();
}
