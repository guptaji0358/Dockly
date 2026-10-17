#include "itemimageprovider.h"

#include "appcore.h"

ItemImageProvider::ItemImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap ItemImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString itemId = id.left(id.indexOf(QLatin1Char('?')));
    const int px = requestedSize.width() > 0 ? requestedSize.width() : 64;
    const QPixmap pm = AppCore::instance()->pixmapFor(itemId, px);
    if (size)
        *size = pm.size();
    return pm;
}
