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
    // "orig:<id>" is the automatic icon of a card, ignoring its custom icon
    // "new:<category>:<percent-encoded target>" previews a card that has not been saved yet
    AppCore *core = AppCore::instance();
    QPixmap pm;
    if (itemId.startsWith(QLatin1String("path:"))) {
        pm = core->pixmapForFile(QUrl::fromPercentEncoding(itemId.mid(5).toUtf8()), px);
    } else if (itemId.startsWith(QLatin1String("orig:"))) {
        pm = core->pixmapOriginal(itemId.mid(5), px);
    } else if (itemId.startsWith(QLatin1String("new:"))) {
        const int sep = itemId.indexOf(QLatin1Char(':'), 4);
        pm = core->pixmapForNew(itemId.mid(4, sep - 4).toInt(),
                                QUrl::fromPercentEncoding(itemId.mid(sep + 1).toUtf8()), QString(), px);
    } else {
        pm = core->pixmapFor(itemId, px);
    }
    if (size)
        *size = pm.size();
    return pm;
}
