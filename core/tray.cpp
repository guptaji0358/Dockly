#include "tray.h"
#include "appcore.h"

#include <QMenu>
#include <QSystemTrayIcon>

TrayController::TrayController(AppCore *core, QObject *parent)
    : QObject(parent), m_core(core), m_icon(new QSystemTrayIcon(AppCore::appIcon(), this))
{
    m_icon->setToolTip(QStringLiteral("Dockly"));

    auto *menu = new QMenu();
    menu->addAction(QStringLiteral("Show dock"), this, &TrayController::dockRequested);
    menu->addAction(QStringLiteral("Manage Dockly..."), this, &TrayController::showRequested);
    menu->addSeparator();
    menu->addAction(QStringLiteral("Quit"), this, &TrayController::quitRequested);
    m_icon->setContextMenu(menu);
    menu->setParent(nullptr);
    connect(this, &QObject::destroyed, menu, &QObject::deleteLater);

    connect(m_icon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger || r == QSystemTrayIcon::DoubleClick) {
            if (m_core->settings()->boolean("dockEnabled"))
                emit dockRequested();
            else
                emit showRequested();
        }
    });

    auto sync = [this] {
        // the dock has no window of its own to find again, so the tray icon must exist while it is on
        const bool on = (m_core->settings()->boolean("minimizeToTray") || m_core->settings()->boolean("dockEnabled"))
                        && QSystemTrayIcon::isSystemTrayAvailable();
        m_icon->setVisible(on);
        emit activeChanged();
    };
    connect(m_core->settings(), &AppSettings::changed, this, [sync](const QString &k) {
        if (k == QLatin1String("minimizeToTray") || k == QLatin1String("dockEnabled"))
            sync();
    });
    sync();
}

bool TrayController::isActive() const
{
    return m_icon->isVisible();
}
