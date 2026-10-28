#include "database.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QSet>
#include <QSqlError>
#include <QSqlQuery>
#include <QVariantMap>

Database::~Database() { close(); }

void Database::close()
{
    if (!m_open)
        return; // also avoids touching QtSql after QCoreApplication is gone
    m_open = false;
    for (const char *name : {"lh_library", "lh_catalog"}) {
        {
            QSqlDatabase db = QSqlDatabase::database(QLatin1String(name), false);
            if (db.isValid())
                db.close();
        }
        QSqlDatabase::removeDatabase(QLatin1String(name));
    }
}

bool Database::open(const QString &dir)
{
    const QString libPath = dir + "/library.db";
    m_libraryIsNew = !QFile::exists(libPath);

    QSqlDatabase l = QSqlDatabase::addDatabase("QSQLITE", "lh_library");
    l.setDatabaseName(libPath);
    QSqlDatabase c = QSqlDatabase::addDatabase("QSQLITE", "lh_catalog");
    c.setDatabaseName(dir + "/catalog.db");
    if (!l.open() || !c.open())
        return false;
    m_open = true;

    QSqlQuery q(l);
    q.exec("PRAGMA journal_mode=WAL");
    q.exec("PRAGMA synchronous=NORMAL");
    q.exec("PRAGMA foreign_keys=ON");
    q.exec("CREATE TABLE IF NOT EXISTS items ("
           " id TEXT PRIMARY KEY, category INTEGER NOT NULL, name TEXT NOT NULL, target TEXT NOT NULL DEFAULT '',"
           " norm TEXT NOT NULL DEFAULT '', favorite INTEGER NOT NULL DEFAULT 0,"
           " last_opened TEXT, status INTEGER NOT NULL DEFAULT 0)");
    q.exec("ALTER TABLE items ADD COLUMN icon TEXT NOT NULL DEFAULT ''"); // fails harmlessly when it exists
    q.exec("CREATE INDEX IF NOT EXISTS idx_items_cat ON items(category, name COLLATE NOCASE)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_items_norm ON items(category, norm)");
    q.exec("CREATE INDEX IF NOT EXISTS idx_items_fav ON items(favorite)");
    q.exec("CREATE TABLE IF NOT EXISTS members ("
           " collection_id TEXT NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
           " item_id TEXT NOT NULL REFERENCES items(id) ON DELETE CASCADE,"
           " pos INTEGER NOT NULL, PRIMARY KEY(collection_id, item_id))");
    q.exec("CREATE TABLE IF NOT EXISTS recents ("
           " item_id TEXT PRIMARY KEY REFERENCES items(id) ON DELETE CASCADE, seq INTEGER NOT NULL)");

    QSqlQuery cq(c);
    cq.exec("PRAGMA journal_mode=WAL");
    cq.exec("CREATE TABLE IF NOT EXISTS apps (target TEXT PRIMARY KEY, name TEXT NOT NULL)");
    cq.exec("CREATE INDEX IF NOT EXISTS idx_apps_name ON apps(name COLLATE NOCASE)");
    return true;
}

QString Database::likePattern(const QString &s)
{
    QString e = s;
    e.replace('\\', "\\\\").replace('%', "\\%").replace('_', "\\_");
    return '%' + e + '%';
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------

static Item rowToItem(QSqlQuery &q)
{
    Item it;
    it.id = q.value(0).toString();
    it.category = LH::Category(q.value(1).toInt());
    it.name = q.value(2).toString();
    it.target = q.value(3).toString();
    it.favorite = q.value(4).toInt() != 0;
    it.lastOpened = QDateTime::fromString(q.value(5).toString(), Qt::ISODate);
    it.status = LH::Status(q.value(6).toInt());
    it.icon = q.value(7).toString();
    return it;
}

bool Database::getItem(const QString &id, Item &out) const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("SELECT id,category,name,target,favorite,last_opened,status,icon FROM items WHERE id=?");
    q.addBindValue(id);
    if (!q.exec() || !q.next())
        return false;
    out = rowToItem(q);
    return true;
}

QStringList Database::queryIds(int mode, int category, const QString &search, const QString &collectionId,
                               int recentLimit) const
{
    if (!m_open)
        return {};
    QString sql;
    QVariantList binds;
    QString where;
    const QString needle = search.trimmed();
    auto addSearch = [&](const QString &prefix) {
        if (needle.isEmpty())
            return;
        where += QStringLiteral(" AND (%1name LIKE ? ESCAPE '\\' OR %1target LIKE ? ESCAPE '\\')").arg(prefix);
        binds << likePattern(needle) << likePattern(needle);
    };

    switch (mode) {
    case LH::ModeRecent:
        sql = "SELECT i.id FROM recents r JOIN items i ON i.id=r.item_id WHERE 1=1";
        addSearch("i.");
        sql += where + " ORDER BY r.seq DESC LIMIT " + QString::number(qMax(0, recentLimit));
        break;
    case LH::ModeMembers:
        sql = "SELECT i.id FROM members m JOIN items i ON i.id=m.item_id WHERE m.collection_id=?";
        binds.prepend(collectionId);
        addSearch("i.");
        sql += where + " ORDER BY m.pos";
        break;
    case LH::ModeFavorites:
        sql = "SELECT id FROM items WHERE favorite=1";
        addSearch(QString());
        sql += where + " ORDER BY name COLLATE NOCASE";
        break;
    default:
        sql = "SELECT id FROM items WHERE 1=1";
        if (category != LH::AllCategories) {
            sql += " AND category=?";
            binds << category;
        }
        // web shortcuts grouped into a collection (e.g. a site's links) live in that collection,
        // not in the loose Web list; searching still finds them.
        // (Site groups have their own page and dock icon: AppCore::queryIds drops them from the Collections list.)
        if (category == LH::Web && needle.isEmpty())
            sql += " AND id NOT IN (SELECT item_id FROM members)";
        addSearch(QString());
        sql += where + " ORDER BY name COLLATE NOCASE";
        break;
    }

    QSqlQuery q(lib());
    q.prepare(sql);
    for (const QVariant &b : binds)
        q.addBindValue(b);
    QStringList ids;
    if (q.exec())
        while (q.next())
            ids << q.value(0).toString();
    return ids;
}

bool Database::insertItem(const Item &it)
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("INSERT INTO items(id,category,name,target,norm,favorite,last_opened,status,icon) VALUES(?,?,?,?,?,?,?,?,?)");
    q.addBindValue(it.id);
    q.addBindValue(int(it.category));
    q.addBindValue(it.name);
    q.addBindValue(it.target.isNull() ? QStringLiteral("") : it.target); // never bind NULL
    q.addBindValue(it.category == LH::Collection ? QStringLiteral("") : Item::normalizedTarget(it.target));
    q.addBindValue(it.favorite ? 1 : 0);
    q.addBindValue(it.lastOpened.isValid() ? QVariant(it.lastOpened.toString(Qt::ISODate)) : QVariant(QMetaType(QMetaType::QString)));
    q.addBindValue(int(it.status));
    q.addBindValue(it.icon.isNull() ? QStringLiteral("") : it.icon); // never bind NULL
    return q.exec();
}

bool Database::updateItem(const Item &it)
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("UPDATE items SET name=?,target=?,norm=?,status=? WHERE id=?");
    q.addBindValue(it.name);
    q.addBindValue(it.target.isNull() ? QStringLiteral("") : it.target); // never bind NULL
    q.addBindValue(it.category == LH::Collection ? QStringLiteral("") : Item::normalizedTarget(it.target));
    q.addBindValue(int(it.status));
    q.addBindValue(it.id);
    return q.exec();
}

void Database::setIcon(const QString &id, const QString &icon)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("UPDATE items SET icon=? WHERE id=?");
    q.addBindValue(icon.isNull() ? QStringLiteral("") : icon); // never bind NULL
    q.addBindValue(id);
    q.exec();
}

void Database::setFavorite(const QString &id, bool favorite)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("UPDATE items SET favorite=? WHERE id=?");
    q.addBindValue(favorite ? 1 : 0);
    q.addBindValue(id);
    q.exec();
}

void Database::setLastOpened(const QString &id)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("UPDATE items SET last_opened=? WHERE id=?");
    q.addBindValue(QDateTime::currentDateTime().toString(Qt::ISODate));
    q.addBindValue(id);
    q.exec();
}

void Database::deleteItem(const QString &id)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("DELETE FROM items WHERE id=?"); // members / recents cascade
    q.addBindValue(id);
    q.exec();
}

QString Database::findByNorm(int category, const QString &norm) const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("SELECT id FROM items WHERE category=? AND norm=? LIMIT 1");
    q.addBindValue(category);
    q.addBindValue(norm);
    return q.exec() && q.next() ? q.value(0).toString() : QString();
}

int Database::itemCount() const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    return q.exec("SELECT COUNT(*) FROM items") && q.next() ? q.value(0).toInt() : 0;
}

// ---------------------------------------------------------------------------
// Collections / recents
// ---------------------------------------------------------------------------

QStringList Database::members(const QString &collectionId) const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("SELECT item_id FROM members WHERE collection_id=? ORDER BY pos");
    q.addBindValue(collectionId);
    QStringList out;
    if (q.exec())
        while (q.next())
            out << q.value(0).toString();
    return out;
}

int Database::memberCount(const QString &collectionId) const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("SELECT COUNT(*) FROM members WHERE collection_id=?");
    q.addBindValue(collectionId);
    return q.exec() && q.next() ? q.value(0).toInt() : 0;
}

bool Database::addMember(const QString &collectionId, const QString &itemId)
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("INSERT OR IGNORE INTO members(collection_id,item_id,pos) "
              "VALUES(?,?,(SELECT COALESCE(MAX(pos),0)+1 FROM members WHERE collection_id=?))");
    q.addBindValue(collectionId);
    q.addBindValue(itemId);
    q.addBindValue(collectionId);
    const bool ok = q.exec();
    return ok && q.numRowsAffected() > 0;
}

void Database::removeMember(const QString &collectionId, const QString &itemId)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("DELETE FROM members WHERE collection_id=? AND item_id=?");
    q.addBindValue(collectionId);
    q.addBindValue(itemId);
    q.exec();
}

void Database::moveMember(const QString &collectionId, const QString &itemId, int toRow)
{
    if (!m_open)
        return;
    QStringList order = members(collectionId);
    const int from = order.indexOf(itemId);
    if (from < 0)
        return;
    order.removeAt(from);
    order.insert(qBound(0, toRow, order.size()), itemId);

    QSqlDatabase db = lib();
    db.transaction();
    QSqlQuery q(db);
    q.prepare("UPDATE members SET pos=? WHERE collection_id=? AND item_id=?");
    for (int i = 0; i < order.size(); ++i) {
        q.addBindValue(i + 1);
        q.addBindValue(collectionId);
        q.addBindValue(order[i]);
        q.exec();
    }
    db.commit();
}

QVariantList Database::collections() const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.exec("SELECT id,name FROM items WHERE category=4 ORDER BY name COLLATE NOCASE");
    QVariantList out;
    while (q.next())
        out.append(QVariantMap{{"id", q.value(0)}, {"name", q.value(1)}});
    return out;
}

QVariantList Database::addable(const QString &collectionId, int limit) const
{
    if (!m_open)
        return {};
    QSqlQuery q(lib());
    q.prepare("SELECT id,name,category FROM items WHERE category<>4 AND id NOT IN "
              "(SELECT item_id FROM members WHERE collection_id=?) ORDER BY name COLLATE NOCASE LIMIT ?");
    q.addBindValue(collectionId);
    q.addBindValue(limit);
    QVariantList out;
    if (q.exec())
        while (q.next())
            out.append(QVariantMap{{"id", q.value(0)},
                                   {"name", q.value(1)},
                                   {"category", Item::categoryLabel(LH::Category(q.value(2).toInt()))}});
    return out;
}

void Database::touchRecent(const QString &id)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("INSERT OR REPLACE INTO recents(item_id,seq) VALUES(?,(SELECT COALESCE(MAX(seq),0)+1 FROM recents))");
    q.addBindValue(id);
    q.exec();
}

void Database::trimRecents(int cap)
{
    if (!m_open)
        return;
    QSqlQuery q(lib());
    q.prepare("DELETE FROM recents WHERE item_id NOT IN (SELECT item_id FROM recents ORDER BY seq DESC LIMIT ?)");
    q.addBindValue(qMax(0, cap));
    q.exec();
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

QVector<Item> Database::checkRows() const
{
    if (!m_open)
        return {};
    QVector<Item> out;
    QSqlQuery q(lib());
    q.exec("SELECT id,category,target,status FROM items WHERE category<>4");
    while (q.next()) {
        Item it;
        it.id = q.value(0).toString();
        it.category = LH::Category(q.value(1).toInt());
        it.target = q.value(2).toString();
        it.status = LH::Status(q.value(3).toInt());
        out.append(it);
    }
    return out;
}

bool Database::applyStatuses(const QHash<QString, int> &s)
{
    if (!m_open)
        return {};
    bool changed = false;
    QSqlDatabase db = lib();
    db.transaction();
    QSqlQuery q(db);
    q.prepare("UPDATE items SET status=? WHERE id=? AND status<>?");
    for (auto it = s.cbegin(); it != s.cend(); ++it) {
        q.addBindValue(it.value());
        q.addBindValue(it.key());
        q.addBindValue(it.value());
        q.exec();
        changed = changed || q.numRowsAffected() > 0;
    }
    db.commit();
    return changed;
}

// ---------------------------------------------------------------------------
// Catalog
// ---------------------------------------------------------------------------

int Database::replaceCatalog(const QVector<ScannedApp> &apps)
{
    if (!m_open)
        return {};
    QSqlDatabase db = cat();
    db.transaction();
    QSqlQuery q(db);
    q.exec("DELETE FROM apps");
    q.prepare("INSERT OR IGNORE INTO apps(target,name) VALUES(?,?)");
    for (const ScannedApp &a : apps) {
        q.addBindValue(a.target);
        q.addBindValue(a.name);
        q.exec();
    }
    db.commit();
    return catalogCount();
}

QVariantList Database::catalog(const QString &search, int limit) const
{
    if (!m_open)
        return {};
    QSqlQuery q(cat());
    const QString needle = search.trimmed();
    if (needle.isEmpty()) {
        q.prepare("SELECT name,target FROM apps ORDER BY name COLLATE NOCASE LIMIT ?");
    } else {
        q.prepare("SELECT name,target FROM apps WHERE name LIKE ? ESCAPE '\\' ORDER BY name COLLATE NOCASE LIMIT ?");
        q.addBindValue(likePattern(needle));
    }
    q.addBindValue(limit);
    QVariantList out;
    if (q.exec())
        while (q.next())
            out.append(QVariantMap{{"name", q.value(0)}, {"target", q.value(1)}});
    return out;
}

int Database::catalogCount() const
{
    if (!m_open)
        return {};
    QSqlQuery q(cat());
    return q.exec("SELECT COUNT(*) FROM apps") && q.next() ? q.value(0).toInt() : 0;
}

// ---------------------------------------------------------------------------
// Backup
// ---------------------------------------------------------------------------

QJsonObject Database::exportAll() const
{
    if (!m_open)
        return {};
    QJsonArray items;
    QSqlQuery q(lib());
    q.exec("SELECT id,category,name,target,favorite,last_opened,status,icon FROM items ORDER BY rowid");
    while (q.next()) {
        Item it = rowToItem(q);
        if (it.category == LH::Collection)
            it.members = members(it.id);
        items.append(it.toJson());
    }
    QJsonArray recent;
    QSqlQuery r(lib());
    r.exec("SELECT item_id FROM recents ORDER BY seq DESC");
    while (r.next())
        recent.append(r.value(0).toString());
    QJsonObject o;
    o["version"] = 2;
    o["items"] = items;
    o["recent"] = recent;
    return o;
}

bool Database::importAll(const QJsonObject &data)
{
    if (!m_open)
        return {};
    QSqlDatabase db = lib();
    if (!db.transaction())
        return false;
    QSqlQuery q(db);
    q.exec("DELETE FROM recents");
    q.exec("DELETE FROM members");
    q.exec("DELETE FROM items");

    QList<Item> collections;
    QSet<QString> ids;
    for (const QJsonValue &v : data["items"].toArray()) {
        Item it = Item::fromJson(v.toObject());
        if (it.id.isEmpty() || ids.contains(it.id))
            continue;
        ids.insert(it.id);
        insertItem(it);
        if (it.category == LH::Collection)
            collections.append(it);
    }
    for (const Item &c : collections)
        for (const QString &m : c.members)
            if (ids.contains(m))
                addMember(c.id, m);

    const QJsonArray recent = data["recent"].toArray();
    for (int i = recent.size() - 1; i >= 0; --i) // oldest first so seq order matches
        if (ids.contains(recent[i].toString()))
            touchRecent(recent[i].toString());
    return db.commit();
}
