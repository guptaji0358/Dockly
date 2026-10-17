#pragma once

#include "item.h"
#include "softwarescanner.h"

#include <QHash>
#include <QJsonObject>
#include <QSqlDatabase>
#include <QVariantList>

// Two SQLite files, queried on demand so nothing large lives in RAM:
//   library.db - the user's cards, collection membership, recents
//   catalog.db - hidden catalog of detected software (only filled on request)
class Database
{
public:
    ~Database();
    void close(); // safe to call more than once
    bool open(const QString &dir);
    bool libraryIsNew() const { return m_libraryIsNew; }

    // --- library: items --------------------------------------------------
    bool getItem(const QString &id, Item &out) const;
    QStringList queryIds(int mode, int category, const QString &search, const QString &collectionId,
                         int recentLimit) const;
    bool insertItem(const Item &it);
    bool updateItem(const Item &it);
    void setFavorite(const QString &id, bool favorite);
    void setLastOpened(const QString &id);
    void deleteItem(const QString &id);
    QString findByNorm(int category, const QString &norm) const;
    int itemCount() const;

    // --- library: collections / recents ----------------------------------
    QStringList members(const QString &collectionId) const;
    int memberCount(const QString &collectionId) const;
    bool addMember(const QString &collectionId, const QString &itemId);
    void removeMember(const QString &collectionId, const QString &itemId);
    void moveMember(const QString &collectionId, const QString &itemId, int toRow);
    QVariantList collections() const;
    QVariantList addable(const QString &collectionId, int limit) const;
    void touchRecent(const QString &id);
    void trimRecents(int cap);

    // --- validation -------------------------------------------------------
    QVector<Item> checkRows() const;                  // id/category/target/status only
    bool applyStatuses(const QHash<QString, int> &s); // returns true if anything changed

    // --- catalog ------------------------------------------------------------
    int replaceCatalog(const QVector<ScannedApp> &apps);
    QVariantList catalog(const QString &search, int limit) const;
    int catalogCount() const;

    // --- backup -------------------------------------------------------------
    QJsonObject exportAll() const;
    bool importAll(const QJsonObject &data); // replaces library contents

private:
    QSqlDatabase lib() const { return QSqlDatabase::database(QStringLiteral("lh_library")); }
    QSqlDatabase cat() const { return QSqlDatabase::database(QStringLiteral("lh_catalog")); }
    static QString likePattern(const QString &s);

    bool m_libraryIsNew = false;
    bool m_open = false;
};
