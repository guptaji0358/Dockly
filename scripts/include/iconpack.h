#pragma once

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

// Custom line-icon set, drawn from SVG markup embedded in the binary (no image files to ship).
// Icons use `currentColor`, so any colour can be applied at render time.
namespace IconPack {

QStringList names();
bool has(const QString &name);

// Rasterise an icon at px x px (device pixels) in the given colour. Cached.
QPixmap pixmap(const QString &name, int px, const QColor &color);
QIcon icon(const QString &name, const QColor &color, int px = 48);

} // namespace IconPack
