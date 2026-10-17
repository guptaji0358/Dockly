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
        {"language", "English"},
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
