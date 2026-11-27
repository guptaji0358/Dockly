#include "theme.h"
#include "appsettings.h"

#include <QGuiApplication>
#include <QStyleHints>

namespace {
struct Preset {
    const char *key;
    const char *label;
    int hue;       // 0..359, the tint of backgrounds and cards
    qreal sat;     // 0..1, how strongly they are tinted
    const char *accent;
};
const Preset kPresets[] = {
    {"ocean", "Ocean", 205, 0.80, "#1fa2ff"},
    {"forest", "Forest", 150, 0.55, "#2fbf71"},
    {"sunset", "Sunset", 18, 0.70, "#ff7a45"},
    {"rose", "Rose", 335, 0.60, "#ff4d94"},
    {"violet", "Violet", 265, 0.60, "#8b5cf6"},
    {"graphite", "Graphite", 220, 0.08, "#9aa7b8"},
};
const Preset *findPreset(const QString &key)
{
    for (const Preset &p : kPresets)
        if (key == QLatin1String(p.key))
            return &p;
    return nullptr;
}
} // namespace

QVariantList Theme::palettes() const
{
    QVariantList out;
    out.append(QVariantMap{{"key", "default"}, {"label", "Skin default"}, {"accent", QString()}});
    for (const Preset &p : kPresets)
        out.append(QVariantMap{{"key", p.key}, {"label", p.label}, {"accent", QString::fromLatin1(p.accent)}});
    return out;
}

Theme::Theme(AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    connect(settings, &AppSettings::changed, this, [this](const QString &key) {
        if (key == "theme" || key == "accent" || key == "style" || key == "navigation" || key == "palette")
            update();
    });
    if (auto *hints = QGuiApplication::styleHints())
        connect(hints, &QStyleHints::colorSchemeChanged, this, [this] { update(); });
    update();
}

void Theme::update()
{
    m_style = m_settings->text("style");
    if (m_style != "playful" && m_style != "editorial")
        m_style = "neon";
    m_dock = m_settings->text("navigation") == "dock";

    const QString mode = m_settings->text("theme");
    if (mode == "dark")
        m_dark = true;
    else if (mode == "light")
        m_dark = false;
    else
        m_dark = QGuiApplication::styleHints()
                 && QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;

    QColor defaultAccent;
    if (m_style == "neon") {
        m_radius = 14; m_borderWidth = 0; m_glow = true; m_tinted = false; m_hardShadow = false;
        m_headingFont = "Bahnschrift";
        if (m_dark) {
            m_bg = "#060913"; m_surface = "#0d1325"; m_card = "#0f1830"; m_cardHover = "#16264d";
            m_text = "#e6f3ff"; m_subtext = "#7f93b8"; m_border = "#1f3a6b";
            m_sidebar = "#04060d"; m_sidebarText = "#8fb4e8"; defaultAccent = "#00b8d9";
        } else {
            m_bg = "#e9f1ff"; m_surface = "#ffffff"; m_card = "#ffffff"; m_cardHover = "#dcebff";
            m_text = "#0b1b3a"; m_subtext = "#5a6f95"; m_border = "#b9cdf0";
            m_sidebar = "#0b1b3a"; m_sidebarText = "#bcd4ff"; defaultAccent = "#0a6cff";
        }
    } else if (m_style == "playful") {
        m_radius = 22; m_borderWidth = 0; m_glow = false; m_tinted = true; m_hardShadow = false;
        m_headingFont = "Trebuchet MS";
        if (m_dark) {
            m_bg = "#1b1233"; m_surface = "#261a47"; m_card = "#2f2158"; m_cardHover = "#3b2a6e";
            m_text = "#fff4ff"; m_subtext = "#bba8e8"; m_border = "#3b2a6e";
            m_sidebar = "#120a26"; m_sidebarText = "#d7c8ff";
        } else {
            m_bg = "#fff6e8"; m_surface = "#ffffff"; m_card = "#ffffff"; m_cardHover = "#ffe9d0";
            m_text = "#2b1b3d"; m_subtext = "#8a6f99"; m_border = "#ffd9b0";
            m_sidebar = "#2b1b3d"; m_sidebarText = "#ffd9ee";
        }
        defaultAccent = "#ff5d8f";
    } else { // editorial
        m_radius = 0; m_borderWidth = 3; m_glow = false; m_tinted = false; m_hardShadow = true;
        m_headingFont = "Georgia";
        if (m_dark) {
            m_bg = "#121212"; m_surface = "#1a1a1a"; m_card = "#1a1a1a"; m_cardHover = "#2a2a2a";
            m_text = "#f2efe6"; m_subtext = "#a29d8f"; m_border = "#f2efe6";
            m_sidebar = "#000000"; m_sidebarText = "#f2efe6";
        } else {
            m_bg = "#f2efe6"; m_surface = "#fffdf7"; m_card = "#fffdf7"; m_cardHover = "#ffe9a8";
            m_text = "#111111"; m_subtext = "#5c584d"; m_border = "#111111";
            m_sidebar = "#111111"; m_sidebarText = "#f2efe6";
        }
        defaultAccent = "#ff3b1f";
    }

    // A colour theme re-tints the surfaces and sets the accent; the skin keeps its shapes, fonts and text colours.
    const Preset *preset = findPreset(m_settings->text("palette"));
    if (preset) {
        const qreal h = preset->hue / 360.0, s = preset->sat;
        auto hsl = [h](qreal sat, qreal l) { return QColor::fromHslF(h, qBound(0.0, sat, 1.0), l); };
        if (m_dark) {
            m_bg = hsl(s * 0.55, 0.065); m_surface = hsl(s * 0.50, 0.105); m_card = hsl(s * 0.50, 0.125);
            m_cardHover = hsl(s * 0.55, 0.19); m_sidebar = hsl(s * 0.55, 0.045); m_sidebarText = hsl(s * 0.8, 0.78);
            if (m_style != "editorial")
                m_border = hsl(s * 0.55, 0.26);
        } else {
            m_bg = hsl(s * 0.9, 0.95); m_surface = hsl(s * 0.5, 0.995); m_card = m_surface;
            m_cardHover = hsl(s * 0.9, 0.89); m_sidebar = hsl(s * 0.6, 0.16); m_sidebarText = hsl(s * 0.9, 0.86);
            if (m_style != "editorial")
                m_border = hsl(s * 0.6, 0.8);
        }
        defaultAccent = preset->accent;
    }

    const QColor chosen(m_settings->text("accent"));
    m_accent = chosen.isValid() ? chosen : defaultAccent;
    m_warning = "#f0a020";
    m_missing = "#e5484d";
    m_favorite = "#f5b301";
    emit changed();
}

QColor Theme::categoryColor(int category) const
{
    static const QColor colors[] = {"#00b8d9", "#f5b301", "#a855f7", "#14b88a", "#ff5d8f"};
    return colors[qBound(0, category, 4)];
}

QColor Theme::accentOf(int category) const
{
    return m_tinted ? categoryColor(category) : m_accent;
}

QColor Theme::cardColor(int category, bool hover) const
{
    const QColor base = hover ? m_cardHover : m_card;
    if (!m_tinted)
        return base;
    const QColor c = categoryColor(category);
    const qreal a = m_dark ? (hover ? 0.42 : 0.30) : (hover ? 0.34 : 0.22);
    return QColor::fromRgbF(base.redF() * (1 - a) + c.redF() * a,
                            base.greenF() * (1 - a) + c.greenF() * a,
                            base.blueF() * (1 - a) + c.blueF() * a);
}
