#pragma once

#include <QQuickImageProvider>

// image://icon/<name>?c=<rrggbb|aarrggbb>  -> custom SVG icon from the IconPack, tinted.
class IconImageProvider : public QQuickImageProvider
{
public:
    IconImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
