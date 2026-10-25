#pragma once

#include <QObject>

class AppCore;
class QSystemTrayIcon;

// System tray icon, visible while "Minimize to System Tray" or the edge dock is enabled.
class TrayController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
public:
    explicit TrayController(AppCore *core, QObject *parent = nullptr);

    bool isActive() const;

    // from the dock: open the main window (id empty), or on that item's "target missing" dialog
    Q_INVOKABLE void openMain(const QString &id = QString()) { emit openMainRequested(id); }

signals:
    void activeChanged();
    void showRequested();     // open the main window
    void dockRequested();     // reveal the edge dock
    void openMainRequested(const QString &id);
    void quitRequested();

private:
    AppCore *m_core;
    QSystemTrayIcon *m_icon;
};
