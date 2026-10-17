#include "iconimageprovider.h"

#include "iconpack.h"

IconImageProvider::IconImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Pixmap)
{
}

QPixmap IconImageProvider::requestPixmap(const QString &id, QSize *size, const QSize &requestedSize)
{
    const int q = id.indexOf(QLatin1Char('?'));
    const QString name = q < 0 ? id : id.left(q);
    QColor color(Qt::white);
    if (q >= 0) {
        const QString query = id.mid(q + 1);
        for (const QString &part : query.split('&')) {
            if (part.startsWith(QLatin1String("c="))) {
                const QString hex = part.mid(2);
                color = QColor(QLatin1Char('#') + hex);
            }
        }
    }
    const int px = requestedSize.width() > 0 ? requestedSize.width() : 48;
    const QPixmap pm = IconPack::pixmap(name, px, color);
    if (size)
        *size = pm.size();
    return pm;
}
