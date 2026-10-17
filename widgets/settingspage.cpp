#include "settingspage.h"

#include "appcore.h"

#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <functional>

SettingsPage::SettingsPage(AppCore *core, QWidget *parent)
    : QWidget(parent), m_core(core)
{
    setObjectName("page");
    AppSettings *s = core->settings();

    auto *content = new QWidget;
    content->setObjectName("page");
    auto *col = new QVBoxLayout(content);
    col->setContentsMargins(0, 0, 12, 12);
    col->setSpacing(14);

    auto group = [&](const QString &title) {
        auto *frame = new QFrame;
        frame->setObjectName("group");
        auto *l = new QVBoxLayout(frame);
        l->setContentsMargins(18, 14, 18, 16);
        l->setSpacing(10);
        auto *h = new QLabel(title);
        h->setObjectName("h2");
        l->addWidget(h);
        col->addWidget(frame);
        return l;
    };

    // Re-sync controls whenever settings change from elsewhere (restore, reset).
    auto *syncList = new QList<std::function<void()>>;
    connect(this, &QObject::destroyed, [syncList] { delete syncList; });
    connect(s, &AppSettings::valuesChanged, this, [syncList] { for (auto &f : *syncList) f(); });

    auto check = [&](QVBoxLayout *l, const QString &label, const QString &key) {
        auto *c = new QCheckBox(label);
        c->setChecked(s->boolean(key));
        connect(c, &QCheckBox::toggled, this, [s, key](bool on) { s->set(key, on); });
        syncList->append([=] { QSignalBlocker b(c); c->setChecked(s->boolean(key)); });
        l->addWidget(c);
        return c;
    };
    auto combo = [&](QFormLayout *f, const QString &label, const QString &key,
                     const QList<QPair<QString, QString>> &options) {
        auto *c = new QComboBox;
        for (const auto &o : options)
            c->addItem(o.first, o.second);
        auto apply = [=] {
            QSignalBlocker b(c);
            const int i = c->findData(s->text(key));
            c->setCurrentIndex(i < 0 ? 0 : i);
        };
        apply();
        connect(c, &QComboBox::activated, this, [=](int i) { s->set(key, c->itemData(i)); });
        syncList->append(apply);
        c->setMaximumWidth(260);
        f->addRow(label, c);
        return c;
    };

    // --- Startup -----------------------------------------------------
    auto *g = group("Startup");
    check(g, "Start with Windows", "startWithWindows");
    check(g, "Minimize to system tray", "minimizeToTray");
    {
        auto *f = new QFormLayout;
        combo(f, "Default tab on startup", "defaultTab",
              {{"Home", "home"}, {"Software", "software"}, {"Custom (Collections)", "custom"}});
        g->addLayout(f);
    }

    // --- Appearance -------------------------------------------------
    g = group("Appearance");
    {
        auto *f = new QFormLayout;
        combo(f, "Style", "style", {{"Neon", "neon"}, {"Playful", "playful"}, {"Editorial", "editorial"}});
        combo(f, "Navigation", "navigation", {{"Sidebar", "sidebar"}, {"Floating dock", "dock"}});
        combo(f, "Theme", "theme", {{"System", "system"}, {"Light", "light"}, {"Dark", "dark"}});

        auto *accent = new QPushButton;
        auto *reset = new QPushButton("Skin default");
        auto paintAccent = [=] {
            const QString c = m_core->theme()->accent().name();
            accent->setText(c);
            accent->setStyleSheet(QStringLiteral("QPushButton { background: %1; color: white; }").arg(c));
            reset->setEnabled(!s->text("accent").isEmpty());
        };
        paintAccent();
        connect(accent, &QPushButton::clicked, this, [=] {
            const QColor c = QColorDialog::getColor(m_core->theme()->accent(), this, "Accent colour");
            if (c.isValid())
                s->set("accent", c.name());
        });
        connect(reset, &QPushButton::clicked, this, [=] { s->set("accent", QString()); });
        connect(m_core->theme(), &Theme::changed, this, paintAccent);
        accent->setMaximumWidth(260);
        auto *accentRow = new QHBoxLayout;
        accentRow->addWidget(accent, 1);
        accentRow->addWidget(reset);
        f->addRow("Accent colour", accentRow);
        g->addLayout(f);
    }
    check(g, "Animations", "animations");

    // --- Items ------------------------------------------------------
    g = group("Items");
    check(g, "Recent items", "recentEnabled");
    {
        auto *f = new QFormLayout;
        auto *max = new QSpinBox;
        max->setRange(1, 100);
        max->setValue(s->integer("maxRecent"));
        connect(max, &QSpinBox::valueChanged, this, [s](int v) { s->set("maxRecent", v); });
        syncList->append([=] { QSignalBlocker b(max); max->setValue(s->integer("maxRecent")); });
        max->setMaximumWidth(120);
        f->addRow("Max recent items", max);
        g->addLayout(f);
    }
    check(g, "Confirm before delete", "confirmDelete");
    check(g, "Check for missing items on startup", "checkMissingOnStartup");

    // --- General ----------------------------------------------------
    g = group("General");
    {
        auto *f = new QFormLayout;
        combo(f, "Language", "language", {{"English", "English"}});
        g->addLayout(f);
    }

    // --- Data ---------------------------------------------------------
    g = group("Data");
    {
        auto *row = new QHBoxLayout;
        auto add = [&](const QString &text, auto &&slot, bool danger = false) {
            auto *b = new QPushButton(text);
            b->setProperty("danger", danger);
            connect(b, &QPushButton::clicked, this, slot);
            row->addWidget(b);
        };
        add("Backup Data (Export JSON)", [this] {
            const QString p = QFileDialog::getSaveFileName(this, "Backup data", m_core->defaultBackupPath(), "JSON (*.json)");
            if (!p.isEmpty())
                m_core->backupTo(p);
        });
        add("Restore Data (Import JSON)", [this] {
            const QString p = QFileDialog::getOpenFileName(this, "Restore data", m_core->dataFolder() + "/backups", "JSON (*.json)");
            if (p.isEmpty())
                return;
            if (QMessageBox::question(this, "Restore data",
                                      "This replaces all current items and settings with the backup. Continue?")
                == QMessageBox::Yes)
                m_core->restoreFrom(p);
        });
        add("Open Backup Folder", [this] { m_core->openBackupFolder(); });
        row->addStretch(1);
        g->addLayout(row);

        auto *row2 = new QHBoxLayout;
        auto add2 = [&](const QString &text, auto &&slot, bool danger = false) {
            auto *b = new QPushButton(text);
            b->setProperty("danger", danger);
            connect(b, &QPushButton::clicked, this, slot);
            row2->addWidget(b);
        };
        add2("Clear Cache", [this] { m_core->clearCache(); });
        add2("Reset All Settings", [this] {
            if (QMessageBox::question(this, "Reset settings", "Restore every setting to its default?") == QMessageBox::Yes)
                m_core->resetSettings();
        }, true);
        row2->addStretch(1);
        g->addLayout(row2);
    }

    // --- About --------------------------------------------------------
    g = group("About");
    {
        auto *row = new QHBoxLayout;
        auto *about = new QPushButton("About");
        auto *updates = new QPushButton("Check for Updates");
        connect(about, &QPushButton::clicked, this, [this] { QMessageBox::about(this, "About Launcher Hub", m_core->aboutText()); });
        connect(updates, &QPushButton::clicked, this, [this] { QMessageBox::information(this, "Check for Updates", m_core->checkForUpdates()); });
        row->addWidget(about);
        row->addWidget(updates);
        row->addStretch(1);
        g->addLayout(row);
    }
    col->addStretch(1);

    auto *heading = new QLabel("Settings");
    heading->setObjectName("h1");
    auto *scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);

    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(heading);
    lay->addSpacing(6);
    lay->addWidget(scroll, 1);
}
