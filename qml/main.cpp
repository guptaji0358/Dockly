#include "appcore.h"
#include "iconimageprovider.h"
#include "itemimageprovider.h"
#include "itemmodel.h"
#include "tray.h"

#include <QApplication>
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
    // Render the scene graph with OpenGL (4x multisampling). LH_NO_GL=1 uses Qt's default backend.
    if (qEnvironmentVariableIsEmpty("LH_NO_GL")) {
        QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
        QSurfaceFormat fmt;
        fmt.setSamples(4);
        QSurfaceFormat::setDefaultFormat(fmt);
    }

    QApplication app(argc, argv);
    QApplication::setOrganizationName("LauncherHub");
    QApplication::setApplicationName("LauncherHub");
    QApplication::setApplicationVersion(LH_VERSION);
    QApplication::setQuitOnLastWindowClosed(false); // the tray keeps the app alive
    QQuickStyle::setStyle("Basic");

    QCommandLineParser cli;
    QCommandLineOption minimized("minimized", "Start hidden in the system tray.");
    QCommandLineOption shot("screenshot", "Save a screenshot of the window and exit (diagnostics).", "file");
    QCommandLineOption page("page", "Open this tab index first (diagnostics).", "index");
    cli.addOptions({minimized, shot, page});
    cli.process(app);

    AppCore *core = AppCore::instance();
    core->init();
    TrayController tray(core);
    QObject::connect(&tray, &TrayController::quitRequested, &app, &QApplication::quit);

    qmlRegisterUncreatableMetaObject(LH::staticMetaObject, "LH", 1, 0, "LH", "Enums only");
    qmlRegisterType<ItemModel>("LH", 1, 0, "ItemModel");

    QQmlApplicationEngine engine;
    engine.addImageProvider("item", new ItemImageProvider);
    engine.addImageProvider("icon", new IconImageProvider);
    engine.rootContext()->setContextProperty("core", core);
    engine.rootContext()->setContextProperty("tray", &tray);
    engine.rootContext()->setContextProperty("startPage", cli.isSet(page) ? cli.value(page).toInt() : -1);
    engine.loadFromModule("LauncherUI", "Main");
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
