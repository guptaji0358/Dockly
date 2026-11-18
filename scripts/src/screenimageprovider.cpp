#include "screenimageprovider.h"

#include <QGuiApplication>
#include <QScreen>

ScreenImageProvider::ScreenImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap ScreenImageProvider::requestPixmap(const QString &, QSize *size, const QSize &)
{
    QScreen *screen = QGuiApplication::primaryScreen();
    const QPixmap shot = screen ? screen->grabWindow(0) : QPixmap();
    if (size)
        *size = shot.size();
    return shot;
}
