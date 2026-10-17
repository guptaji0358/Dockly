#pragma once

#include <QStyledItemDelegate>
#include <functional>

class AppCore;

// Paints items as cards (grid), rows (list) or slim rows (compact) in the active skin, with
// hover buttons for edit / favorite / delete, a hover lift and an entrance animation.
class ItemDelegate : public QStyledItemDelegate
{
public:
    enum Button { None = 0, Edit, Favorite, Delete };

    explicit ItemDelegate(AppCore *core, QObject *parent = nullptr);

    void setViewportWidth(int w) { m_viewportWidth = w; }
    void setForcedStyle(const QString &style) { m_forcedStyle = style; }
    QString viewStyle() const;

    // Animation state is owned by the view; these report 0..1 progress for an index.
    std::function<qreal(const QModelIndex &)> hoverProgress;
    std::function<qreal(const QModelIndex &)> enterProgress;

    // Which hover button (if any) is under pos, for an item occupying itemRect.
    Button hitButton(const QRect &itemRect, const QPoint &pos) const;

    void paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const override;
    QSize sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &index) const override;

private:
    int cardWidth() const;
    QRectF buttonRect(const QRectF &card, Button b) const;
    void paintBackground(QPainter *p, const QRectF &r, int category, qreal lit) const;
    void paintButtons(QPainter *p, const QRectF &card, bool hover, bool favorite) const;
    void paintCard(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const;
    void paintRow(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index, bool compact) const;
    void paintWarning(QPainter *p, const QRectF &r, int status) const;

    AppCore *m_core;
    int m_viewportWidth = 600;
    QString m_forcedStyle;
};
