#include "theme.h"
#include "appsettings.h"

#include <QGuiApplication>
#include <QRegularExpression>
#include <QStyleHints>

Theme::Theme(AppSettings *settings, QObject *parent)
    : QObject(parent), m_settings(settings)
{
    connect(settings, &AppSettings::changed, this, [this](const QString &key) {
        if (key == "theme" || key == "accent" || key == "style" || key == "navigation")
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

QPalette Theme::palette() const
{
    QPalette p;
    p.setColor(QPalette::Window, m_bg);
    p.setColor(QPalette::WindowText, m_text);
    p.setColor(QPalette::Base, m_surface);
    p.setColor(QPalette::AlternateBase, m_card);
    p.setColor(QPalette::Text, m_text);
    p.setColor(QPalette::Button, m_surface);
    p.setColor(QPalette::ButtonText, m_text);
    p.setColor(QPalette::ToolTipBase, m_surface);
    p.setColor(QPalette::ToolTipText, m_text);
    p.setColor(QPalette::Highlight, m_accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::PlaceholderText, m_subtext);
    p.setColor(QPalette::Disabled, QPalette::Text, m_subtext);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, m_subtext);
    return p;
}

QString Theme::styleSheet(const QString &checkIconPath) const
{
    QString s = QStringLiteral(R"(
QWidget { color: @text@; font-family: "Segoe UI"; font-size: 10pt; }
QMainWindow, QDialog, QWidget#page, QScrollArea, QScrollArea > QWidget > QWidget { background: @bg@; }
QLabel { background: transparent; }
QLabel#h1 { font-family: "@heading@"; font-size: 18pt; font-weight: 600; }
QLabel#h2 { font-family: "@heading@"; font-size: 12pt; font-weight: 600; }
QLabel#sub { color: @subtext@; }
QLineEdit, QSpinBox, QComboBox, QTextEdit {
    background: @surface@; border: 1px solid @border@; border-radius: 8px;
    padding: 6px 10px; selection-background-color: @accent@; selection-color: white; }
QLineEdit:focus, QSpinBox:focus, QComboBox:focus, QTextEdit:focus { border: 1px solid @accent@; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: @surface@; border: 1px solid @border@;
    selection-background-color: @accent@; selection-color: white; outline: none; }
QPushButton { background: @surface@; border: 1px solid @border@; border-radius: 8px; padding: 7px 14px; }
QPushButton:hover { border-color: @accent@; }
QPushButton:pressed { background: @cardHover@; }
QPushButton:disabled { color: @subtext@; }
QPushButton[primary="true"] { background: @accent@; color: white; border: 1px solid @accent@; }
QPushButton[primary="true"]:hover { border-color: @text@; }
QPushButton[danger="true"] { color: @missing@; }
QListView, QListWidget { background: transparent; border: none; outline: none; }
QListWidget#picker { background: @surface@; border: 1px solid @border@; border-radius: 8px; padding: 4px; }
QListWidget#picker::item { padding: 5px; border-radius: 6px; }
QListWidget#picker::item:selected { background: @cardHover@; color: @text@; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: @border@; border-radius: 5px; min-height: 30px; }
QScrollBar::handle:vertical:hover { background: @subtext@; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: transparent; }
QScrollBar:horizontal { height: 0; }
QMenu { background: @surface@; border: 1px solid @border@; border-radius: 8px; padding: 6px; }
QMenu::item { padding: 7px 26px; border-radius: 6px; }
QMenu::item:selected { background: @accent@; color: white; }
QMenu::separator { height: 1px; background: @border@; margin: 5px 8px; }
QToolTip { background: @surface@; color: @text@; border: 1px solid @border@; padding: 4px; }
QCheckBox { spacing: 10px; background: transparent; }
QCheckBox::indicator { width: 18px; height: 18px; border: 1px solid @subtext@; border-radius: 5px; background: @surface@; }
QCheckBox::indicator:hover { border-color: @accent@; }
QCheckBox::indicator:checked { background: @accent@; border-color: @accent@; image: url("@check@"); }
QSpinBox { padding-right: 6px; }
QSpinBox::up-button, QSpinBox::down-button { width: 0; border: none; }
QFrame#sidebar { background: @sidebar@; }
QFrame#sidebar QLabel { color: white; }
QFrame#sidebar QLabel#sub { color: @sidebarText@; }
QPushButton#nav { background: transparent; color: @sidebarText@; border: none; border-radius: 12px; padding: 0; min-height: 46px; }
QPushButton#nav:hover { background: rgba(255,255,255,0.08); border: none; }
QPushButton#nav:checked { background: @accent@; color: white; border: none; }
QFrame#dock { background: @sidebar@; border: 1px solid @border@; border-radius: 22px; }
QPushButton#dockBtn { background: transparent; color: @sidebarText@; border: none; border-radius: 16px; padding: 7px 16px; }
QPushButton#dockBtn:hover { background: rgba(255,255,255,0.10); border: none; }
QPushButton#dockBtn:checked { background: @accent@; color: white; border: none; }
QFrame#group, QFrame#panel { background: @surface@; border: 1px solid @border@; border-radius: 12px; }
QFrame#group QLabel, QFrame#group QCheckBox, QFrame#panel QLabel { background: transparent; }
QFrame#viewbar { background: @surface@; border: 1px solid @border@; border-radius: 12px; }
QToolButton#viewBtn { background: transparent; border: none; border-radius: 8px; padding: 6px; }
QToolButton#viewBtn:hover { background: @cardHover@; }
QToolButton#viewBtn:checked { background: @accent@; }
QSlider::groove:horizontal { height: 4px; background: @border@; border-radius: 2px; }
QSlider::sub-page:horizontal { background: @accent@; border-radius: 2px; }
QSlider::handle:horizontal { width: 14px; height: 14px; margin: -5px 0; background: @accent@; border-radius: 7px; }
QLabel#dropOverlay { background: rgba(@accentRgb@, 36); border: 3px dashed @accent@; border-radius: 16px; color: @accent@; font-size: 22pt; font-weight: 600; }
QLabel#toast { color: @subtext@; padding: 4px 8px; }
)");

    // Scale corner radii and border widths to the skin (editorial = square and thick).
    const qreal scale = m_radius / 14.0;
    const QRegularExpression radii("border-radius: (\\d+)px");
    QString out;
    int last = 0;
    auto it = radii.globalMatch(s);
    while (it.hasNext()) {
        const auto m = it.next();
        out += s.mid(last, m.capturedStart() - last);
        // Qt draws square corners when the radius exceeds half the control height, so cap it.
        const int orig = m.captured(1).toInt();
        out += QStringLiteral("border-radius: %1px").arg(qMin(int(orig * scale + 0.5), orig > 16 ? 26 : 14));
        last = m.capturedEnd();
    }
    out += s.mid(last);
    s = out;
    if (m_style == "editorial")
        s.replace("1px solid", "2px solid");

    const QList<QPair<QString, QString>> tokens = {
        {"@heading@", m_headingFont},
        {"@accentRgb@", QStringLiteral("%1,%2,%3").arg(m_accent.red()).arg(m_accent.green()).arg(m_accent.blue())}, {"@text@", m_text.name()}, {"@bg@", m_bg.name()},
        {"@surface@", m_surface.name()}, {"@border@", m_border.name()}, {"@accent@", m_accent.name()},
        {"@subtext@", m_subtext.name()}, {"@cardHover@", m_cardHover.name()}, {"@check@", checkIconPath},
        {"@missing@", m_missing.name()}, {"@sidebar@", m_sidebar.name()}, {"@sidebarText@", m_sidebarText.name()},
    };
    for (const auto &t : tokens)
        s.replace(t.first, t.second);
    return s;
}
