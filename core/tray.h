#pragma once

#include <QObject>

class AppCore;
class QSystemTrayIcon;

// System tray icon, visible only while "Minimize to System Tray" is enabled.
class TrayController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive NOTIFY activeChanged)
public:
    explicit TrayController(AppCore *core, QObject *parent = nullptr);

    bool isActive() const;

signals:
    void activeChanged();
    void showRequested();
    void quitRequested();

private:
    AppCore *m_core;
    QSystemTrayIcon *m_icon;
};
