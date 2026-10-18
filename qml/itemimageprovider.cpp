#include "itemimageprovider.h"

#include "appcore.h"

#include <QUrl>

ItemImageProvider::ItemImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap ItemImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QString itemId = id.left(id.indexOf(QLatin1Char('?')));
    const int px = requestedSize.width() > 0 ? requestedSize.width() : 64;
    // "path:<percent-encoded file>" asks for the shell icon of a file that is not a card yet
    const QPixmap pm = itemId.startsWith(QLatin1String("path:"))
        ? AppCore::instance()->pixmapForFile(QUrl::fromPercentEncoding(itemId.mid(5).toUtf8()), px)
        : AppCore::instance()->pixmapFor(itemId, px);
    if (size)
        *size = pm.size();
    return pm;
}
