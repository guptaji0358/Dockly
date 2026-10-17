#include "itemmodel.h"
#include "appcore.h"


ItemModel::ItemModel(QObject *parent)
    : QAbstractListModel(parent), m_core(AppCore::instance())
{
    connect(m_core, &AppCore::itemsChanged, this, &ItemModel::rebuild);
    connect(m_core, &AppCore::iconsChanged, this, [this] {
        if (!m_ids.isEmpty())
            emit dataChanged(index(0), index(m_ids.size() - 1));
    });
    rebuild();
}

int ItemModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_ids.size();
}

QHash<int, QByteArray> ItemModel::roleNames() const
{
    return {
        {IdRole, "itemId"},
        {NameRole, "name"},
        {TargetRole, "target"},
        {CategoryRole, "category"},
        {FavoriteRole, "favorite"},
        {StatusRole, "status"},
        {IconSourceRole, "iconSource"},
        {CategoryNameRole, "categoryName"},
        {SubtitleRole, "subtitle"},
    };
}

QVariant ItemModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_ids.size())
        return {};
    const Item item = m_core->item(m_ids.at(index.row()));
    if (item.id.isEmpty())
        return {};
    const Item *it = &item;
    switch (role) {
    case Qt::DisplayRole:
    case NameRole: return it->name;
    case Qt::ToolTipRole: return it->target.isEmpty() ? it->name : it->target;
    case IdRole: return it->id;
    case TargetRole: return it->target;
    case CategoryRole: return int(it->category);
    case FavoriteRole: return it->favorite;
    case StatusRole: return int(it->status);
    case IconSourceRole:
        return QStringLiteral("image://item/%1?v=%2").arg(it->id).arg(m_core->iconVersion());
    case CategoryNameRole: return Item::categoryLabel(it->category);
    case SubtitleRole: return m_core->subtitleFor(*it);
    }
    return {};
}

void ItemModel::setCategory(int c)
{
    if (m_category == c)
        return;
    m_category = c;
    rebuild();
    emit filterChanged();
}

void ItemModel::setMode(int m)
{
    if (m_mode == m)
        return;
    m_mode = m;
    rebuild();
    emit filterChanged();
}

void ItemModel::setSearch(const QString &s)
{
    if (m_search == s)
        return;
    m_search = s;
    rebuild();
    emit filterChanged();
}

void ItemModel::setCollectionId(const QString &id)
{
    if (m_collectionId == id)
        return;
    m_collectionId = id;
    rebuild();
    emit filterChanged();
}

void ItemModel::rebuild()
{
    // Only ids are held in memory; each row is loaded from SQLite when a view asks for it.
    const QStringList next = m_core->queryIds(m_mode, m_category, m_search, m_collectionId);
    if (next == m_ids) {
        if (!m_ids.isEmpty())
            emit dataChanged(index(0), index(m_ids.size() - 1));
        return;
    }
    const int before = m_ids.size();
    beginResetModel();
    m_ids = next;
    endResetModel();
    if (before != m_ids.size())
        emit countChanged();
}
