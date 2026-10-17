#include "detailpanel.h"

#include "appcore.h"
#include "iconpack.h"
#include "itemactions.h"

#include <QDir>
#include <QFileDialog>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

DetailPanel::DetailPanel(AppCore *core, QWidget *parent)
    : QFrame(parent), m_core(core)
{
    setObjectName("panel");
    setFixedWidth(300);

    m_icon = new QLabel;
    m_icon->setFixedSize(64, 64);
    m_icon->setScaledContents(true);
    m_tag = new QLabel;
    m_tag->setObjectName("sub");
    auto *close = new QPushButton;
    close->setObjectName("panelClose");
    close->setFixedSize(30, 30);
    close->setToolTip("Close panel");
    connect(close, &QPushButton::clicked, this, [this] { m_core->select({}); });

    auto *head = new QHBoxLayout;
    head->addWidget(m_icon);
    head->addSpacing(8);
    head->addWidget(m_tag, 1, Qt::AlignVCenter);
    head->addWidget(close, 0, Qt::AlignTop);

    m_name = new QLineEdit;
    m_target = new QLineEdit;
    m_browse = new QPushButton("Browse...");
    m_targetLabel = new QLabel;
    m_targetLabel->setObjectName("sub");
    m_status = new QLabel;
    m_status->setWordWrap(true);

    m_open = new QPushButton("Open");
    m_open->setProperty("primary", true);
    m_save = new QPushButton("Save changes");
    m_fav = new QPushButton;
    m_locate = new QPushButton("Locate Again...");
    m_delete = new QPushButton("Delete");
    m_delete->setProperty("danger", true);

    auto *nameLabel = new QLabel("Name");
    nameLabel->setObjectName("sub");

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 16, 16, 16);
    lay->setSpacing(8);
    lay->addLayout(head);
    lay->addSpacing(6);
    lay->addWidget(nameLabel);
    lay->addWidget(m_name);
    lay->addWidget(m_targetLabel);
    lay->addWidget(m_target);
    lay->addWidget(m_browse);
    lay->addWidget(m_status);
    lay->addSpacing(6);
    lay->addWidget(m_open);
    lay->addWidget(m_save);
    lay->addWidget(m_fav);
    lay->addWidget(m_locate);
    lay->addStretch(1);
    lay->addWidget(m_delete);

    connect(m_name, &QLineEdit::textEdited, this, [this] { m_dirty = true; });
    connect(m_target, &QLineEdit::textEdited, this, [this] { m_dirty = true; });
    connect(m_browse, &QPushButton::clicked, this, [this] {
        const QString f = QFileDialog::getOpenFileName(this, "Choose file", {}, m_core->fileFilter(m_category));
        if (!f.isEmpty()) {
            m_target->setText(QDir::toNativeSeparators(f));
            m_dirty = true;
        }
    });
    connect(m_open, &QPushButton::clicked, this, [this] { ItemActions::launch(this, m_core, m_id); });
    connect(m_save, &QPushButton::clicked, this, [this] {
        if (m_core->updateItem(m_id, m_name->text(), m_target->text())) {
            m_dirty = false;
            load();
        }
    });
    connect(m_fav, &QPushButton::clicked, this, [this] {
        m_core->setFavorite(m_id, !m_core->itemInfo(m_id)["favorite"].toBool());
    });
    connect(m_locate, &QPushButton::clicked, this, [this] {
        const QString f = QFileDialog::getOpenFileName(this, "Locate " + m_name->text(), {}, m_core->fileFilter(m_category));
        if (!f.isEmpty()) {
            m_target->setText(QDir::toNativeSeparators(f));
            m_core->updateItem(m_id, m_name->text(), f);
            m_dirty = false;
            load();
        }
    });
    connect(m_delete, &QPushButton::clicked, this, [this] { ItemActions::remove(this, m_core, m_id); });

    auto applyIcons = [this, close] {
        const Theme *t = m_core->theme();
        close->setIcon(IconPack::icon("close", t->text(), 32));
        m_open->setIcon(IconPack::icon("open", Qt::white, 40));
        m_save->setIcon(IconPack::icon("check", t->text(), 40));
        m_locate->setIcon(IconPack::icon("locate", t->text(), 40));
        m_delete->setIcon(IconPack::icon("trash", t->missing(), 40));
    };
    applyIcons();
    connect(core->theme(), &Theme::changed, this, applyIcons);

    connect(core, &AppCore::selectionChanged, this, &DetailPanel::load);
    connect(core, &AppCore::itemsChanged, this, &DetailPanel::refreshState);
    connect(core, &AppCore::iconsChanged, this, &DetailPanel::refreshState);
    load();
}

void DetailPanel::load()
{
    m_id = m_core->selectedId();
    m_dirty = false;
    const QVariantMap info = m_core->itemInfo(m_id);
    if (info.isEmpty()) {
        m_id.clear();
        hide();
        return;
    }
    m_category = info["category"].toInt();
    const bool collection = m_category == LH::Collection;
    const bool web = m_category == LH::Web;
    m_name->setText(info["name"].toString());
    m_target->setText(info["target"].toString());
    m_targetLabel->setText(web ? "URL" : "Path");
    for (QWidget *w : {static_cast<QWidget *>(m_targetLabel), static_cast<QWidget *>(m_target)})
        w->setVisible(!collection);
    m_browse->setVisible(!collection && !web);
    m_open->setText(collection ? "Launch All" : "Open");
    if (!isVisible() && m_core->settings()->boolean("animations")) {
        auto *fx = new QGraphicsOpacityEffect(this);
        setGraphicsEffect(fx);
        auto *anim = new QPropertyAnimation(fx, "opacity", fx);
        anim->setDuration(220);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, this, [this] { setGraphicsEffect(nullptr); });
        anim->start();
    }
    show();
    refreshState();
}

void DetailPanel::refreshState()
{
    if (m_id.isEmpty())
        return;
    const QVariantMap info = m_core->itemInfo(m_id);
    if (info.isEmpty()) {
        m_core->select({});
        return;
    }
    const int status = info["status"].toInt();
    const bool collection = m_category == LH::Collection;
    m_tag->setText(info["categoryName"].toString()
                   + (collection ? QStringLiteral("  •  %1 item(s)").arg(info["memberCount"].toInt()) : QString()));
    QPixmap pm = m_core->pixmapFor(m_id, 128);
    m_icon->setPixmap(pm);

    if (status == LH::Missing) {
        m_status->setText("⚠ Missing: nothing was found at this path.");
        m_status->setStyleSheet(QStringLiteral("color: %1;").arg(m_core->theme()->missing().name()));
    } else if (status == LH::Warning) {
        m_status->setText("⚠ This URL looks invalid.");
        m_status->setStyleSheet(QStringLiteral("color: %1;").arg(m_core->theme()->warning().name()));
    } else {
        m_status->setText(collection ? QString() : QStringLiteral("✓ Available"));
        m_status->setStyleSheet(QStringLiteral("color: %1;").arg(m_core->theme()->subtext().name()));
    }
    m_status->setVisible(!m_status->text().isEmpty());
    m_locate->setVisible(status != LH::Normal && m_category != LH::Web && !collection);
    const bool fav = info["favorite"].toBool();
    m_fav->setText(fav ? "Remove from Favorites" : "Add to Favorites");
    m_fav->setIcon(IconPack::icon(fav ? "star-fill" : "star", fav ? m_core->theme()->favorite() : m_core->theme()->text(), 40));
    m_fav->setVisible(!collection || true);
    if (!m_dirty) {
        m_name->setText(info["name"].toString());
        m_target->setText(info["target"].toString());
    }
}
