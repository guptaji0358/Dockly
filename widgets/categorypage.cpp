#include "categorypage.h"

#include "appcore.h"
#include "iconpack.h"
#include "itemactions.h"
#include "itemmodel.h"
#include "itemview.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

CategoryPage::CategoryPage(AppCore *core, LH::Category category, QWidget *parent)
    : QWidget(parent), m_core(core), m_category(category)
{
    setObjectName("page");
    m_model = new ItemModel(this);
    m_model->setCategory(category);

    QString title, empty;
    switch (category) {
    case LH::Software:
        title = "Software Hub";
        empty = "No programs added yet.\nClick \"Add Software\" to pick from the detected programs or browse for an .exe.";
        break;
    case LH::Document:
        title = "Documents Hub";
        empty = "No documents yet.\nClick \"Add Document\" to pin files here.";
        break;
    case LH::Entertainment:
        title = "Entertainment Hub";
        empty = "No media yet.\nClick \"Add Media\" to add videos, music or images.";
        break;
    default:
        title = "Web Shortcuts";
        empty = "No shortcuts yet.\nClick \"Add URL\" to pin a website.";
        break;
    }

    auto *heading = new QLabel(title);
    heading->setObjectName("h1");
    m_count = new QLabel;
    m_count->setObjectName("sub");

    auto *bar = new QHBoxLayout;
    bar->addWidget(heading);
    bar->addWidget(m_count, 0, Qt::AlignBottom);
    bar->addStretch(1);

    auto button = [&](const QString &text, bool primary, auto &&slot) {
        auto *b = new QPushButton(text);
        b->setProperty("primary", primary);
        const QString iconName = text.startsWith("Scan") ? "scan" : "add";
        auto setIcon = [this, b, primary, iconName] {
            b->setIcon(IconPack::icon(iconName, primary ? QColor(Qt::white) : m_core->theme()->text(), 40));
        };
        setIcon();
        connect(m_core->theme(), &Theme::changed, b, setIcon);
        connect(b, &QPushButton::clicked, this, slot);
        bar->addWidget(b);
        return b;
    };

    switch (category) {
    case LH::Software: {
        auto *scan = button("Scan this PC", false, [this] { m_core->refreshSoftware(); });
        connect(core, &AppCore::scanningChanged, scan, [this, scan] {
            scan->setEnabled(!m_core->scanning());
            scan->setText(m_core->scanning() ? "Scanning..." : "Scan this PC");
        });
        button("Add Software", true, [this] { ItemActions::addSoftware(this, m_core); });
        break;
    }
    case LH::Document:
        button("Add Document", true, [this] { ItemActions::addFiles(this, m_core, LH::Document); });
        break;
    case LH::Entertainment:
        button("Add Media", true, [this] { ItemActions::addFiles(this, m_core, LH::Entertainment); });
        break;
    default:
        button("Add URL", true, [this] { ItemActions::addUrl(this, m_core); });
        break;
    }

    m_view = new ItemView(core);
    m_view->setItemModel(m_model);
    m_view->setEmptyText(empty);
    ItemActions::connectView(m_view, this, m_core);

    auto updateCount = [this] { m_count->setText(QStringLiteral("%1 item(s)").arg(m_model->count())); };
    connect(m_model, &ItemModel::countChanged, this, updateCount);
    updateCount();

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addLayout(bar);
    lay->addSpacing(6);
    lay->addWidget(m_view, 1);
}
