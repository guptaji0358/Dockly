#include "appsettings.h"

#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>

AppSettings::AppSettings(QObject *parent)
    : QObject(parent), m_values(defaults())
{
}

QVariantMap AppSettings::defaults()
{
    return {
        {"startWithWindows", false},
        {"minimizeToTray", false},
        {"theme", "dark"},        // light | dark | system
        {"style", "neon"},        // neon | playful | editorial
        {"navigation", "sidebar"}, // sidebar | dock
        {"accent", ""},           // empty = the skin's own accent
        {"recentEnabled", true},
        {"maxRecent", 12},
        {"confirmDelete", true},
        {"checkMissingOnStartup", true},
        {"defaultTab", "home"},   // home | software | custom
        {"cardSize", "medium"},   // small | medium | large
        {"viewStyle", "grid"},    // grid | list | compact
        {"animations", true},
        {"floatingTiles", true},  // tiles gently float and tilt toward the cursor
        {"layoutMode", "hex"},    // hex | orbit | grid
        {"gpuLevel", "balanced"}, // low | balanced | high : how much the GPU is used for effects
        {"graphicsApi", "opengl"}, // opengl | d3d11 | software (applies after restart)
        {"bridgeEnabled", false},  // browser connector: loopback endpoint for the Chrome / Edge extension
        {"bridgePort", 47821},
        {"pauseWhenInactive", true}, // stop animated effects while the window is in the background
        {"language", "English"},
        {"dockEnabled", true},    // launch from a slim edge dock instead of opening the main window
        {"dockMode", "categories"}, // categories (hover a category for a grid of its apps) | favorites (a strip of pinned icons)
        {"dockEdge", "right"},    // left | right | top | bottom
        {"dockHotkey", "Ctrl+Alt+D"}, // system-wide shortcut that opens the dock; empty = none
        {"dockAutoHide", true},   // slide away after launching / when the pointer leaves; hover the edge to bring it back
        {"dockHideFullscreen", true}, // get out of the way of full-screen apps; rest the pointer on the edge to bring it back
        {"dockIdleHide", 10},    // seconds without touching the mouse or trackpad before the collapsed line hides; 0 = never
        {"dockOpacity", 40},    // dock background opacity in percent; 0 = fully see-through
        {"defaultBrowser", ""},  // empty = the system default; otherwise the browser's exe path
    };
}

void AppSettings::set(const QString &key, const QVariant &value)
{
    const QVariantMap defs = defaults();
    if (!defs.contains(key))
        return;
    QVariant v = value;
    if (!v.convert(defs.value(key).metaType()))
        return;
    if (m_values.value(key) == v)
        return;
    m_values[key] = v;
    emit changed(key);
    emit valuesChanged();
}

void AppSettings::resetToDefaults()
{
    m_values = defaults();
    emit valuesChanged();
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it)
        emit changed(it.key());
}

QJsonObject AppSettings::toJson() const
{
    return QJsonObject::fromVariantMap(m_values);
}

void AppSettings::fromJson(const QJsonObject &o)
{
    const QVariantMap defs = defaults();
    QVariantMap next = defs;
    for (auto it = defs.cbegin(); it != defs.cend(); ++it) {
        if (!o.contains(it.key()))
            continue;
        QVariant v = o.value(it.key()).toVariant();
        if (v.convert(it.value().metaType()))
            next[it.key()] = v;
    }
    m_values = next;
    emit valuesChanged();
    for (auto it = m_values.cbegin(); it != m_values.cend(); ++it)
        emit changed(it.key());
}

bool AppSettings::load(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return false;
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject())
        return false;
    fromJson(doc.object());
    return true;
}

bool AppSettings::save(const QString &path) const
{
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly))
        return false;
    f.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return f.commit();
}
