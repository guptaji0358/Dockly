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
        {"home", R"(<path d="M15 21v-8a1 1 0 0 0-1-1h-4a1 1 0 0 0-1 1v8"/><path d="M3 10a2 2 0 0 1 .709-1.528l7-5.999a2 2 0 0 1 2.582 0l7 5.999A2 2 0 0 1 21 10v9a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2z"/>)"},
        {"software", R"(<rect x="2" y="4" width="20" height="16" rx="2"/><path d="M10 4v4"/><path d="M2 8h20"/><path d="M6 4v4"/>)"},
        {"documents", R"(<path d="M15 2H6a2 2 0 0 0-2 2v16a2 2 0 0 0 2 2h12a2 2 0 0 0 2-2V7Z"/><path d="M14 2v4a2 2 0 0 0 2 2h4"/><path d="M10 9H8"/><path d="M16 13H8"/><path d="M16 17H8"/>)"},
        {"entertainment", R"(<circle cx="12" cy="12" r="10"/><path d="M10 8l6 4-6 4Z"/>)"},
        {"web", R"(<circle cx="12" cy="12" r="10"/><path d="M12 2a14.5 14.5 0 0 0 0 20 14.5 14.5 0 0 0 0-20"/><path d="M2 12h20"/>)"},
        {"collections", R"(<path d="M20 20a2 2 0 0 0 2-2V8a2 2 0 0 0-2-2h-7.9a2 2 0 0 1-1.69-.9L9.6 3.9A2 2 0 0 0 7.93 3H4a2 2 0 0 0-2 2v13a2 2 0 0 0 2 2Z"/>)"},
        {"connector", R"(<path d="M10 13a5 5 0 0 0 7.54.54l3-3a5 5 0 0 0-7.07-7.07l-1.72 1.71"/><path d="M14 11a5 5 0 0 0-7.54-.54l-3 3a5 5 0 0 0 7.07 7.07l1.71-1.71"/>)"},
        {"github", R"(<path d="M15 22v-4a4.8 4.8 0 0 0-1-3.5c3 0 6-2 6-5.5.08-1.25-.27-2.48-1-3.5.28-1.15.28-2.35 0-3.5 0 0-1 0-3 1.5-2.64-.5-5.36-.5-8 0C6 2 5 2 5 2c-.3 1.15-.3 2.35 0 3.5A5.403 5.403 0 0 0 4 9c0 3.5 3 5.5 6 5.5-.39.49-.68 1.05-.85 1.65-.17.6-.22 1.23-.15 1.85v4"/><path d="M9 18c-4.51 2-5-2-7-2"/>)"},
        {"settings", R"(<path d="M21 4h-7M10 4H3M21 12h-9M8 12H3M21 20h-5M12 20H3M14 2v4M8 10v4M16 18v4"/>)"},
        {"edit", R"(<path d="M21.174 6.812a1 1 0 0 0-3.986-3.987L3.842 16.174a2 2 0 0 0-.5.83l-1.321 4.352a.5.5 0 0 0 .623.622l4.353-1.32a2 2 0 0 0 .83-.497z"/><path d="m15 5 4 4"/>)"},
        {"star", R"(<path d="M11.525 2.295a.53.53 0 0 1 .95 0l2.31 4.679a2.123 2.123 0 0 0 1.595 1.16l5.166.756a.53.53 0 0 1 .294.904l-3.736 3.638a2.123 2.123 0 0 0-.611 1.878l.882 5.14a.53.53 0 0 1-.771.56l-4.618-2.428a2.122 2.122 0 0 0-1.973 0L6.396 21.01a.53.53 0 0 1-.77-.56l.881-5.139a2.122 2.122 0 0 0-.611-1.879L2.16 9.795a.53.53 0 0 1 .294-.906l5.165-.755a2.122 2.122 0 0 0 1.597-1.16z"/>)"},
        {"star-fill", R"(<path fill="currentColor" d="M11.525 2.295a.53.53 0 0 1 .95 0l2.31 4.679a2.123 2.123 0 0 0 1.595 1.16l5.166.756a.53.53 0 0 1 .294.904l-3.736 3.638a2.123 2.123 0 0 0-.611 1.878l.882 5.14a.53.53 0 0 1-.771.56l-4.618-2.428a2.122 2.122 0 0 0-1.973 0L6.396 21.01a.53.53 0 0 1-.77-.56l.881-5.139a2.122 2.122 0 0 0-.611-1.879L2.16 9.795a.53.53 0 0 1 .294-.906l5.165-.755a2.122 2.122 0 0 0 1.597-1.16z"/>)"},
        {"close", R"(<path d="M18 6 6 18M6 6l12 12"/>)"},
        {"trash", R"(<path d="M3 6h18"/><path d="M19 6v14c0 1-1 2-2 2H7c-1 0-2-1-2-2V6"/><path d="M8 6V4c0-1 1-2 2-2h4c1 0 2 1 2 2v2"/><path d="M10 11v6M14 11v6"/>)"},
        {"grid", R"(<rect x="3" y="3" width="7" height="7" rx="1"/><rect x="14" y="3" width="7" height="7" rx="1"/><rect x="14" y="14" width="7" height="7" rx="1"/><rect x="3" y="14" width="7" height="7" rx="1"/>)"},
        {"list", R"(<path d="M3 6h.01M3 12h.01M3 18h.01M8 6h13M8 12h13M8 18h13"/>)"},
        {"compact", R"(<path d="M3 6h18M3 12h18M3 18h18"/>)"},
        {"hex", R"(<path d="M21 16V8a2 2 0 0 0-1-1.73l-7-4a2 2 0 0 0-2 0l-7 4A2 2 0 0 0 3 8v8a2 2 0 0 0 1 1.73l7 4a2 2 0 0 0 2 0l7-4A2 2 0 0 0 21 16z"/>)"},
        {"orbit", R"(<circle cx="12" cy="12" r="3"/><circle cx="19" cy="5" r="2"/><circle cx="5" cy="19" r="2"/><path d="M10.4 21.9a10 10 0 0 0 9.941-15.416"/><path d="M13.5 2.1a10 10 0 0 0-9.841 15.416"/>)"},
        {"search", R"(<circle cx="11" cy="11" r="8"/><path d="m21 21-4.3-4.3"/>)"},
        {"scan", R"(<path d="M3 7V5a2 2 0 0 1 2-2h2M17 3h2a2 2 0 0 1 2 2v2M21 17v2a2 2 0 0 1-2 2h-2M7 21H5a2 2 0 0 1-2-2v-2M7 12h10"/>)"},
        {"add", R"(<path d="M5 12h14M12 5v14"/>)"},
        {"warning", R"(<path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3"/><path d="M12 9v4M12 17h.01"/>)"},
        {"check", R"(<path d="M20 6 9 17l-5-5"/>)"},
        {"open", R"(<path d="M15 3h6v6M10 14 21 3"/><path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/>)"},
        {"locate", R"(<circle cx="12" cy="12" r="10"/><path d="M22 12h-4M6 12H2M12 6V2M12 22v-4"/>)"},
    };
    return m;
}

QByteArray svgFor(const QString &name, const QColor &color)
{
    const QString inner = markup().value(name);
    if (inner.isEmpty())
        return {};
    // keep the alpha (QColor::name() drops it) and inset the 24-grid slightly so round caps never clip at the edge
    const QString alpha = QString::number(color.alphaF(), 'f', 3);
    return QStringLiteral("<svg xmlns='http://www.w3.org/2000/svg' viewBox='-0.5 -0.5 25 25' fill='none' "
                          "stroke='%1' stroke-opacity='%3' fill-opacity='%3' stroke-width='2' "
                          "stroke-linecap='round' stroke-linejoin='round' color='%1'>%2</svg>")
        .arg(color.name(), inner, alpha)
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
