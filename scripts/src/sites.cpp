#include "sites.h"

#include "appcore.h"
#include "lhtypes.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QRegularExpression>
#include <QUrl>
#include <QUuid>
#include <algorithm>

namespace {
constexpr int kSuggestMin = 5; // loose links on one host before we offer to make it a site

struct Preset
{
    const char *id, *name, *hosts, *groupBy, *icon, *color;
};
const Preset kPresets[] = {
    {"github", "GitHub", "github.com", "path", "github", "#E2E8F0"},
    {"chatgpt", "ChatGPT", "chatgpt.com,chat.openai.com", "none", "web", "#10A37F"},
    {"claude", "Claude", "claude.ai", "none", "web", "#D97757"},
    {"gemini", "Gemini", "gemini.google.com", "none", "web", "#8AB4F8"},
    {"youtube", "YouTube", "youtube.com,youtu.be", "none", "web", "#FF4D4D"},
};

const char *const kPalette[] = {"#4C8DFF", "#FFA94D", "#F06595", "#3DDC97", "#A78BFA", "#22D3EE", "#FACC15", "#FB7185"};
} // namespace

Sites::Sites(AppCore *core)
    : QObject(core)
    , m_core(core)
{
}

QString Sites::normalizeHost(const QString &sIn)
{
    QString s = sIn.trimmed().toLower();
    const int scheme = s.indexOf(QStringLiteral("://"));
    if (scheme >= 0)
        s.remove(0, scheme + 3);
    const int slash = s.indexOf('/');
    if (slash >= 0)
        s.truncate(slash);
    if (s.startsWith(QLatin1String("www.")))
        s.remove(0, 4);
    return s;
}

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

void Sites::seedPresets(bool onlyMissing)
{
    for (const Preset &p : kPresets) {
        const QString id = QString::fromLatin1(p.id);
        const bool have = std::any_of(m_sites.cbegin(), m_sites.cend(), [&](const Site &s) { return s.id == id; });
        if (have && onlyMissing)
            continue;
        Site s;
        s.id = id;
        s.name = QString::fromLatin1(p.name);
        s.hosts = QString::fromLatin1(p.hosts).split(',');
        s.groupBy = QString::fromLatin1(p.groupBy);
        s.icon = QString::fromLatin1(p.icon);
        s.color = QString::fromLatin1(p.color);
        m_sites.append(s);
    }
}

void Sites::load(const QString &dir)
{
    m_path = dir + QStringLiteral("/sites.json");
    m_sites.clear();
    m_dismissed.clear();

    QFile f(m_path);
    if (f.open(QIODevice::ReadOnly)) {
        const QJsonObject root = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        for (const QJsonValue &v : root["sites"].toArray()) {
            const QJsonObject o = v.toObject();
            Site s;
            s.id = o["id"].toString();
            s.name = o["name"].toString();
            s.groupBy = o["groupBy"].toString(QStringLiteral("none"));
            s.icon = o["icon"].toString(QStringLiteral("web"));
            s.color = o["color"].toString(QStringLiteral("#4C8DFF"));
            s.autoFile = o["autoFile"].toBool(true);
            for (const QJsonValue &h : o["hosts"].toArray())
                s.hosts << normalizeHost(h.toString());
            s.hosts.removeAll(QString());
            if (!s.id.isEmpty() && !s.name.isEmpty() && !s.hosts.isEmpty())
                m_sites.append(s);
        }
        for (const QJsonValue &d : root["dismissed"].toArray())
            m_dismissed.insert(d.toString());
        if (root.contains("sites"))
            return;
    }
    seedPresets(false);
    save();
}

void Sites::save() const
{
    if (m_path.isEmpty())
        return;
    QJsonArray arr;
    for (const Site &s : m_sites) {
        QJsonObject o;
        o["id"] = s.id;
        o["name"] = s.name;
        o["hosts"] = QJsonArray::fromStringList(s.hosts);
        o["groupBy"] = s.groupBy;
        o["icon"] = s.icon;
        o["color"] = s.color;
        o["autoFile"] = s.autoFile;
        arr.append(o);
    }
    QJsonObject root;
    root["sites"] = arr;
    root["dismissed"] = QJsonArray::fromStringList(QStringList(m_dismissed.cbegin(), m_dismissed.cend()));
    QFile f(m_path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

// ---------------------------------------------------------------------------
// Matching
// ---------------------------------------------------------------------------

int Sites::siteForUrl(const QString &url, QString *matchedHost) const
{
    const QUrl u(url);
    const QString scheme = u.scheme().toLower();
    if (scheme != QLatin1String("http") && scheme != QLatin1String("https"))
        return -1;
    QString host = u.host().toLower();
    if (host.startsWith(QLatin1String("www.")))
        host.remove(0, 4);
    if (host.isEmpty())
        return -1;
    for (int i = 0; i < m_sites.size(); ++i)
        for (const QString &h : m_sites[i].hosts)
            if (host == h || host.endsWith(QLatin1Char('.') + h)) {
                if (matchedHost)
                    *matchedHost = h;
                return i;
            }
    return -1;
}

// "" when the link can't be placed in a group (e.g. github.com/someone has no repo part)
QString Sites::groupNameFor(const Site &s, const QString &url, const QString &matchedHost) const
{
    const QUrl u(url);
    if (s.groupBy == QLatin1String("path")) {
        const QStringList seg = u.path().split('/', Qt::SkipEmptyParts);
        return seg.size() >= 2 ? s.name + QStringLiteral(" · ") + seg.first() : QString();
    }
    if (s.groupBy == QLatin1String("subdomain")) {
        QString host = u.host().toLower();
        if (host.startsWith(QLatin1String("www.")))
            host.remove(0, 4);
        if (host.size() > matchedHost.size() + 1)
            return s.name + QStringLiteral(" · ") + host.left(host.size() - matchedHost.size() - 1);
    }
    return s.name;
}

QString Sites::findOrCreateGroup(const QString &name)
{
    for (const QVariant &cv : m_core->collectionsList()) {
        const QVariantMap c = cv.toMap();
        if (c.value("name").toString().compare(name, Qt::CaseInsensitive) == 0)
            return c.value("id").toString();
    }
    return m_core->createCollection(name);
}

void Sites::autoFile(const QString &itemId)
{
    const Item it = m_core->item(itemId);
    if (it.id.isEmpty() || it.category != LH::Web)
        return;
    QString host;
    const int idx = siteForUrl(it.target, &host);
    if (idx < 0 || !m_sites[idx].autoFile)
        return;
    const QString name = groupNameFor(m_sites[idx], it.target, host);
    if (name.isEmpty())
        return;
    const QString col = findOrCreateGroup(name);
    if (!col.isEmpty())
        m_core->addToCollection(col, {itemId});
}

// ---------------------------------------------------------------------------
// Groups
// ---------------------------------------------------------------------------

int Sites::siteOfCollection(const QString &collectionId) const
{
    const QStringList ids = m_core->queryIds(LH::ModeMembers, LH::AllCategories, QString(), collectionId);
    int site = -1;
    for (const QString &id : ids) {
        const int i = siteForUrl(m_core->item(id).target);
        if (i < 0 || (site >= 0 && i != site))
            return -1;
        site = i;
    }
    return site;
}

bool Sites::isSiteGroup(const QString &collectionId) const { return siteOfCollection(collectionId) >= 0; }

QVariantList Sites::groups(int siteIndex) const
{
    struct G
    {
        QString id, name;
    };
    QVector<G> found;
    for (const QVariant &cv : m_core->collectionsList()) {
        const QVariantMap c = cv.toMap();
        const QString cid = c.value("id").toString();
        if (siteOfCollection(cid) == siteIndex)
            found.append({cid, c.value("name").toString()});
    }
    std::sort(found.begin(), found.end(), [](const G &a, const G &b) { return a.name.compare(b.name, Qt::CaseInsensitive) < 0; });

    const bool byPath = siteIndex >= 0 && siteIndex < m_sites.size() && m_sites[siteIndex].groupBy == QLatin1String("path");
    QVariantList out;
    for (const G &g : found) {
        QVariantList links;
        for (const QString &id : m_core->queryIds(LH::ModeMembers, LH::AllCategories, QString(), g.id)) {
            const Item it = m_core->item(id);
            const QUrl u(it.target);
            QString path = u.path();
            if (path.startsWith('/'))
                path.remove(0, 1);
            links.append(QVariantMap{{"id", id}, {"name", it.name}, {"path", byPath ? path : u.host() + u.path()}});
        }
        out.append(QVariantMap{{"id", g.id}, {"name", g.name}, {"links", links}});
    }
    return out;
}

QStringList Sites::viewIds(int siteIndex, const QString &search) const
{
    const QString needle = search.trimmed();
    QStringList out;
    QSet<QString> seen;
    for (const QVariant &gv : groups(siteIndex)) {
        for (const QVariant &lv : gv.toMap().value("links").toList()) {
            const QString id = lv.toMap().value("id").toString();
            if (seen.contains(id))
                continue;
            seen.insert(id);
            if (!needle.isEmpty()) {
                const Item it = m_core->item(id);
                if (!it.name.contains(needle, Qt::CaseInsensitive) && !it.target.contains(needle, Qt::CaseInsensitive))
                    continue;
            }
            out << id;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// Listing and editing rules
// ---------------------------------------------------------------------------

QVariantList Sites::list() const
{
    QVector<int> groupCount(m_sites.size(), 0), linkCount(m_sites.size(), 0);
    for (int i = 0; i < m_sites.size(); ++i) {
        const QVariantList g = groups(i);
        groupCount[i] = g.size();
        for (const QVariant &gv : g)
            linkCount[i] += gv.toMap().value("links").toList().size();
    }
    QVariantList out;
    for (int i = 0; i < m_sites.size(); ++i) {
        const Site &s = m_sites[i];
        out.append(QVariantMap{{"index", i}, {"id", s.id}, {"name", s.name}, {"hosts", s.hosts},
                               {"hostsText", s.hosts.join(QStringLiteral(", "))}, {"groupBy", s.groupBy},
                               {"autoFile", s.autoFile}, {"icon", s.icon}, {"color", s.color},
                               {"groups", groupCount[i]}, {"links", linkCount[i]}});
    }
    return out;
}

QVariantList Sites::dockEntries() const
{
    QVariantList out;
    for (int i = 0; i < m_sites.size(); ++i)
        if (!groups(i).isEmpty())
            out.append(QVariantMap{{"name", m_sites[i].name}, {"cat", LH::SiteViewBase + i},
                                   {"icon", m_sites[i].icon}, {"col", m_sites[i].color}});
    return out;
}

bool Sites::saveSite(const QVariantMap &in)
{
    Site s;
    s.name = in.value("name").toString().trimmed();
    const QVariant h = in.value("hosts");
    const QStringList raw = h.typeId() == QMetaType::QString ? h.toString().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts)
                                                              : h.toStringList();
    for (const QString &x : raw) {
        const QString n = normalizeHost(x);
        if (!n.isEmpty() && !s.hosts.contains(n))
            s.hosts << n;
    }
    if (s.name.isEmpty() || s.hosts.isEmpty())
        return false;
    const QString gb = in.value("groupBy").toString();
    s.groupBy = (gb == QLatin1String("path") || gb == QLatin1String("subdomain")) ? gb : QStringLiteral("none");
    s.autoFile = in.value("autoFile", true).toBool();
    s.icon = in.value("icon", QStringLiteral("web")).toString();
    if (s.icon.isEmpty())
        s.icon = QStringLiteral("web");
    s.color = in.value("color", QStringLiteral("#4C8DFF")).toString();
    s.id = in.value("id").toString();

    bool replaced = false;
    for (Site &old : m_sites)
        if (!s.id.isEmpty() && old.id == s.id) {
            old = s;
            replaced = true;
        }
    if (!replaced) {
        s.id = QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
        m_sites.append(s);
    }
    save();
    emit changed();
    return true;
}

void Sites::removeSite(const QString &id)
{
    for (int i = 0; i < m_sites.size(); ++i)
        if (m_sites[i].id == id) {
            m_sites.removeAt(i);
            break;
        }
    save();
    emit changed();
}

void Sites::resetPresets()
{
    seedPresets(true);
    save();
    emit changed();
}

// ---------------------------------------------------------------------------
// Tidy-up of links that were added before a site existed
// ---------------------------------------------------------------------------

QMultiMap<QString, QString> Sites::looseByGroup() const
{
    QMultiMap<QString, QString> out;
    for (const QString &id : m_core->queryIds(LH::ModeCategory, LH::Web, QString(), QString())) {
        const Item it = m_core->item(id);
        QString host;
        const int idx = siteForUrl(it.target, &host);
        if (idx < 0 || !m_sites[idx].autoFile)
            continue;
        const QString name = groupNameFor(m_sites[idx], it.target, host);
        if (!name.isEmpty())
            out.insert(name, id);
    }
    return out;
}

int Sites::looseCount() const { return looseByGroup().size(); }

int Sites::organizeAll()
{
    const QMultiMap<QString, QString> loose = looseByGroup();
    int moved = 0;
    for (const QString &name : loose.uniqueKeys()) {
        const QString col = findOrCreateGroup(name);
        if (col.isEmpty())
            continue;
        const QStringList ids = loose.values(name);
        m_core->addToCollection(col, ids);
        moved += ids.size();
    }
    if (moved)
        m_core->message(QStringLiteral("Organized %1 link(s) into their sites.").arg(moved));
    return moved;
}

// ---------------------------------------------------------------------------
// Suggestions
// ---------------------------------------------------------------------------

QVariantList Sites::suggestions() const
{
    QMap<QString, int> counts;
    for (const QString &id : m_core->queryIds(LH::ModeCategory, LH::Web, QString(), QString())) {
        const QString target = m_core->item(id).target;
        if (siteForUrl(target) >= 0)
            continue;
        const QUrl u(target);
        if (u.scheme() != QLatin1String("http") && u.scheme() != QLatin1String("https"))
            continue;
        const QString host = normalizeHost(u.host());
        if (!host.isEmpty() && !m_dismissed.contains(host))
            counts[host]++;
    }
    QVector<QPair<int, QString>> ranked;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        if (it.value() >= kSuggestMin)
            ranked.append({it.value(), it.key()});
    std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) { return a.first > b.first; });

    QVariantList out;
    for (const auto &r : ranked.mid(0, 3)) {
        const QStringList parts = r.second.split('.');
        QString label = parts.size() >= 2 ? parts[parts.size() - 2] : r.second;
        if (!label.isEmpty())
            label[0] = label[0].toUpper();
        out.append(QVariantMap{{"host", r.second}, {"name", label}, {"count", r.first}});
    }
    return out;
}

bool Sites::acceptSuggestion(const QString &host)
{
    const QString h = normalizeHost(host);
    const QStringList parts = h.split('.');
    QString label = parts.size() >= 2 ? parts[parts.size() - 2] : h;
    if (!label.isEmpty())
        label[0] = label[0].toUpper();
    const int n = int(sizeof(kPalette) / sizeof(kPalette[0]));
    if (!saveSite({{"name", label}, {"hosts", h}, {"groupBy", "none"}, {"icon", "web"},
                   {"color", QString::fromLatin1(kPalette[qHash(h) % n])}, {"autoFile", true}}))
        return false;
    organizeAll();
    return true;
}

void Sites::dismissSuggestion(const QString &host)
{
    m_dismissed.insert(normalizeHost(host));
    save();
    emit changed();
}
