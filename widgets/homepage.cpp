#include "homepage.h"

#include "appcore.h"
#include "itemactions.h"
#include "itemmodel.h"
#include "itemview.h"

#include <QLabel>
#include <QStackedWidget>
#include <QVBoxLayout>

HomePage::HomePage(AppCore *core, QWidget *parent)
    : QWidget(parent), m_core(core)
{
    setObjectName("page");
    m_favModel = new ItemModel(this);
    m_favModel->setMode(LH::ModeFavorites);
    m_recentModel = new ItemModel(this);
    m_recentModel->setMode(LH::ModeRecent);
    m_searchModel = new ItemModel(this);

    auto wire = [this](ItemView *v, ItemModel *m, const QString &empty) {
        v->setItemModel(m);
        v->setEmptyText(empty);
        ItemActions::connectView(v, this, m_core);
    };

    // Dashboard (favorites + recent)
    auto *dash = new QWidget;
    auto *dl = new QVBoxLayout(dash);
    dl->setContentsMargins(0, 0, 0, 0);
    auto *favHeader = new QLabel("★  Favorites");
    favHeader->setObjectName("h2");
    m_recentHeader = new QLabel("\U0001F551  Recent");
    m_recentHeader->setObjectName("h2");
    m_favView = new ItemView(core);
    m_recentView = new ItemView(core);
    wire(m_favView, m_favModel, "No favorites yet.\nRight-click any card and choose \"Add to Favorites\".");
    wire(m_recentView, m_recentModel, "Items you open will show up here.");
    dl->addWidget(favHeader);
    dl->addWidget(m_favView, 1);
    dl->addSpacing(6);
    dl->addWidget(m_recentHeader);
    dl->addWidget(m_recentView, 1);

    // Search results
    auto *results = new QWidget;
    auto *rl = new QVBoxLayout(results);
    rl->setContentsMargins(0, 0, 0, 0);
    m_resultsHeader = new QLabel("Search results");
    m_resultsHeader->setObjectName("h2");
    m_searchView = new ItemView(core);
    wire(m_searchView, m_searchModel, "Nothing matches your search.");
    rl->addWidget(m_resultsHeader);
    rl->addWidget(m_searchView, 1);

    m_stack = new QStackedWidget;
    m_stack->addWidget(dash);
    m_stack->addWidget(results);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_stack);

    auto syncRecent = [this] {
        const bool on = m_core->settings()->boolean("recentEnabled");
        m_recentHeader->setVisible(on);
        m_recentView->setVisible(on);
    };
    connect(core->settings(), &AppSettings::valuesChanged, this, syncRecent);
    syncRecent();

    connect(m_searchModel, &ItemModel::countChanged, this, [this] {
        m_resultsHeader->setText(QStringLiteral("Search results (%1)").arg(m_searchModel->count()));
    });
}

void HomePage::setSearch(const QString &text)
{
    const bool searching = !text.trimmed().isEmpty();
    m_searchModel->setSearch(text);
    m_resultsHeader->setText(QStringLiteral("Search results (%1)").arg(m_searchModel->count()));
    // Favorites / Recent are hidden while a search is active.
    m_stack->setCurrentIndex(searching ? 1 : 0);
}
