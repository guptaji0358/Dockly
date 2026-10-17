#include "mainwindow.h"

#include "appcore.h"
#include "categorypage.h"
#include "collectionspage.h"
#include "detailpanel.h"
#include "homepage.h"
#include "iconpack.h"
#include "settingspage.h"
#include "tray.h"
#include "viewtoolbar.h"

#include <QApplication>
#include <QCloseEvent>
#include <QDragEnterEvent>
#include <QFile>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMimeData>
#include <QPainter>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QShortcut>
#include <QStackedWidget>
#include <QUrl>
#include <QVBoxLayout>

namespace {
const char *kTabIcons[] = {"home", "software", "documents", "entertainment", "web", "collections", "settings"};
const char *kFullNames[] = {"Home", "Software Hub", "Documents Hub", "Entertainment Hub",
                            "Web Shortcuts", "Collections", "Settings"};
}

MainWindow::MainWindow(AppCore *core, QWidget *parent)
    : QMainWindow(parent), m_core(core)
{
    setWindowTitle("Launcher Hub");
    setWindowIcon(AppCore::appIcon());
    resize(1240, 780);
    setMinimumSize(900, 560);
    setAcceptDrops(true);

    // --- icon-only rail ----------------------------------------------------------
    m_sidebar = new QFrame;
    m_sidebar->setObjectName("sidebar");
    m_sidebar->setFixedWidth(76);
    auto *sl = new QVBoxLayout(m_sidebar);
    sl->setContentsMargins(10, 16, 10, 12);
    sl->setSpacing(6);
    m_logo = new QLabel;
    m_logo->setAlignment(Qt::AlignCenter);
    m_logo->setFixedHeight(44);
    m_logo->setToolTip("Launcher Hub");
    sl->addWidget(m_logo);
    sl->addSpacing(14);

    // --- pages ----------------------------------------------------------------
    m_stack = new QStackedWidget;
    m_home = new HomePage(core);
    m_stack->addWidget(m_home);
    m_stack->addWidget(new CategoryPage(core, LH::Software));
    m_stack->addWidget(new CategoryPage(core, LH::Document));
    m_stack->addWidget(new CategoryPage(core, LH::Entertainment));
    m_stack->addWidget(new CategoryPage(core, LH::Web));
    m_stack->addWidget(new CollectionsPage(core));
    m_stack->addWidget(new SettingsPage(core));

    // --- floating dock (icon-only too) ----------------------------------------
    m_dock = new QFrame;
    m_dock->setObjectName("dock");
    auto *dl = new QHBoxLayout(m_dock);
    dl->setContentsMargins(10, 8, 10, 8);
    dl->setSpacing(4);

    for (int i = 0; i < 7; ++i) {
        auto *b = new QPushButton;
        b->setObjectName("nav");
        b->setCheckable(true);
        b->setAutoExclusive(true);
        b->setIconSize(QSize(24, 24));
        b->setFixedHeight(46);
        b->setToolTip(QStringLiteral("%1   (Ctrl+%2)").arg(kFullNames[i]).arg(i + 1));
        b->setCursor(Qt::PointingHandCursor);
        connect(b, &QPushButton::clicked, this, [this, i] { goTo(i); });
        m_nav << b;
        if (i == 6)
            sl->addStretch(1);
        sl->addWidget(b);

        auto *d = new QPushButton;
        d->setObjectName("dockBtn");
        d->setCheckable(true);
        d->setAutoExclusive(true);
        d->setIconSize(QSize(24, 24));
        d->setFixedSize(52, 42);
        d->setToolTip(QString::fromLatin1(kFullNames[i]));
        d->setCursor(Qt::PointingHandCursor);
        connect(d, &QPushButton::clicked, this, [this, i] { goTo(i); });
        m_dockBtns << d;
        dl->addWidget(d);
    }

    // --- main column -----------------------------------------------------------
    m_brand = new QLabel("Launcher Hub");
    m_brand->setObjectName("h2");
    m_search = new QLineEdit;
    m_search->setPlaceholderText("Search everything...   (Ctrl+F)");
    m_search->setClearButtonEnabled(true);
    m_search->setMinimumHeight(40);
    auto *searchAction = m_search->addAction(IconPack::icon("search", core->theme()->subtext()), QLineEdit::LeadingPosition);
    Q_UNUSED(searchAction)

    auto *top = new QHBoxLayout;
    top->setSpacing(14);
    top->addWidget(m_brand);
    top->addWidget(m_search, 1);
    top->addWidget(new ViewToolbar(core));

    m_toast = new QLabel;
    m_toast->setObjectName("toast");
    m_toastTimer.setSingleShot(true);
    m_toastTimer.setInterval(5000);
    connect(&m_toastTimer, &QTimer::timeout, m_toast, &QLabel::clear);

    auto *dockRow = new QHBoxLayout;
    dockRow->addStretch(1);
    dockRow->addWidget(m_dock);
    dockRow->addStretch(1);

    auto *right = new QVBoxLayout;
    right->setContentsMargins(22, 18, 14, 8);
    right->setSpacing(12);
    right->addLayout(top);
    right->addWidget(m_stack, 1);
    right->addLayout(dockRow);
    right->addWidget(m_toast);

    auto *panel = new DetailPanel(core);
    auto *panelWrap = new QVBoxLayout;
    panelWrap->setContentsMargins(0, 18, 18, 18);
    panelWrap->addWidget(panel);

    auto *central = new QWidget;
    central->setObjectName("page");
    auto *root = new QHBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_sidebar);
    root->addLayout(right, 1);
    root->addLayout(panelWrap);
    setCentralWidget(central);

    // "Drop to add" overlay, shown while files are dragged over the window.
    m_dropOverlay = new QLabel("Drop files, folders or links to add them", central);
    m_dropOverlay->setObjectName("dropOverlay");
    m_dropOverlay->setAlignment(Qt::AlignCenter);
    m_dropOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_dropOverlay->hide();

    // --- behaviour -----------------------------------------------------------------
    // Global search: any text jumps to Home and shows matches across every hub.
    connect(m_search, &QLineEdit::textChanged, this, [this](const QString &t) {
        if (!t.trimmed().isEmpty() && m_stack->currentIndex() != 0)
            goTo(0);
        m_home->setSearch(t);
    });
    new QShortcut(QKeySequence::Find, this, [this] { m_search->setFocus(); m_search->selectAll(); });
    for (int i = 0; i < 7; ++i)
        new QShortcut(QKeySequence(QStringLiteral("Ctrl+%1").arg(i + 1)), this, [this, i] { goTo(i); });

    connect(core, &AppCore::message, this, &MainWindow::toast);
    connect(core->theme(), &Theme::changed, this, &MainWindow::applyTheme);

    m_tray = new TrayController(core, this);
    connect(m_tray, &TrayController::showRequested, this, &MainWindow::showFromTray);
    connect(m_tray, &TrayController::quitRequested, this, [this] {
        m_quitting = true;
        close();
        qApp->quit();
    });

    applyTheme();

    const QString def = core->settings()->text("defaultTab");
    goTo(def == "software" ? 1 : def == "custom" ? 5 : 0);
}

bool MainWindow::trayActive() const { return m_tray->isActive(); }

void MainWindow::applyNavIcons()
{
    const Theme *t = m_core->theme();
    for (int i = 0; i < 7; ++i) {
        QIcon rail;
        rail.addPixmap(IconPack::pixmap(kTabIcons[i], 56, t->sidebarText()), QIcon::Normal, QIcon::Off);
        rail.addPixmap(IconPack::pixmap(kTabIcons[i], 56, Qt::white), QIcon::Normal, QIcon::On);
        m_nav[i]->setIcon(rail);
        m_dockBtns[i]->setIcon(rail);
    }
    m_logo->setPixmap(IconPack::pixmap("logo", 64, Qt::white).scaled(36, 36, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void MainWindow::applyTheme()
{
    const Theme *t = m_core->theme();
    // QSS can only draw the check mark from a file, so render a small PNG next to the data.
    const QString check = m_core->dataFolder() + "/check.png";
    if (!QFile::exists(check)) {
        QPixmap pm(36, 36);
        pm.fill(Qt::transparent);
        QPainter p(&pm);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(Qt::white, 4.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPolyline(QPolygonF({QPointF(8, 19), QPointF(15, 26), QPointF(28, 11)}));
        p.end();
        pm.save(check, "PNG");
    }
    qApp->setPalette(t->palette());
    qApp->setStyleSheet(t->styleSheet(check));
    applyNavIcons();

    // Navigation: icon rail or a floating dock under the content.
    m_sidebar->setVisible(!t->dock());
    m_dock->setVisible(t->dock());
    m_brand->setVisible(t->dock());
}

void MainWindow::goTo(int index)
{
    if (index < 0 || index >= m_stack->count())
        return;
    const bool changed = m_stack->currentIndex() != index;
    m_stack->setCurrentIndex(index);
    for (int i = 0; i < m_nav.size(); ++i) {
        m_nav[i]->setChecked(i == index);
        m_dockBtns[i]->setChecked(i == index);
    }

    if (changed && m_core->settings()->boolean("animations") && isVisible()) {
        QWidget *page = m_stack->currentWidget();
        auto *fx = new QGraphicsOpacityEffect(page);
        page->setGraphicsEffect(fx);
        auto *anim = new QPropertyAnimation(fx, "opacity", fx);
        anim->setDuration(200);
        anim->setStartValue(0.0);
        anim->setEndValue(1.0);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        connect(anim, &QPropertyAnimation::finished, page, [page] { page->setGraphicsEffect(nullptr); });
        anim->start();
    }
}

void MainWindow::toast(const QString &text)
{
    m_toast->setText(text);
    m_toastTimer.start();
}

void MainWindow::showFromTray()
{
    setWindowState(windowState() & ~Qt::WindowMinimized);
    show();
    raise();
    activateWindow();
}

// ---------------------------------------------------------------------------
// Drag & drop of files / folders / links onto the window
// ---------------------------------------------------------------------------

bool MainWindow::acceptsDrop(const QMimeData *m)
{
    if (m->hasFormat("application/x-launcherhub-item"))
        return false; // internal card drags are handled by the views
    return m->hasUrls() || (m->hasText() && m->text().trimmed().startsWith("http"));
}

void MainWindow::resizeEvent(QResizeEvent *e)
{
    QMainWindow::resizeEvent(e);
    if (m_dropOverlay && centralWidget())
        m_dropOverlay->setGeometry(centralWidget()->rect().adjusted(12, 12, -12, -12));
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (!acceptsDrop(e->mimeData())) {
        e->ignore();
        return;
    }
    e->acceptProposedAction();
    m_dropOverlay->setGeometry(centralWidget()->rect().adjusted(12, 12, -12, -12));
    m_dropOverlay->raise();
    m_dropOverlay->show();
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent *e)
{
    Q_UNUSED(e)
    m_dropOverlay->hide();
}

void MainWindow::dropEvent(QDropEvent *e)
{
    m_dropOverlay->hide();
    QStringList items;
    if (e->mimeData()->hasUrls()) {
        for (const QUrl &u : e->mimeData()->urls())
            items << (u.isLocalFile() ? u.toLocalFile() : u.toString());
    } else {
        items << e->mimeData()->text().trimmed();
    }
    // The tab you're on hints where files of an unknown type go.
    static const int hints[] = {-1, LH::Software, LH::Document, LH::Entertainment, LH::Web, -1, -1};
    m_core->addDropped(items, hints[qBound(0, m_stack->currentIndex(), 6)]);
    e->acceptProposedAction();
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (m_tray->isActive() && !m_quitting) {
        hide();
        e->ignore();
        return;
    }
    e->accept();
    qApp->quit();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if (e->type() == QEvent::WindowStateChange && isMinimized() && m_tray->isActive())
        QTimer::singleShot(0, this, &QWidget::hide);
}
