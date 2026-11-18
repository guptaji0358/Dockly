#pragma once

#include <QQuickImageProvider>

// image://screen/<anything>  -> a fresh snapshot of the primary screen (the id only defeats QML's image cache).
class ScreenImageProvider : public QQuickImageProvider
{
public:
    ScreenImageProvider();
    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;
};
