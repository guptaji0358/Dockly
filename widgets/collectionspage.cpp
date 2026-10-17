#include "collectionspage.h"

#include "appcore.h"
#include "itemactions.h"
#include "itemmodel.h"
#include "itemview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CollectionsPage::CollectionsPage(AppCore *core, QWidget *parent)
    : QWidget(parent), m_core(core)
{
    setObjectName("page");
    m_collections = new ItemModel(this);
    m_collections->setCategory(LH::Collection);
    m_members = new ItemModel(this);
    m_members->setMode(LH::ModeMembers);

    auto *heading = new QLabel("Collections");
    heading->setObjectName("h1");
    auto *create = new QPushButton("Create Collection");
    create->setProperty("primary", true);

    auto *top = new QHBoxLayout;
    top->addWidget(heading);
    top->addStretch(1);
    top->addWidget(create);

    // Left: collections
    m_collectionView = new ItemView(core);
    m_collectionView->setForcedViewStyle("list");
    m_collectionView->setItemModel(m_collections);
    m_collectionView->setEmptyText("No collections yet.\nCreate one to launch several items at once.");
    m_collectionView->setFixedWidth(300);

    // Right: members
    m_title = new QLabel;
    m_title->setObjectName("h2");
    m_launchAll = new QPushButton("Launch All");
    m_launchAll->setProperty("primary", true);
    m_addCards = new QPushButton("Add Existing Cards");
    m_rename = new QPushButton("Edit");
    m_delete = new QPushButton("Delete");
    m_delete->setProperty("danger", true);

    auto *rtop = new QHBoxLayout;
    rtop->addWidget(m_title, 1);
    rtop->addWidget(m_launchAll);
    rtop->addWidget(m_addCards);
    rtop->addWidget(m_rename);
    rtop->addWidget(m_delete);

    m_memberView = new ItemView(core);
    m_memberView->setItemModel(m_members);

    auto *right = new QVBoxLayout;
    right->addLayout(rtop);
    right->addWidget(m_memberView, 1);

    auto *body = new QHBoxLayout;
    body->addWidget(m_collectionView);
    body->addSpacing(12);
    body->addLayout(right, 1);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addLayout(top);
    lay->addSpacing(6);
    lay->addLayout(body, 1);

    connect(create, &QPushButton::clicked, this, [this] {
        const QString id = ItemActions::addCollection(this, m_core);
        if (!id.isEmpty())
            select(id);
    });
    connect(m_launchAll, &QPushButton::clicked, this, [this] { m_core->launchAll(m_current); });
    connect(m_addCards, &QPushButton::clicked, this, [this] { ItemActions::addExistingToCollection(this, m_core, m_current); });
    connect(m_rename, &QPushButton::clicked, this, [this] { ItemActions::edit(this, m_core, m_current); });
    connect(m_delete, &QPushButton::clicked, this, [this] { ItemActions::remove(this, m_core, m_current); });

    connect(m_collectionView->selectionModel(), &QItemSelectionModel::currentChanged, this, [this] { selectionChanged(); });
    ItemActions::connectView(m_collectionView, this, m_core);
    connect(m_collectionView, &ItemView::launchRequested, this, [this](const QString &id) { m_core->launchAll(id); });
    ItemActions::connectView(m_memberView, this, m_core, [this] { return m_current; }, true);

    // Drag & drop: drop any card on a collection to add it; drag members to reorder them.
    m_collectionView->setDropMode(ItemView::DropOntoCollections);
    connect(m_collectionView, &ItemView::droppedOnCollection, this, [this](const QString &item, const QString &coll) {
        m_core->addToCollection(coll, {item});
    });
    connect(m_memberView, &ItemView::reorderRequested, this, [this](const QString &item, int row) {
        m_core->moveMember(m_current, item, row);
    });

    // Keep the selection valid as collections come and go, and refresh the title/member count.
    connect(m_collections, &QAbstractItemModel::modelReset, this, [this] {
        if (m_current.isEmpty() || !m_core->exists(m_current))
            select(m_collections->rowCount() ? m_collections->idAt(0) : QString());
        else
            select(m_current);
    });
    connect(m_core, &AppCore::itemsChanged, this, [this] { select(m_current); });
    select(m_collections->rowCount() ? m_collections->idAt(0) : QString());
}

void CollectionsPage::selectionChanged()
{
    const QModelIndex i = m_collectionView->currentIndex();
    if (!i.isValid())
        return;
    const QString id = i.data(ItemModel::IdRole).toString();
    if (id != m_current)
        select(id);
}

void CollectionsPage::select(const QString &id)
{
    const Item c = m_core->item(id);
    const bool valid = !c.id.isEmpty() && c.category == LH::Collection;
    m_current = valid ? id : QString();
    m_members->setCollectionId(m_current);
    m_memberView->setDropMode(m_current.isEmpty() ? ItemView::NoDrops : ItemView::ReorderMembers, m_current);
    m_memberView->setDragSource(m_current);

    for (QWidget *w : {static_cast<QWidget *>(m_launchAll), static_cast<QWidget *>(m_addCards),
                       static_cast<QWidget *>(m_rename), static_cast<QWidget *>(m_delete)})
        w->setEnabled(valid);
    m_launchAll->setEnabled(valid && c.memberCount > 0);

    if (!valid) {
        m_title->setText("Select or create a collection");
        m_memberView->setEmptyText({});
        return;
    }
    m_title->setText(QStringLiteral("%1  •  %2 item(s)").arg(c.name).arg(c.memberCount));
    m_memberView->setEmptyText("This collection is empty.\nUse \"Add Existing Cards\" to fill it.");

    // Reflect the selection in the left list without re-entering.
    for (int r = 0; r < m_collections->rowCount(); ++r) {
        if (m_collections->idAt(r) == id) {
            if (m_collectionView->currentIndex().row() != r) {
                QSignalBlocker b(m_collectionView->selectionModel());
                m_collectionView->setCurrentIndex(m_collections->index(r));
            }
            break;
        }
    }
}
