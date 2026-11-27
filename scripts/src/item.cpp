#include "item.h"

#include <QDir>
#include <QJsonArray>
#include <QRegularExpression>

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

QString Item::normalizedLaunch(const QString &target, const QString &args)
{
    const QString a = args.simplified();
    return a.isEmpty() ? normalizedTarget(target) : normalizedTarget(target) + QChar(0x1f) + a.toLower();
}

QStringList Item::parseTags(const QString &text)
{
    QStringList out;
    for (QString t : text.split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts)) {
        t = t.toLower();
        while (t.startsWith(QLatin1Char('#')))
            t.remove(0, 1);
        t.replace(QLatin1Char('_'), QLatin1Char('-'));
        if (!t.isEmpty() && t.size() <= 24 && !out.contains(t))
            out << t;
    }
    out.sort();
    return out.mid(0, 12);
}

QJsonObject Item::toJson() const
{
    QJsonObject o;
    o["id"] = id;
    o["category"] = categoryKey(category);
    o["name"] = name;
    o["target"] = target;
    o["favorite"] = favorite;
    if (!guard)
        o["guard"] = false; // written only when switched off; a missing key means on
    if (!args.isEmpty())
        o["args"] = args;
    if (!icon.isEmpty())
        o["icon"] = icon;
    if (!tags.isEmpty())
        o["tags"] = QJsonArray::fromStringList(tags);
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
    it.guard = o["guard"].toBool(true); // older backups have no key: on
    it.args = o["args"].toString();
    it.icon = o["icon"].toString();
    for (const QJsonValue &v : o["tags"].toArray())
        it.tags << v.toString();
    it.tags = parseTags(it.tags.join(QLatin1Char(',')));
    it.lastOpened = QDateTime::fromString(o["lastOpened"].toString(), Qt::ISODate);
    for (const QJsonValue &v : o["members"].toArray())
        it.members << v.toString();
    return it;
}
