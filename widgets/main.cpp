#include "appcore.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QScreen>
#include <QWindow>
#include <QPainter>
#include <QSurfaceFormat>
#include <QTimer>

int main(int argc, char *argv[])
{
    // OpenGL-backed card views: share one context and ask for 4x multisampling.
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
    QSurfaceFormat fmt;
    fmt.setSamples(4);
    QSurfaceFormat::setDefaultFormat(fmt);

    QApplication app(argc, argv);
    QApplication::setOrganizationName("LauncherHub");
    QApplication::setApplicationName("LauncherHub");
    QApplication::setApplicationVersion(LH_VERSION);
    QApplication::setStyle("Fusion");
    QApplication::setQuitOnLastWindowClosed(false); // the tray keeps the app alive

    QCommandLineParser cli;
    QCommandLineOption minimized("minimized", "Start hidden in the system tray.");
    QCommandLineOption shot("screenshot", "Save a screenshot of the window and exit (diagnostics).", "file");
    QCommandLineOption page("page", "Open this tab index first (diagnostics).", "index");
    cli.addOptions({minimized, shot, page});
    cli.process(app);

    AppCore *core = AppCore::instance();
    core->init();

    MainWindow w(core);
    if (cli.isSet(page))
        w.goTo(cli.value(page).toInt());

    if (cli.isSet(minimized) && w.trayActive())
        ; // stay hidden; the tray icon brings the window back
    else
        w.show();

    if (cli.isSet(shot)) {
        const QString path = cli.value(shot);
        QTimer::singleShot(2500, &app, [&w, path, &app] {
            // Grab the window's real on-screen pixels (QWidget::grab() cannot read OpenGL viewports).
            w.raise();
            w.activateWindow();
            QPixmap shot = w.windowHandle()->screen()->grabWindow(w.winId());
            shot.save(path);
            app.quit();
        });
    }
    return app.exec();
}
