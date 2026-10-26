#include "item.h"

#include <QDir>
#include <QJsonArray>

QString Item::categoryKey(LH::Category c)
{
    switch (c) {
    case LH::Software: return QStringLiteral("software");
    case LH::Document: return QStringLiteral("document");
    case LH::Entertainment: return QStringLiteral("entertainment");
    case LH::Web: return QStringLiteral("web");
    case LH::Collection: return QStringLiteral("collection");
    default: return QStringLiteral("software");
    }
}

LH::Category Item::categoryFromKey(const QString &key)
{
    if (key == QLatin1String("document")) return LH::Document;
    if (key == QLatin1String("entertainment")) return LH::Entertainment;
    if (key == QLatin1String("web")) return LH::Web;
    if (key == QLatin1String("collection")) return LH::Collection;
    return LH::Software;
}

QString Item::categoryLabel(LH::Category c)
{
    switch (c) {
    case LH::Software: return QStringLiteral("Software");
    case LH::Document: return QStringLiteral("Document");
    case LH::Entertainment: return QStringLiteral("Entertainment");
    case LH::Web: return QStringLiteral("Web");
    case LH::Collection: return QStringLiteral("Collection");
    default: return QString();
    }
}

QString Item::normalizedTarget(const QString &target)
{
    QString t = target.trimmed();
    if (!t.contains(QLatin1String("://")))
        t = QDir::cleanPath(t);
    return t.toLower();
}

QJsonObject Item::toJson() const
{
    QJsonObject o;
    o["id"] = id;
    o["category"] = categoryKey(category);
    o["name"] = name;
    o["target"] = target;
    o["favorite"] = favorite;
    if (!icon.isEmpty())
        o["icon"] = icon;
    if (lastOpened.isValid())
        o["lastOpened"] = lastOpened.toString(Qt::ISODate);
    if (category == LH::Collection)
        o["members"] = QJsonArray::fromStringList(members);
    return o;
}

Item Item::fromJson(const QJsonObject &o)
{
    Item it;
    it.id = o["id"].toString();
    it.category = categoryFromKey(o["category"].toString());
    it.name = o["name"].toString();
    it.target = o["target"].toString();
    it.favorite = o["favorite"].toBool();
    it.icon = o["icon"].toString();
    it.lastOpened = QDateTime::fromString(o["lastOpened"].toString(), Qt::ISODate);
    for (const QJsonValue &v : o["members"].toArray())
        it.members << v.toString();
    return it;
}
