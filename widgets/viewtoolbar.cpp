#include "viewtoolbar.h"

#include "appcore.h"
#include "iconpack.h"

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QSlider>
#include <QToolButton>

namespace {
const char *kStyles[3] = {"grid", "list", "compact"};
const char *kSizes[3] = {"small", "medium", "large"};
const char *kIcons[3] = {"grid", "list", "compact"};
const char *kTips[3] = {"Grid", "List", "Compact"};
}

ViewToolbar::ViewToolbar(AppCore *core, QWidget *parent)
    : QFrame(parent), m_core(core)
{
    setObjectName("viewbar");
    auto *lay = new QHBoxLayout(this);
    lay->setContentsMargins(6, 4, 10, 4);
    lay->setSpacing(2);

    auto *group = new QButtonGroup(this);
    group->setExclusive(true);
    for (int i = 0; i < 3; ++i) {
        auto *b = new QToolButton;
        b->setObjectName("viewBtn");
        b->setCheckable(true);
        b->setToolTip(QString("%1 view").arg(kTips[i]));
        b->setIconSize(QSize(20, 20));
        b->setCursor(Qt::PointingHandCursor);
        group->addButton(b, i);
        lay->addWidget(b);
        m_buttons[i] = b;
        connect(b, &QToolButton::clicked, this, [this, i] { m_core->settings()->set("viewStyle", kStyles[i]); });
    }

    lay->addSpacing(8);
    m_size = new QSlider(Qt::Horizontal);
    m_size->setRange(0, 2);
    m_size->setFixedWidth(64);
    m_size->setToolTip("Card size");
    m_size->setPageStep(1);
    lay->addWidget(m_size);
    connect(m_size, &QSlider::valueChanged, this, [this](int v) { m_core->settings()->set("cardSize", kSizes[v]); });

    connect(core->settings(), &AppSettings::valuesChanged, this, &ViewToolbar::syncFromSettings);
    connect(core->theme(), &Theme::changed, this, &ViewToolbar::applyIcons);
    applyIcons();
    syncFromSettings();
}

void ViewToolbar::applyIcons()
{
    const Theme *t = m_core->theme();
    for (int i = 0; i < 3; ++i) {
        QIcon icon;
        icon.addPixmap(IconPack::pixmap(kIcons[i], 40, t->text()), QIcon::Normal, QIcon::Off);
        icon.addPixmap(IconPack::pixmap(kIcons[i], 40, Qt::white), QIcon::Normal, QIcon::On);
        m_buttons[i]->setIcon(icon);
    }
}

void ViewToolbar::syncFromSettings()
{
    const QString style = m_core->settings()->text("viewStyle");
    for (int i = 0; i < 3; ++i)
        m_buttons[i]->setChecked(style == kStyles[i]);
    const QString size = m_core->settings()->text("cardSize");
    QSignalBlocker b(m_size);
    for (int i = 0; i < 3; ++i)
        if (size == kSizes[i])
            m_size->setValue(i);
}
