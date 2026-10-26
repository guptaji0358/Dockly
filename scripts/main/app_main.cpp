#include "appcore.h"
#include "iconimageprovider.h"
#include "itemimageprovider.h"
#include "hotkey.h"
#include "itemmodel.h"
#include "tray.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMessageBox>
#include <QMouseEvent>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QCommandLineParser>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QScreen>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QTimer>
#include <QtQml>
#include <qpa/qwindowsysteminterface.h>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
QString dataDirPath()
{
    const QString env = qEnvironmentVariable("LH_DATA_DIR");
    return env.isEmpty() ? QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) : env;
}

// Everything Qt / QML prints is also kept in <data folder>/dockly.log, because a GUI .exe has no console.
QFile *g_log = nullptr;
void logHandler(QtMsgType, const QMessageLogContext &, const QString &msg)
{
    if (g_log && g_log->isOpen()) {
        g_log->write(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss  ").toUtf8() + msg.toUtf8() + '\n');
        g_log->flush();
    }
    fprintf(stderr, "%s\n", msg.toLocal8Bit().constData());
}
// Show the window and really put it in front. Windows refuses to let a freshly started (or tray-hidden)
// program steal focus from the app you are working in, so a plain show() can leave Dockly behind a
// maximised window with only a blinking taskbar button - which looks like "nothing happened".
void bringToFront(QQuickWindow *win)
{
    win->showNormal();
    win->raise();
    win->requestActivate();
#ifdef Q_OS_WIN
    const HWND h = reinterpret_cast<HWND>(win->winId());
    if (IsIconic(h))
        ShowWindow(h, SW_RESTORE);
    // topmost for an instant puts the window above everything without waiting for permission ...
    SetWindowPos(h, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetWindowPos(h, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    // ... and attaching to the foreground thread lets us take keyboard focus as well
    const DWORD fg = GetWindowThreadProcessId(GetForegroundWindow(), nullptr);
    const DWORD me = GetCurrentThreadId();
    if (fg && fg != me)
        AttachThreadInput(me, fg, TRUE);
    SetForegroundWindow(h);
    BringWindowToTop(h);
    if (fg && fg != me)
        AttachThreadInput(me, fg, FALSE);
#endif
    qInfo() << "window brought to front, visible =" << win->isVisible();
}
} // namespace

extern "C" __declspec(dllexport) int dockly_main(int argc, char *argv[])
{
    // The graphics API is a user setting (Settings > Performance); it has to be known before the
    // first window exists, so read it straight from settings.json. LH_NO_GL=1 forces Qt's default.
    QCoreApplication::setOrganizationName("Robin Gupta Studios");
    QCoreApplication::setApplicationName("Dockly");
    if (qEnvironmentVariableIsEmpty("LH_NO_GL")) {
        QFile f(dataDirPath() + "/settings.json");
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
    QCommandLineOption dbl("dblclick", "Send a double-click at x,y (window coordinates) after startup (diagnostics).", "x,y");
    QCommandLineOption window("window", "Open the main window even when the edge dock is enabled.");
    QCommandLineOption dockShot("dockshot", "Save a screenshot of the opened dock and exit (diagnostics).", "file");
    cli.addOptions({minimized, shot, page, ring, demo, dialog, dbl, window, dockShot});
    cli.process(app);

    // One Dockly per data folder: starting it again brings the running one to the front instead of
    // opening a second copy (which is easy to mistake for "nothing happened" when it lives in the tray).
    const QString dir = dataDirPath();
    QDir().mkpath(dir);
    const QString instanceKey = "Dockly-" + QString::fromLatin1(QCryptographicHash::hash(dir.toUtf8(), QCryptographicHash::Md5).toHex().left(12));
    if (!cli.isSet(shot)) {
        QLocalSocket probe;
        probe.connectToServer(instanceKey);
        if (probe.waitForConnected(250)) {
            probe.write("show\n");
            probe.waitForBytesWritten(500);
            if (probe.waitForReadyRead(1500) && !cli.isSet(minimized))
                return 0; // the running instance answered and is showing its window
            if (probe.state() == QLocalSocket::ConnectedState && cli.isSet(minimized))
                return 0; // autostart while already running: nothing to do
        }
    }
    QLocalServer instanceServer;
    QLocalServer::removeServer(instanceKey);
    instanceServer.listen(instanceKey);

    QFile logFile(dir + "/dockly.log");
    if (logFile.size() > 512 * 1024)
        logFile.remove();
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        g_log = &logFile;
        qInstallMessageHandler(logHandler);
    }
    qInfo() << "Dockly" << LH_VERSION << "starting, data folder" << dir;

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
    // the dock is a frameless window of its own; it is not created for the window screenshot diagnostics
    const bool dockWanted = core->settings()->boolean("dockEnabled") && !cli.isSet(shot);
    // the dock lives on the primary screen, inside the area the taskbar leaves free
    QScreen *primary = QGuiApplication::primaryScreen();
    engine.rootContext()->setContextProperty("workArea", QRectF(primary->availableGeometry()));
    QObject::connect(primary, &QScreen::availableGeometryChanged, &engine, [&engine](const QRect &r) {
        engine.rootContext()->setContextProperty("workArea", QRectF(r));
    });
    engine.rootContext()->setContextProperty("dockAllowed", !cli.isSet(shot));
    engine.rootContext()->setContextProperty("dockPinned", cli.isSet(dockShot));
    engine.loadFromModule("DocklyUI", "Main");
    if (!engine.rootObjects().isEmpty())
        engine.loadFromModule("DocklyUI", "DockWindow");
    if (engine.rootObjects().size() < 2) {
        qCritical() << "The interface failed to load.";
        QMessageBox::critical(nullptr, "Dockly",
                              "Dockly couldn't load its interface.\n\nDetails were written to:\n"
                              + QDir::toNativeSeparators(dir + "/dockly.log"));
        return 1;
    }

    auto *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    win->setIcon(AppCore::appIcon());
    auto *dockWin = qobject_cast<QQuickWindow *>(engine.rootObjects().at(1));
    auto revealDock = [dockWin] { QMetaObject::invokeMethod(dockWin, "reveal"); };
    QObject::connect(&tray, &TrayController::showRequested, win, [win] { bringToFront(win); });
    QObject::connect(&tray, &TrayController::dockRequested, dockWin, revealDock);
    // the gear (no id) opens the full window; a tile whose target is gone opens it on that item's recovery dialog
    QObject::connect(&tray, &TrayController::editMainRequested, win, [win](const QString &id) {
        bringToFront(win);
        QMetaObject::invokeMethod(win, "edit", Q_ARG(QVariant, id));
    });
    GlobalHotkey hotkey;
    auto applyHotkey = [&hotkey, core] {
        hotkey.setSequence(core->settings()->boolean("dockEnabled") ? core->settings()->text("dockHotkey") : QString());
    };
    QObject::connect(&hotkey, &GlobalHotkey::triggered, dockWin, [dockWin] { QMetaObject::invokeMethod(dockWin, "toggle"); });
    QObject::connect(core->settings(), &AppSettings::changed, &app, [applyHotkey](const QString &k) {
        if (k == QLatin1String("dockHotkey") || k == QLatin1String("dockEnabled"))
            applyHotkey();
    });
    applyHotkey();
    QObject::connect(&tray, &TrayController::openMainRequested, win, [win](const QString &id) {
        bringToFront(win);
        if (!id.isEmpty())
            QMetaObject::invokeMethod(win, "launch", Q_ARG(QVariant, id));
    });

    QObject::connect(&instanceServer, &QLocalServer::newConnection, win, [&instanceServer, win, core, revealDock] {
        while (QLocalSocket *c = instanceServer.nextPendingConnection()) {
            c->write("ok\n");
            c->flush();
            c->disconnectFromServer();
            c->deleteLater();
            if (core->settings()->boolean("dockEnabled"))
                revealDock();
            else
                bringToFront(win);
        }
    });

    const bool diagnostics = cli.isSet(page) || cli.isSet(ring) || cli.isSet(dialog) || cli.isSet(window);
    if (cli.isSet(dockShot)) {
        const QString path = cli.value(dockShot);
        QTimer::singleShot(2000, &app, [dockWin, path, &app] {
            dockWin->grabWindow().save(path);
            app.quit();
        });
    } else if (dockWanted && !diagnostics)
        ; // slim dock only; the tray icon or the dock's gear opens the full window
    else if (cli.isSet(minimized) && tray.isActive())
        ; // stay hidden; the tray icon brings the window back
    else if (cli.isSet(shot))
        win->show();
    else
        bringToFront(win);

    if (cli.isSet(dbl)) {
        const QStringList xy = cli.value(dbl).split(',');
        const QPointF pos(xy.value(0).toDouble(), xy.value(1).toDouble());
        QTimer::singleShot(1500, &app, [win, pos] {
            // goes through the same path as real input, including Qt's own double-click detection
            QWindowSystemInterface::setSynchronousWindowSystemEvents(true);
            // the window system interface works in native (device) pixels
            const qreal dpr = win->devicePixelRatio();
            const QPointF local = pos * dpr;
            const QPointF global = QPointF(win->mapToGlobal(pos.toPoint())) * dpr;
            auto mouse = [&](QEvent::Type t, Qt::MouseButtons state, Qt::MouseButton b) {
                QWindowSystemInterface::handleMouseEvent(win, local, global, state, b, t);
            };
            mouse(QEvent::MouseMove, Qt::NoButton, Qt::NoButton);
            for (int i = 0; i < 2; ++i) {
                mouse(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
                mouse(QEvent::MouseButtonRelease, Qt::NoButton, Qt::LeftButton);
            }
        });
    }

    if (cli.isSet(shot)) {
        const QString path = cli.value(shot);
        QTimer::singleShot(2500, &app, [win, path, &app] {
            win->grabWindow().save(path);
            app.quit();
        });
    }
    return app.exec();
}
