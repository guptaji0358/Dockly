#include "itemview.h"

#include "appcore.h"
#include "itemdelegate.h"
#include "itemmodel.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDrag>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPainter>

namespace {
const char *kItemMime = "application/x-launcherhub-item";
constexpr int kEnterMs = 280;     // per-card entrance duration
constexpr int kStaggerMs = 35;    // delay between consecutive cards
}

ItemView::ItemView(AppCore *core, QWidget *parent)
    : QListView(parent), m_core(core), m_delegate(new ItemDelegate(core, this))
{
    // OpenGL-backed viewport (smooth, GPU composited). LH_NO_GL=1 falls back to the raster engine.
    if (qEnvironmentVariableIsEmpty("LH_NO_GL"))
        setViewport(new QOpenGLWidget);

    setItemDelegate(m_delegate);
    setSelectionMode(SingleSelection);
    setEditTriggers(NoEditTriggers);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setResizeMode(Adjust);
    setMovement(Static);
    setFrameShape(NoFrame);
    setVerticalScrollMode(ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setContextMenuPolicy(Qt::DefaultContextMenu);
    setAcceptDrops(true);
    setDropIndicatorShown(false);

    m_delegate->hoverProgress = [this](const QModelIndex &i) { return hoverOf(i); };
    m_delegate->enterProgress = [this](const QModelIndex &i) { return enterOf(i); };
    m_anim.setInterval(16);
    connect(&m_anim, &QTimer::timeout, this, &ItemView::tick);

    connect(this, &QListView::doubleClicked, this, [this](const QModelIndex &i) {
        emit launchRequested(i.data(ItemModel::IdRole).toString());
    });
    // A single click selects the card and fills the detail panel.
    connect(this, &QListView::clicked, this, [this](const QModelIndex &i) {
        m_core->select(i.data(ItemModel::IdRole).toString());
    });
    connect(m_core->settings(), &AppSettings::valuesChanged, this, &ItemView::applySettings);
    connect(m_core->theme(), &Theme::changed, this, [this] { viewport()->update(); });
    applySettings();
}

void ItemView::setItemModel(ItemModel *model)
{
    m_model = model;
    setModel(model);
    connect(model, &QAbstractItemModel::modelReset, this, &ItemView::startEnter);
    connect(model, &QAbstractItemModel::rowsInserted, this, &ItemView::startEnter);
    startEnter();
}

void ItemView::setForcedViewStyle(const QString &style)
{
    m_forcedStyle = style;
    m_delegate->setForcedStyle(style);
    applySettings();
}

void ItemView::setDropMode(DropMode mode, const QString &collectionId)
{
    m_dropMode = mode;
    m_dropCollection = collectionId;
}

QString ItemView::currentId() const
{
    return currentIndex().isValid() ? currentIndex().data(ItemModel::IdRole).toString() : QString();
}

void ItemView::applySettings()
{
    const QString style = m_delegate->viewStyle();
    if (style == "grid") {
        setViewMode(IconMode);
        setMovement(Static);
        setResizeMode(Adjust);
        setWrapping(true);
        setFlow(LeftToRight);
        setSpacing(8);
        setUniformItemSizes(true);
    } else {
        setViewMode(ListMode);
        setWrapping(false);
        setFlow(TopToBottom);
        setSpacing(style == "compact" ? 2 : 4);
        setUniformItemSizes(true);
    }
    // setViewMode() resets drag/drop related state on the view.
    setAcceptDrops(true);
    setDragEnabled(false);
    setDropIndicatorShown(false);
    m_delegate->setViewportWidth(viewport()->width() - 12);
    doItemsLayout();
    viewport()->update();
}

// ---------------------------------------------------------------------------
// Animation
// ---------------------------------------------------------------------------

qreal ItemView::hoverOf(const QModelIndex &i) const
{
    return m_hover.value(i.data(ItemModel::IdRole).toString(), 0.0);
}

qreal ItemView::enterOf(const QModelIndex &i) const
{
    if (!m_entering)
        return 1.0;
    const qint64 t = m_enterClock.elapsed() - qint64(qMin(i.row(), 24)) * kStaggerMs;
    return qBound<qreal>(0.0, qreal(t) / kEnterMs, 1.0);
}

void ItemView::setHoverId(const QString &id)
{
    if (id == m_hoverId)
        return;
    m_hoverId = id;
    if (!m_core->settings()->boolean("animations")) {
        m_hover.clear();
        if (!id.isEmpty())
            m_hover.insert(id, 1.0);
        viewport()->update();
        return;
    }
    if (!id.isEmpty() && !m_hover.contains(id))
        m_hover.insert(id, 0.0);
    if (!m_anim.isActive())
        m_anim.start();
}

void ItemView::startEnter()
{
    if (!m_core->settings()->boolean("animations") || !isVisible()) {
        m_entering = false;
        return;
    }
    m_entering = true;
    m_enterClock.start();
    if (!m_anim.isActive())
        m_anim.start();
}

void ItemView::tick()
{
    constexpr qreal step = 16.0 / 140.0; // ~140 ms hover transition
    bool busy = false;
    for (auto it = m_hover.begin(); it != m_hover.end();) {
        const qreal target = it.key() == m_hoverId ? 1.0 : 0.0;
        qreal v = it.value();
        v += v < target ? step : -step;
        v = qBound(0.0, v, 1.0);
        if (v == 0.0 && target == 0.0) {
            it = m_hover.erase(it);
            continue;
        }
        it.value() = v;
        busy = busy || v != target;
        ++it;
    }
    if (m_entering) {
        const int rows = model() ? qMin(model()->rowCount(), 24) : 0;
        if (m_enterClock.elapsed() > kEnterMs + qint64(rows) * kStaggerMs)
            m_entering = false;
        else
            busy = true;
    }
    viewport()->update();
    if (!busy)
        m_anim.stop();
}

void ItemView::leaveEvent(QEvent *e)
{
    QListView::leaveEvent(e);
    setHoverId({});
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void ItemView::resizeEvent(QResizeEvent *e)
{
    QListView::resizeEvent(e);
    m_delegate->setViewportWidth(viewport()->width() - 12);
    if (m_delegate->viewStyle() != "grid")
        scheduleDelayedItemsLayout();
}

void ItemView::paintEvent(QPaintEvent *e)
{
    // A QOpenGLWidget viewport ignores the palette and clears to black: paint the page colour.
    {
        QPainter bg(viewport());
        bg.fillRect(e->rect(), m_core->theme()->bg());
    }
    QListView::paintEvent(e);
    QPainter p(viewport());
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    const Theme *t = m_core->theme();

    if (model() && model()->rowCount() == 0 && !m_empty.isEmpty()) {
        p.setPen(t->subtext());
        p.drawText(viewport()->rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap, m_empty);
    }

    // drag & drop feedback
    if (m_dropTarget.isValid()) {
        p.setPen(QPen(t->accent(), 3));
        QColor f = t->accent();
        f.setAlpha(40);
        p.setBrush(f);
        const int rad = t->radius() == 0 ? 0 : t->radius();
        p.drawRoundedRect(QRectF(visualRect(m_dropTarget)).adjusted(3, 3, -3, -3), rad, rad);
    } else if (m_dropRow >= 0 && model()) {
        // insertion bar before row m_dropRow (or after the last card)
        const bool grid = m_delegate->viewStyle() == "grid";
        QRect r;
        if (m_dropRow < model()->rowCount()) {
            r = visualRect(model()->index(m_dropRow, 0));
            r = grid ? QRect(r.left() - 2, r.top() + 4, 4, r.height() - 8) : QRect(r.left() + 4, r.top() - 2, r.width() - 8, 4);
        } else if (model()->rowCount() > 0) {
            r = visualRect(model()->index(model()->rowCount() - 1, 0));
            r = grid ? QRect(r.right() - 2, r.top() + 4, 4, r.height() - 8) : QRect(r.left() + 4, r.bottom() - 2, r.width() - 8, 4);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(t->accent());
        p.drawRoundedRect(r, 2, 2);
    }
}

// ---------------------------------------------------------------------------
// Mouse
// ---------------------------------------------------------------------------

int ItemView::buttonAt(const QPoint &pos, QModelIndex *index) const
{
    const QModelIndex i = indexAt(pos);
    if (index)
        *index = i;
    if (!i.isValid())
        return ItemDelegate::None;
    return m_delegate->hitButton(visualRect(i), pos);
}

void ItemView::mousePressEvent(QMouseEvent *e)
{
    m_pressPos = e->pos();
    m_pressIndex = e->button() == Qt::LeftButton ? indexAt(e->pos()) : QModelIndex();
    QListView::mousePressEvent(e);
}

void ItemView::mouseMoveEvent(QMouseEvent *e)
{
    QListView::mouseMoveEvent(e);
    const QModelIndex i = indexAt(e->pos());
    setHoverId(i.isValid() ? i.data(ItemModel::IdRole).toString() : QString());
    viewport()->setCursor(buttonAt(e->pos(), nullptr) != ItemDelegate::None ? Qt::PointingHandCursor : Qt::ArrowCursor);

    if ((e->buttons() & Qt::LeftButton) && m_pressIndex.isValid()
        && (e->pos() - m_pressPos).manhattanLength() >= QApplication::startDragDistance()
        && buttonAt(m_pressPos, nullptr) == ItemDelegate::None) {
        const QModelIndex idx = m_pressIndex;
        m_pressIndex = QModelIndex();
        startDrag(idx);
    }
}

void ItemView::mouseReleaseEvent(QMouseEvent *e)
{
    m_pressIndex = QModelIndex();
    if (e->button() == Qt::LeftButton) {
        QModelIndex i;
        const int b = buttonAt(e->pos(), &i);
        if (b != ItemDelegate::None) {
            const QString id = i.data(ItemModel::IdRole).toString();
            if (b == ItemDelegate::Edit)
                emit editRequested(id);
            else if (b == ItemDelegate::Favorite)
                emit favoriteRequested(id);
            else
                emit deleteRequested(id);
            return;
        }
    }
    QListView::mouseReleaseEvent(e);
}

void ItemView::mouseDoubleClickEvent(QMouseEvent *e)
{
    if (buttonAt(e->pos(), nullptr) != ItemDelegate::None)
        return;
    QListView::mouseDoubleClickEvent(e);
}

void ItemView::contextMenuEvent(QContextMenuEvent *e)
{
    const QModelIndex i = indexAt(e->pos());
    if (!i.isValid())
        return;
    setCurrentIndex(i);
    emit menuRequested(i.data(ItemModel::IdRole).toString(), e->globalPos());
}

void ItemView::keyPressEvent(QKeyEvent *e)
{
    const QString id = currentId();
    if (!id.isEmpty()) {
        if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
            emit launchRequested(id);
            return;
        }
        if (e->key() == Qt::Key_Delete) {
            emit deleteRequested(id);
            return;
        }
        if (e->key() == Qt::Key_F2) {
            emit editRequested(id);
            return;
        }
    }
    QListView::keyPressEvent(e);
}

// ---------------------------------------------------------------------------
// Drag & drop
// ---------------------------------------------------------------------------

void ItemView::startDrag(const QModelIndex &index)
{
    const QString id = index.data(ItemModel::IdRole).toString();
    auto *mime = new QMimeData;
    mime->setData(kItemMime, id.toUtf8());
    mime->setData("application/x-launcherhub-source", m_dragCollection.toUtf8());

    // Drag pixmap: the card's icon over a soft disc.
    const int px = 56;
    QPixmap pm(px + 16, px + 16);
    pm.fill(Qt::transparent);
    {
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        QColor bg = m_core->theme()->accent();
        bg.setAlpha(70);
        p.setBrush(bg);
        p.setPen(Qt::NoPen);
        p.drawEllipse(pm.rect());
        const QPixmap icon = m_core->pixmapFor(id, px);
        p.drawPixmap(8, 8, px, px, icon);
    }
    auto *drag = new QDrag(this);
    drag->setMimeData(mime);
    drag->setPixmap(pm);
    drag->setHotSpot(pm.rect().center());
    drag->exec(Qt::CopyAction | Qt::MoveAction, Qt::CopyAction);
    m_dropRow = -1;
    m_dropTarget = QModelIndex();
    viewport()->update();
}

int ItemView::dropRowFor(const QPoint &pos) const
{
    if (!model())
        return -1;
    const QModelIndex i = indexAt(pos);
    if (!i.isValid())
        return model()->rowCount();
    const QRect r = visualRect(i);
    const bool grid = m_delegate->viewStyle() == "grid";
    const bool after = grid ? pos.x() > r.center().x() : pos.y() > r.center().y();
    return i.row() + (after ? 1 : 0);
}

void ItemView::dragEnterEvent(QDragEnterEvent *e)
{
    if (m_dropMode != NoDrops && e->mimeData()->hasFormat(kItemMime))
        e->acceptProposedAction();
    else
        e->ignore(); // external files/URLs bubble up to the window
}

void ItemView::dragMoveEvent(QDragMoveEvent *e)
{
    if (m_dropMode == NoDrops || !e->mimeData()->hasFormat(kItemMime)) {
        e->ignore();
        return;
    }
    m_dropTarget = QModelIndex();
    m_dropRow = -1;
    if (m_dropMode == DropOntoCollections) {
        const QModelIndex i = indexAt(e->position().toPoint());
        if (i.isValid() && i.data(ItemModel::CategoryRole).toInt() == LH::Collection)
            m_dropTarget = i;
        else {
            e->ignore();
            viewport()->update();
            return;
        }
    } else {
        // only cards that belong to the same collection can be reordered
        if (QString::fromUtf8(e->mimeData()->data("application/x-launcherhub-source")) != m_dropCollection) {
            e->ignore();
            viewport()->update();
            return;
        }
        m_dropRow = dropRowFor(e->position().toPoint());
    }
    e->acceptProposedAction();
    viewport()->update();
}

void ItemView::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e)
    m_dropRow = -1;
    m_dropTarget = QModelIndex();
    viewport()->update();
}

void ItemView::dropEvent(QDropEvent *e)
{
    const QString id = QString::fromUtf8(e->mimeData()->data(kItemMime));
    if (m_dropMode == DropOntoCollections && m_dropTarget.isValid()) {
        const QString coll = m_dropTarget.data(ItemModel::IdRole).toString();
        emit droppedOnCollection(id, coll);
        e->acceptProposedAction();
    } else if (m_dropMode == ReorderMembers && m_dropRow >= 0) {
        // moving an item forward shifts the insertion point by one
        int row = m_dropRow;
        if (m_model) {
            const int from = m_model->rowOf(id);
            if (from >= 0 && from < row)
                --row;
        }
        emit reorderRequested(id, row);
        e->acceptProposedAction();
    }
    m_dropRow = -1;
    m_dropTarget = QModelIndex();
    viewport()->update();
}
