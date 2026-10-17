#pragma once

#include <QElapsedTimer>
#include <QHash>
#include <QListView>
#include <QTimer>

class AppCore;
class ItemDelegate;
class ItemModel;

// QListView preconfigured for item cards: grid / list / compact, OpenGL-backed viewport,
// hover + entrance animations, empty-state text, hover buttons, context menu and drag & drop.
class ItemView : public QListView
{
    Q_OBJECT
public:
    // What dropping a dragged card onto this view does.
    enum DropMode {
        NoDrops,
        DropOntoCollections, // dropping on a collection card adds the dragged card to it
        ReorderMembers,      // dropping between member cards reorders them
    };

    explicit ItemView(AppCore *core, QWidget *parent = nullptr);

    void setItemModel(ItemModel *model);
    ItemModel *itemModel() const { return m_model; }
    void setEmptyText(const QString &text) { m_empty = text; viewport()->update(); }
    void setForcedViewStyle(const QString &style);
    void setDropMode(DropMode mode, const QString &collectionId = QString());
    void setDragSource(const QString &collectionId) { m_dragCollection = collectionId; }
    QString currentId() const;

public slots:
    void applySettings();

signals:
    void launchRequested(const QString &id);
    void menuRequested(const QString &id, const QPoint &globalPos);
    void deleteRequested(const QString &id);
    void editRequested(const QString &id);
    void favoriteRequested(const QString &id);
    void droppedOnCollection(const QString &itemId, const QString &collectionId);
    void reorderRequested(const QString &itemId, int toRow);

protected:
    void resizeEvent(QResizeEvent *e) override;
    void paintEvent(QPaintEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *e) override;
    void keyPressEvent(QKeyEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseMoveEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void mouseDoubleClickEvent(QMouseEvent *e) override;
    void leaveEvent(QEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragMoveEvent(QDragMoveEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    int buttonAt(const QPoint &pos, QModelIndex *index) const;
    qreal hoverOf(const QModelIndex &i) const;
    qreal enterOf(const QModelIndex &i) const;
    void setHoverId(const QString &id);
    void tick();
    void startEnter();
    void startDrag(const QModelIndex &index);
    int dropRowFor(const QPoint &pos) const;

    AppCore *m_core;
    ItemModel *m_model = nullptr;
    ItemDelegate *m_delegate;
    QString m_empty;
    QString m_forcedStyle;

    // animation state
    QTimer m_anim;
    QHash<QString, qreal> m_hover;   // item id -> 0..1
    QString m_hoverId;
    QElapsedTimer m_enterClock;
    bool m_entering = false;

    // drag & drop
    DropMode m_dropMode = NoDrops;
    QString m_dropCollection;        // for ReorderMembers: the collection shown
    QString m_dragCollection;
    QPoint m_pressPos;
    QModelIndex m_pressIndex;
    int m_dropRow = -1;              // insertion row while reordering
    QModelIndex m_dropTarget;        // collection card being hovered
};
