#pragma once

#include "lhtypes.h"

#include <QAbstractListModel>
#include <QStringList>

class AppCore;

// Filtered, sorted view over AppCore's items. Shared by both frontends.
class ItemModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int category READ category WRITE setCategory NOTIFY filterChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY filterChanged)
    Q_PROPERTY(QString search READ search WRITE setSearch NOTIFY filterChanged)
    Q_PROPERTY(QString collectionId READ collectionId WRITE setCollectionId NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        TargetRole,
        CategoryRole,
        FavoriteRole,
        StatusRole,
        IconSourceRole,
        CategoryNameRole,
        SubtitleRole,
    };

    explicit ItemModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int category() const { return m_category; }
    void setCategory(int c);
    int mode() const { return m_mode; }
    void setMode(int m);
    QString search() const { return m_search; }
    void setSearch(const QString &s);
    QString collectionId() const { return m_collectionId; }
    void setCollectionId(const QString &id);
    int count() const { return m_ids.size(); }

    Q_INVOKABLE QString idAt(int row) const { return m_ids.value(row); }
    Q_INVOKABLE int rowOf(const QString &id) const { return m_ids.indexOf(id); }

signals:
    void filterChanged();
    void countChanged();

private:
    void rebuild();

    AppCore *m_core;
    int m_category = LH::AllCategories;
    int m_mode = LH::ModeCategory;
    QString m_search;
    QString m_collectionId;
    QStringList m_ids;
};
