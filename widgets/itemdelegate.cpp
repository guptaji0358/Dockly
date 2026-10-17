#include "itemdelegate.h"

#include "appcore.h"
#include "iconpack.h"
#include "itemmodel.h"

#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kBtn = 26;     // hover button size
constexpr int kBtnGap = 4;
constexpr int kLift = 3;     // hover lift in px

qreal ease(qreal t) { return 1 - (1 - t) * (1 - t) * (1 - t); } // ease-out cubic
}

ItemDelegate::ItemDelegate(AppCore *core, QObject *parent)
    : QStyledItemDelegate(parent), m_core(core)
{
}

QString ItemDelegate::viewStyle() const
{
    return m_forcedStyle.isEmpty() ? m_core->settings()->text("viewStyle") : m_forcedStyle;
}

int ItemDelegate::cardWidth() const
{
    const QString s = m_core->settings()->text("cardSize");
    if (s == "small") return 128;
    if (s == "large") return 210;
    return 168;
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem &opt, const QModelIndex &) const
{
    const QString style = viewStyle();
    const int lh = opt.fontMetrics.height();
    if (style == "list")
        return {qMax(260, m_viewportWidth - 6), 66};
    if (style == "compact")
        return {qMax(260, m_viewportWidth - 6), 38};
    const int w = cardWidth();
    const int icon = int(w * 0.5);
    return {w + 6, 20 + icon + 12 + lh + 3 + lh + 18 + 6};
}

// Buttons sit top-right on grid cards and vertically centred at the right of rows.
QRectF ItemDelegate::buttonRect(const QRectF &card, Button b) const
{
    const int slot = int(Delete) - int(b); // Delete is rightmost
    const qreal x = card.right() - 8 - kBtn - slot * (kBtn + kBtnGap);
    const bool grid = viewStyle() == "grid";
    const qreal y = grid ? card.top() + 8 : card.center().y() - kBtn / 2.0;
    return QRectF(x, y, kBtn, kBtn);
}

ItemDelegate::Button ItemDelegate::hitButton(const QRect &itemRect, const QPoint &pos) const
{
    // Buttons only exist while hovered, i.e. when the card is fully lifted.
    const QRectF card = QRectF(itemRect).adjusted(3, 3, -3, -3);
    for (Button b : {Edit, Favorite, Delete})
        if (buttonRect(card, b).contains(pos))
            return b;
    return None;
}

void ItemDelegate::paint(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    p->save();
    p->setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);

    // Entrance: fade + slide up, staggered per row by the view.
    const qreal enter = enterProgress ? ease(enterProgress(index)) : 1.0;
    if (enter < 1.0) {
        p->setOpacity(enter);
        p->translate(0, (1.0 - enter) * 16);
    }

    const QString style = viewStyle();
    if (style == "grid")
        paintCard(p, opt, index);
    else
        paintRow(p, opt, index, style == "compact");
    p->restore();
}

void ItemDelegate::paintBackground(QPainter *p, const QRectF &r, int category, qreal lit) const
{
    const Theme *t = m_core->theme();
    const int rad = t->radius();
    const QColor accent = t->accentOf(category);

    if (t->hardShadow()) { // editorial: flat block with a hard offset shadow
        p->setPen(Qt::NoPen);
        p->setBrush(t->border());
        p->drawRect(r.translated(4 + lit, 4 + lit));
    } else if (lit > 0.01) { // soft drop shadow that grows with the lift
        p->setPen(Qt::NoPen);
        for (int i = 3; i >= 1; --i) {
            p->setBrush(QColor(0, 0, 0, int(14 * lit * (4 - i))));
            p->drawRoundedRect(r.adjusted(-i, i, i, i + 2).translated(0, kLift * lit), rad + i, rad + i);
        }
    }
    if (t->glow() && lit > 0.01) { // neon: soft halo around the lit card
        p->setBrush(Qt::NoBrush);
        for (int i = 3; i >= 1; --i) {
            QColor c = accent;
            c.setAlpha(int(22 * (4 - i) * lit));
            p->setPen(QPen(c, i * 2.2));
            p->drawRoundedRect(r.adjusted(-i, -i, i, i), rad + i, rad + i);
        }
    }

    const QColor fill = lit > 0.5 ? t->cardColor(category, true) : t->cardColor(category, false);
    QColor mixed = fill;
    if (lit > 0.01 && lit < 0.99) { // cross-fade between the resting and lit colours
        const QColor a = t->cardColor(category, false), b = t->cardColor(category, true);
        mixed = QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * lit,
                                 a.greenF() + (b.greenF() - a.greenF()) * lit,
                                 a.blueF() + (b.blueF() - a.blueF()) * lit);
    }
    p->setBrush(mixed);
    if (t->borderWidth() > 0) {
        QColor edge = t->hardShadow() ? t->border() : t->border();
        if (!t->hardShadow() && lit > 0.01)
            edge = QColor::fromRgbF(edge.redF() + (accent.redF() - edge.redF()) * lit,
                                    edge.greenF() + (accent.greenF() - edge.greenF()) * lit,
                                    edge.blueF() + (accent.blueF() - edge.blueF()) * lit);
        p->setPen(QPen(edge, t->borderWidth()));
    } else {
        p->setPen(lit > 0.01 ? QPen(accent, 2 * lit) : Qt::NoPen);
    }
    if (rad == 0)
        p->drawRect(r);
    else
        p->drawRoundedRect(r, rad, rad);
}

void ItemDelegate::paintButtons(QPainter *p, const QRectF &card, bool hover, bool favorite) const
{
    const Theme *t = m_core->theme();
    const qreal dpr = p->device()->devicePixelRatioF();
    const int px = int(16 * dpr);

    auto draw = [&](Button b, const QString &icon, const QColor &fg) {
        const QRectF r = buttonRect(card, b);
        if (hover) {
            QColor bg = t->surface();
            bg.setAlpha(240);
            p->setPen(t->hardShadow() ? QPen(t->border(), 1.5) : Qt::NoPen);
            p->setBrush(bg);
            const qreal rad = t->radius() == 0 ? 0 : 8;
            p->drawRoundedRect(r, rad, rad);
        }
        const QPixmap pm = IconPack::pixmap(icon, px, fg);
        p->drawPixmap(QRectF(r.center().x() - 8, r.center().y() - 8, 16, 16), pm, pm.rect());
    };
    if (hover) {
        draw(Edit, "edit", t->text());
        draw(Favorite, favorite ? "star-fill" : "star", favorite ? t->favorite() : t->text());
        draw(Delete, "trash", t->missing());
    } else if (favorite) {
        draw(Favorite, "star-fill", t->favorite());
    }
}

void ItemDelegate::paintWarning(QPainter *p, const QRectF &r, int status) const
{
    // Semi-transparent warning badge (amber = warning, red = missing).
    const Theme *t = m_core->theme();
    const QColor c = status == LH::Missing ? t->missing() : t->warning();
    const QPixmap pm = IconPack::pixmap("warning", int(r.width() * p->device()->devicePixelRatioF()), c);
    p->save();
    p->setOpacity(p->opacity() * 0.95);
    p->drawPixmap(r, pm, pm.rect());
    p->restore();
}

void ItemDelegate::paintCard(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index) const
{
    const Theme *t = m_core->theme();
    const bool hover = opt.state & QStyle::State_MouseOver;
    const bool sel = opt.state & QStyle::State_Selected;
    const qreal h = hoverProgress ? hoverProgress(index) : (hover ? 1.0 : 0.0);
    const qreal lit = qMax(h, sel ? 1.0 : 0.0);
    const int status = index.data(ItemModel::StatusRole).toInt();
    const int category = index.data(ItemModel::CategoryRole).toInt();

    QRectF r = QRectF(opt.rect).adjusted(3, 3 + kLift, -3, -3);
    r.translate(0, -kLift * h); // hover lift

    paintBackground(p, r, category, lit);

    const int iconSize = int(r.width() * 0.5 * (1.0 + 0.05 * h));
    const QRectF iconRect(r.center().x() - iconSize / 2.0, r.top() + 20 - 2 * h, iconSize, iconSize);

    if (t->tinted()) { // playful: colourful disc behind the icon
        QColor disc = t->categoryColor(category);
        disc.setAlpha(70);
        p->setPen(Qt::NoPen);
        p->setBrush(disc);
        p->drawEllipse(iconRect.adjusted(-9, -9, 9, 9));
    }

    p->save();
    if (status != LH::Normal)
        p->setOpacity(p->opacity() * 0.45);
    const qreal dpr = p->device()->devicePixelRatioF();
    const QPixmap pm = m_core->pixmapFor(index.data(ItemModel::IdRole).toString(), int(iconSize * dpr));
    if (!pm.isNull())
        p->drawPixmap(iconRect, pm, pm.rect());
    p->restore();

    if (status != LH::Normal) {
        const qreal s = iconSize * 0.55;
        paintWarning(p, QRectF(iconRect.center().x() - s / 2, iconRect.center().y() - s / 2, s, s), status);
    }

    paintButtons(p, r, h > 0.3, index.data(ItemModel::FavoriteRole).toBool());

    QFont bold = opt.font;
    bold.setWeight(QFont::DemiBold);
    if (t->style() == "editorial")
        bold.setFamily(t->headingFont());
    p->setFont(bold);
    p->setPen(t->text());
    const QRectF nameRect(r.left() + 10, iconRect.bottom() + 12, r.width() - 20, QFontMetrics(bold).height());
    p->drawText(nameRect, Qt::AlignHCenter | Qt::AlignVCenter,
                QFontMetrics(bold).elidedText(index.data(ItemModel::NameRole).toString(), Qt::ElideRight, int(nameRect.width())));

    QFont small = opt.font;
    small.setPointSizeF(opt.font.pointSizeF() * 0.85);
    p->setFont(small);
    p->setPen(status == LH::Missing ? t->missing() : status == LH::Warning ? t->warning() : t->subtext());
    const QRectF subRect(nameRect.left(), nameRect.bottom() + 3, nameRect.width(), QFontMetrics(small).height());
    p->drawText(subRect, Qt::AlignHCenter | Qt::AlignVCenter,
                QFontMetrics(small).elidedText(index.data(ItemModel::SubtitleRole).toString(), Qt::ElideRight, int(subRect.width())));
}

void ItemDelegate::paintRow(QPainter *p, const QStyleOptionViewItem &opt, const QModelIndex &index, bool compact) const
{
    const Theme *t = m_core->theme();
    const bool hover = opt.state & QStyle::State_MouseOver;
    const bool sel = opt.state & QStyle::State_Selected;
    const qreal h = hoverProgress ? hoverProgress(index) : (hover ? 1.0 : 0.0);
    const int status = index.data(ItemModel::StatusRole).toInt();
    const int category = index.data(ItemModel::CategoryRole).toInt();
    QRectF r = QRectF(opt.rect).adjusted(3, 3, -3, -3);
    r.translate(2 * h, 0); // rows nudge sideways instead of lifting

    paintBackground(p, r, category, qMax(h, sel ? 1.0 : 0.0));

    const int iconSize = compact ? 22 : 38;
    const QRectF iconRect(r.left() + 14, r.center().y() - iconSize / 2.0, iconSize, iconSize);
    p->save();
    if (status != LH::Normal)
        p->setOpacity(p->opacity() * 0.45);
    const qreal dpr = p->device()->devicePixelRatioF();
    const QPixmap pm = m_core->pixmapFor(index.data(ItemModel::IdRole).toString(), int(iconSize * dpr));
    if (!pm.isNull())
        p->drawPixmap(iconRect, pm, pm.rect());
    p->restore();
    if (status != LH::Normal) {
        const qreal s = iconSize * 0.7;
        paintWarning(p, QRectF(iconRect.center().x() - s / 2, iconRect.center().y() - s / 2, s, s), status);
    }

    paintButtons(p, r, h > 0.3, index.data(ItemModel::FavoriteRole).toBool());

    const qreal left = iconRect.right() + 14;
    const qreal right = buttonRect(r, Edit).left() - 8;
    QFont bold = opt.font;
    bold.setWeight(QFont::DemiBold);
    if (t->style() == "editorial")
        bold.setFamily(t->headingFont());
    p->setFont(bold);
    p->setPen(t->text());
    const QString name = index.data(ItemModel::NameRole).toString();
    if (compact) {
        const QRectF nr(left, r.top(), right - left, r.height());
        p->drawText(nr, Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(bold).elidedText(name, Qt::ElideRight, int(nr.width())));
        return;
    }
    const QRectF nr(left, r.top() + 9, right - left, r.height() / 2 - 9);
    p->drawText(nr, Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(bold).elidedText(name, Qt::ElideRight, int(nr.width())));

    QFont small = opt.font;
    small.setPointSizeF(opt.font.pointSizeF() * 0.88);
    p->setFont(small);
    p->setPen(t->subtext());
    const QRectF sr(left, r.center().y(), right - left, r.height() / 2 - 9);
    QString sub = index.data(ItemModel::TargetRole).toString();
    if (sub.isEmpty())
        sub = index.data(ItemModel::SubtitleRole).toString();
    if (status != LH::Normal)
        sub = index.data(ItemModel::SubtitleRole).toString() + QStringLiteral(" — ") + sub;
    p->drawText(sr, Qt::AlignVCenter | Qt::AlignLeft, QFontMetrics(small).elidedText(sub, Qt::ElideMiddle, int(sr.width())));
}
