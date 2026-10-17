#pragma once

#include <QColor>
#include <QObject>
#include <QPalette>

class AppSettings;

// Resolved look: skin (neon / playful / editorial), light / dark, accent and shape tokens.
class Theme : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString style READ style NOTIFY changed)
    Q_PROPERTY(bool dark READ dark NOTIFY changed)
    Q_PROPERTY(bool dock READ dock NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)
    Q_PROPERTY(int borderWidth READ borderWidth NOTIFY changed)
    Q_PROPERTY(bool glow READ glow NOTIFY changed)
    Q_PROPERTY(bool tinted READ tinted NOTIFY changed)
    Q_PROPERTY(bool hardShadow READ hardShadow NOTIFY changed)
    Q_PROPERTY(QString headingFont READ headingFont NOTIFY changed)
    Q_PROPERTY(QColor accent READ accent NOTIFY changed)
    Q_PROPERTY(QColor bg READ bg NOTIFY changed)
    Q_PROPERTY(QColor surface READ surface NOTIFY changed)
    Q_PROPERTY(QColor card READ card NOTIFY changed)
    Q_PROPERTY(QColor cardHover READ cardHover NOTIFY changed)
    Q_PROPERTY(QColor text READ text NOTIFY changed)
    Q_PROPERTY(QColor subtext READ subtext NOTIFY changed)
    Q_PROPERTY(QColor border READ border NOTIFY changed)
    Q_PROPERTY(QColor sidebar READ sidebar NOTIFY changed)
    Q_PROPERTY(QColor sidebarText READ sidebarText NOTIFY changed)
    Q_PROPERTY(QColor warning READ warning NOTIFY changed)
    Q_PROPERTY(QColor missing READ missing NOTIFY changed)
    Q_PROPERTY(QColor favorite READ favorite NOTIFY changed)

public:
    explicit Theme(AppSettings *settings, QObject *parent = nullptr);

    QString style() const { return m_style; }
    bool dark() const { return m_dark; }
    bool dock() const { return m_dock; }
    int radius() const { return m_radius; }
    int borderWidth() const { return m_borderWidth; }
    bool glow() const { return m_glow; }
    bool tinted() const { return m_tinted; }
    bool hardShadow() const { return m_hardShadow; }
    QString headingFont() const { return m_headingFont; }
    QColor accent() const { return m_accent; }
    QColor bg() const { return m_bg; }
    QColor surface() const { return m_surface; }
    QColor card() const { return m_card; }
    QColor cardHover() const { return m_cardHover; }
    QColor text() const { return m_text; }
    QColor subtext() const { return m_subtext; }
    QColor border() const { return m_border; }
    QColor sidebar() const { return m_sidebar; }
    QColor sidebarText() const { return m_sidebarText; }
    QColor warning() const { return m_warning; }
    QColor missing() const { return m_missing; }
    QColor favorite() const { return m_favorite; }

    // Each category has its own colour (used for tinted cards, tags and the dock).
    Q_INVOKABLE QColor categoryColor(int category) const;
    // Colour a card should be filled with for a given category (tinted in the playful skin).
    Q_INVOKABLE QColor cardColor(int category, bool hover) const;
    Q_INVOKABLE QColor accentOf(int category) const;

    // checkIconPath: an image used for the checked state of checkboxes.
    QString styleSheet(const QString &checkIconPath = QString()) const;
    QPalette palette() const;

signals:
    void changed();

private:
    void update();

    AppSettings *m_settings;
    QString m_style = "neon";
    bool m_dark = true;
    bool m_dock = false;
    int m_radius = 14;
    int m_borderWidth = 1;
    bool m_glow = false;
    bool m_tinted = false;
    bool m_hardShadow = false;
    QString m_headingFont;
    QColor m_accent, m_bg, m_surface, m_card, m_cardHover, m_text, m_subtext, m_border;
    QColor m_sidebar, m_sidebarText, m_warning, m_missing, m_favorite;
};
