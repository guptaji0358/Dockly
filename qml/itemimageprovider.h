#pragma once

#include <QQuickImageProvider>

// image://item/<id>?v=<n>  -> the card icon for an item (file icon, thumbnail, favicon...).
class ItemImageProvider : public QQuickImageProvider
{
public:
    ItemImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
