#pragma once

#include <QJsonObject>
#include <QObject>
#include <QVariantMap>

// Flat key/value settings. QML binds to `values` (e.g. settings.values.theme);
// C++ uses the typed accessors.
class AppSettings : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY valuesChanged)

public:
    explicit AppSettings(QObject *parent = nullptr);

    static QVariantMap defaults();

    QVariantMap values() const { return m_values; }
    QVariant value(const QString &key) const { return m_values.value(key); }
    bool boolean(const QString &key) const { return m_values.value(key).toBool(); }
    int integer(const QString &key) const { return m_values.value(key).toInt(); }
    QString text(const QString &key) const { return m_values.value(key).toString(); }

    Q_INVOKABLE void set(const QString &key, const QVariant &value);
    void resetToDefaults();

    QJsonObject toJson() const;
    void fromJson(const QJsonObject &o);
    bool load(const QString &path);
    bool save(const QString &path) const;

signals:
    void valuesChanged();
    void changed(const QString &key);

private:
    QVariantMap m_values;
};
