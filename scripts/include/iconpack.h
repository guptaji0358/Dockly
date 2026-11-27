#pragma once

#include <QColor>
#include <QPixmap>
#include <QString>

// Custom line-icon set, drawn from SVG markup embedded in the binary (no image files to ship).
// Icons use `currentColor`, so any colour can be applied at render time.
namespace IconPack {

QStringList names();

// Rasterise an icon at px x px (device pixels) in the given colour. Cached.
QPixmap pixmap(const QString &name, int px, const QColor &color);

} // namespace IconPack
