#include "iconpack.h"

#include <QHash>
#include <QPainter>
#include <QSvgRenderer>

namespace {

// Inner SVG markup on a 24x24 grid. The wrapper supplies stroke / fill defaults.
const QHash<QString, QString> &markup()
{
    static const QHash<QString, QString> m = {
        {"logo", R"(<path d="M12 2.8c3.6 2.1 5.7 5.7 5.7 9.9L15.2 17H8.8l-2.5-4.3C6.3 8.5 8.4 4.9 12 2.8Z"/><circle cx="12" cy="10" r="1.9"/><path d="M8.8 17 7 21l4-1.8M15.2 17 17 21l-4-1.8"/>)"},
        {"home", R"(<path d="M3 11.5 12 4l9 7.5"/><path d="M5.5 10v9.5h13V10"/><path d="M10 19.5v-5h4v5"/>)"},
        {"software", R"(<rect x="3.5" y="4" width="17" height="12.5" rx="2.5"/><path d="M3.5 8.5h17"/><path d="M8 20.5h8M12 16.5v4"/>)"},
        {"documents", R"(<path d="M7 3.5h7l4.5 4.5v12A1.5 1.5 0 0 1 17 21.5H7A1.5 1.5 0 0 1 5.5 20V5A1.5 1.5 0 0 1 7 3.5Z"/><path d="M14 3.5V8h4.5"/><path d="M8.5 13h7M8.5 16.5h5"/>)"},
        {"entertainment", R"(<rect x="3" y="5" width="18" height="14" rx="3.5"/><path d="M10 9.3v5.4l4.6-2.7Z" fill="currentColor"/>)"},
        {"web", R"(<circle cx="12" cy="12" r="9"/><path d="M3 12h18"/><path d="M12 3c2.8 2.6 4 5.6 4 9s-1.2 6.4-4 9c-2.8-2.6-4-5.6-4-9s1.2-6.4 4-9Z"/>)"},
        {"collections", R"(<path d="M3.5 7.5A1.5 1.5 0 0 1 5 6h4l2 2h8a1.5 1.5 0 0 1 1.5 1.5V18a1.5 1.5 0 0 1-1.5 1.5H5A1.5 1.5 0 0 1 3.5 18Z"/>)"},
        {"connector", R"(<path d="M10 14a4 4 0 0 0 5.7 0l3-3a4 4 0 0 0-5.7-5.7l-1 1M14 10a4 4 0 0 0-5.7 0l-3 3a4 4 0 0 0 5.7 5.7l1-1"/>)"},
        {"settings", R"(<path d="M4 7h9M19 7h1M4 17h1M11 17h9"/><circle cx="16" cy="7" r="2.3"/><circle cx="8" cy="17" r="2.3"/>)"},
        {"edit", R"(<path d="M4 20h4L18.5 9.5a2.1 2.1 0 0 0-3-3L5 17v3Z"/><path d="M13.5 8.5l3 3"/>)"},
        {"star", R"(<path d="M12 3.5l2.6 5.4 5.9.8-4.3 4.1 1.1 5.9L12 17l-5.3 2.7 1.1-5.9L3.5 9.7l5.9-.8Z"/>)"},
        {"star-fill", R"(<path fill="currentColor" d="M12 3.5l2.6 5.4 5.9.8-4.3 4.1 1.1 5.9L12 17l-5.3 2.7 1.1-5.9L3.5 9.7l5.9-.8Z"/>)"},
        {"close", R"(<path d="M6 6l12 12M18 6 6 18"/>)"},
        {"trash", R"(<path d="M4.5 7h15"/><path d="M9.5 7V4.5h5V7"/><path d="M6.5 7l1 13h9l1-13"/><path d="M10 11v5M14 11v5"/>)"},
        {"grid", R"(<rect x="4" y="4" width="6.5" height="6.5" rx="1.6"/><rect x="13.5" y="4" width="6.5" height="6.5" rx="1.6"/><rect x="4" y="13.5" width="6.5" height="6.5" rx="1.6"/><rect x="13.5" y="13.5" width="6.5" height="6.5" rx="1.6"/>)"},
        {"list", R"(<rect x="3.5" y="5" width="4" height="4" rx="1.1"/><rect x="3.5" y="15" width="4" height="4" rx="1.1"/><path d="M11 7h9.5M11 17h9.5"/>)"},
        {"compact", R"(<path d="M4 6h16M4 10h16M4 14h16M4 18h16"/>)"},
        {"hex", R"(<path d="M12 3l7.8 4.5v9L12 21l-7.8-4.5v-9z"/>)"},
        {"orbit", R"(<circle cx="12" cy="12" r="2.2"/><circle cx="12" cy="12" r="8.2"/><circle cx="18" cy="6.2" r="1.5"/>)"},
        {"search", R"(<circle cx="10.5" cy="10.5" r="6.5"/><path d="M15.5 15.5 20.5 20.5"/>)"},
        {"scan", R"(<path d="M4 8V5.5A1.5 1.5 0 0 1 5.5 4H8M16 4h2.5A1.5 1.5 0 0 1 20 5.5V8M20 16v2.5a1.5 1.5 0 0 1-1.5 1.5H16M8 20H5.5A1.5 1.5 0 0 1 4 18.5V16"/><path d="M4 12h16"/>)"},
        {"add", R"(<path d="M12 5v14M5 12h14"/>)"},
        {"warning", R"(<path d="M12 3.5 21.5 20h-19Z"/><path d="M12 10v4.5"/><circle cx="12" cy="17.3" r=".7" fill="currentColor"/>)"},
        {"check", R"(<path d="M5 12.5l4.5 4.5L19 7.5"/>)"},
        {"open", R"(<path d="M8 16 17 7"/><path d="M9.5 7H17v7.5"/>)"},
        {"locate", R"(<circle cx="12" cy="12" r="6.5"/><circle cx="12" cy="12" r="1.3" fill="currentColor"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3"/>)"},
    };
    return m;
}

QByteArray svgFor(const QString &name, const QColor &color)
{
    const QString inner = markup().value(name);
    if (inner.isEmpty())
        return {};
    return QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' "
                          "stroke='%1' stroke-width='1.7' stroke-linecap='round' stroke-linejoin='round' color='%1'>%2</svg>")
        .arg(color.name(), inner)
        .replace(QLatin1String("currentColor"), color.name())
        .toUtf8();
}

} // namespace

namespace IconPack {

QStringList names() { return markup().keys(); }
bool has(const QString &name) { return markup().contains(name); }

QPixmap pixmap(const QString &name, int px, const QColor &color)
{
    if (px <= 0)
        return {};
    static QHash<QString, QPixmap> cache;
    const QString key = name + QLatin1Char('|') + QString::number(px) + QLatin1Char('|') + color.name(QColor::HexArgb);
    if (const auto it = cache.constFind(key); it != cache.cend())
        return it.value();

    QPixmap pm(px, px);
    pm.fill(Qt::transparent);
    const QByteArray svg = svgFor(name, color);
    if (!svg.isEmpty()) {
        QSvgRenderer r(svg);
        QPainter p(&pm);
        p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
        r.render(&p, QRectF(0, 0, px, px));
    }
    if (cache.size() > 600)
        cache.clear();
    cache.insert(key, pm);
    return pm;
}

QIcon icon(const QString &name, const QColor &color, int px)
{
    return QIcon(pixmap(name, px, color));
}

} // namespace IconPack
