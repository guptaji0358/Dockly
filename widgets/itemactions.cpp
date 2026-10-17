#include "itemactions.h"

#include "appcore.h"
#include "itemview.h"

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

// Name + path/URL editor used for add and edit.
class EditItemDialog : public QDialog
{
public:
    EditItemDialog(AppCore *core, LH::Category cat, const QString &title, QWidget *parent)
        : QDialog(parent), m_core(core), m_cat(cat)
    {
        setWindowTitle(title);
        setMinimumWidth(460);
        auto *form = new QFormLayout;
        m_name = new QLineEdit;
        m_name->setPlaceholderText(cat == LH::Collection ? "Collection name" : "Optional - taken from the target");
        form->addRow("Name", m_name);

        if (cat != LH::Collection) {
            m_target = new QLineEdit;
            m_target->setPlaceholderText(cat == LH::Web ? "https://example.com" : "Choose a file...");
            auto *row = new QHBoxLayout;
            row->addWidget(m_target, 1);
            if (cat != LH::Web) {
                auto *browse = new QPushButton("Browse...");
                connect(browse, &QPushButton::clicked, this, [this] {
                    const QString f = QFileDialog::getOpenFileName(this, "Choose file", {}, m_core->fileFilter(m_cat));
                    if (!f.isEmpty())
                        m_target->setText(QDir::toNativeSeparators(f));
                });
                row->addWidget(browse);
            }
            form->addRow(cat == LH::Web ? "URL" : "Path", row);
        }

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        m_ok = buttons->button(QDialogButtonBox::Ok);
        m_ok->setProperty("primary", true);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto *lay = new QVBoxLayout(this);
        lay->addLayout(form);
        lay->addSpacing(8);
        lay->addWidget(buttons);

        auto validate = [this] {
            const bool ok = m_cat == LH::Collection ? !m_name->text().trimmed().isEmpty()
                                                    : !m_target->text().trimmed().isEmpty();
            m_ok->setEnabled(ok);
        };
        connect(m_name, &QLineEdit::textChanged, this, validate);
        if (m_target)
            connect(m_target, &QLineEdit::textChanged, this, validate);
        validate();
    }

    void setValues(const QString &name, const QString &target)
    {
        m_name->setText(name);
        if (m_target)
            m_target->setText(target);
    }
    QString name() const { return m_name->text(); }
    QString target() const { return m_target ? m_target->text() : QString(); }

private:
    AppCore *m_core;
    LH::Category m_cat;
    QLineEdit *m_name = nullptr;
    QLineEdit *m_target = nullptr;
    QPushButton *m_ok = nullptr;
};

// Multi-select list of existing items to add to a collection.
class PickItemsDialog : public QDialog
{
public:
    PickItemsDialog(AppCore *core, const QString &collectionId, QWidget *parent) : QDialog(parent)
    {
        setWindowTitle("Add Existing Cards");
        resize(460, 520);
        auto *search = new QLineEdit;
        search->setPlaceholderText("Filter...");
        m_list = new QListWidget;
        m_list->setObjectName("picker");
        for (const QVariant &v : core->addableItems(collectionId)) {
            const QVariantMap m = v.toMap();
            auto *it = new QListWidgetItem(QStringLiteral("%1   (%2)").arg(m["name"].toString(), m["category"].toString()));
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            it->setCheckState(Qt::Unchecked);
            it->setData(Qt::UserRole, m["id"]);
            m_list->addItem(it);
        }
        connect(search, &QLineEdit::textChanged, this, [this](const QString &t) {
            for (int i = 0; i < m_list->count(); ++i)
                m_list->item(i)->setHidden(!m_list->item(i)->text().contains(t, Qt::CaseInsensitive));
        });
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        buttons->button(QDialogButtonBox::Ok)->setProperty("primary", true);
        buttons->button(QDialogButtonBox::Ok)->setText("Add Selected");
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        auto *lay = new QVBoxLayout(this);
        lay->addWidget(search);
        lay->addWidget(m_list, 1);
        lay->addWidget(buttons);
    }

    QStringList selectedIds() const
    {
        QStringList ids;
        for (int i = 0; i < m_list->count(); ++i)
            if (m_list->item(i)->checkState() == Qt::Checked)
                ids << m_list->item(i)->data(Qt::UserRole).toString();
        return ids;
    }

private:
    QListWidget *m_list;
};

// Searchable list of programs from the hidden catalog; only ticked ones become cards.
class PickSoftwareDialog : public QDialog
{
public:
    PickSoftwareDialog(AppCore *core, QWidget *parent) : QDialog(parent), m_core(core)
    {
        setWindowTitle("Add Software");
        resize(520, 600);
        m_search = new QLineEdit;
        m_search->setPlaceholderText("Search installed programs...");
        m_list = new QListWidget;
        m_list->setObjectName("picker");
        m_info = new QLabel;
        m_info->setObjectName("sub");
        m_info->setWordWrap(true);

        m_scan = new QPushButton("Scan this PC");
        auto *browse = new QPushButton("Browse for .exe...");
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Cancel);
        auto *add = buttons->addButton("Add Selected", QDialogButtonBox::AcceptRole);
        add->setProperty("primary", true);
        connect(buttons, &QDialogButtonBox::accepted, this, [this] { addPicked(); accept(); });
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto *row = new QHBoxLayout;
        row->addWidget(m_scan);
        row->addWidget(browse);
        row->addStretch(1);
        auto *lay = new QVBoxLayout(this);
        lay->addWidget(m_search);
        lay->addWidget(m_list, 1);
        lay->addWidget(m_info);
        lay->addLayout(row);
        lay->addWidget(buttons);

        connect(m_search, &QLineEdit::textChanged, this, [this] { populate(); });
        connect(m_list, &QListWidget::itemChanged, this, [this](QListWidgetItem *it) {
            const QString target = it->data(Qt::UserRole).toString();
            if (it->checkState() == Qt::Checked)
                m_picked.insert(target, it->data(Qt::UserRole + 1).toString());
            else
                m_picked.remove(target);
        });
        connect(m_scan, &QPushButton::clicked, this, [this] { m_core->refreshSoftware(); });
        connect(core, &AppCore::scanningChanged, this, [this] {
            m_scan->setEnabled(!m_core->scanning());
            m_scan->setText(m_core->scanning() ? "Scanning..." : "Scan this PC");
        });
        connect(core, &AppCore::catalogChanged, this, [this] { populate(); });
        connect(browse, &QPushButton::clicked, this, [this] {
            const QStringList f = QFileDialog::getOpenFileNames(this, "Add Software", {}, m_core->fileFilter(LH::Software));
            if (!f.isEmpty()) {
                m_core->addFiles(LH::Software, f);
                accept();
            }
        });
        populate();
    }

private:
    void populate()
    {
        QSignalBlocker block(m_list);
        m_list->clear();
        const QVariantList rows = m_core->catalogSearch(m_search->text(), 300);
        for (const QVariant &v : rows) {
            const QVariantMap m = v.toMap();
            auto *it = new QListWidgetItem(m["name"].toString());
            it->setToolTip(m["target"].toString());
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            it->setCheckState(m_picked.contains(m["target"].toString()) ? Qt::Checked : Qt::Unchecked);
            it->setData(Qt::UserRole, m["target"]);
            it->setData(Qt::UserRole + 1, m["name"]);
            m_list->addItem(it);
        }
        const int total = m_core->catalogCount();
        m_info->setText(total == 0
            ? QStringLiteral("The catalog is empty. Press \"Scan this PC\" to look through your Start Menu shortcuts "
                             "(this never scans whole drives), or browse for an .exe yourself.")
            : QStringLiteral("%1 program(s) in the catalog%2. Tick the ones you want as cards.")
                  .arg(total).arg(rows.size() >= 300 ? " (showing the first 300, search to narrow)" : ""));
    }
    void addPicked()
    {
        QVariantList entries;
        for (auto it = m_picked.cbegin(); it != m_picked.cend(); ++it)
            entries.append(QVariantMap{{"target", it.key()}, {"name", it.value()}});
        if (!entries.isEmpty())
            m_core->addSoftware(entries);
    }

    AppCore *m_core;
    QLineEdit *m_search;
    QListWidget *m_list;
    QLabel *m_info;
    QPushButton *m_scan;
    QHash<QString, QString> m_picked; // target -> name
};

} // namespace

namespace ItemActions {

void edit(QWidget *parent, AppCore *core, const QString &id)
{
    const QVariantMap info = core->itemInfo(id);
    if (info.isEmpty())
        return;
    const auto cat = LH::Category(info["category"].toInt());
    EditItemDialog dlg(core, cat, cat == LH::Collection ? "Rename Collection" : "Edit Item", parent);
    dlg.setValues(info["name"].toString(), info["target"].toString());
    if (dlg.exec() == QDialog::Accepted)
        core->updateItem(id, dlg.name(), dlg.target());
}

void remove(QWidget *parent, AppCore *core, const QString &id)
{
    const QVariantMap info = core->itemInfo(id);
    if (info.isEmpty())
        return;
    if (core->settings()->boolean("confirmDelete")) {
        QMessageBox box(QMessageBox::Question, "Delete card",
                        QStringLiteral("Delete \"%1\"?").arg(info["name"].toString()),
                        QMessageBox::NoButton, parent);
        if (info["category"].toInt() == LH::Collection)
            box.setInformativeText("The items inside the collection are not deleted.");
        else if (info["category"].toInt() == LH::Software)
            box.setInformativeText("Only the card is removed; the program stays installed and in the catalog.");
        QPushButton *yes = box.addButton("Delete", QMessageBox::DestructiveRole);
        box.addButton("Cancel", QMessageBox::RejectRole);
        box.exec();
        if (box.clickedButton() != yes)
            return;
    }
    core->removeItem(id);
}

void launch(QWidget *parent, AppCore *core, const QString &id)
{
    const QVariantMap info = core->itemInfo(id);
    if (info.isEmpty())
        return;
    const int r = core->launch(id);
    if (r != LH::TargetMissing)
        return;

    // Item exists? -> No: show the warning and offer the recovery actions.
    const auto cat = LH::Category(info["category"].toInt());
    const bool isWeb = cat == LH::Web;
    QMessageBox box(QMessageBox::Warning, isWeb ? "Invalid link" : "Item missing",
                    QStringLiteral("\"%1\" can't be opened.").arg(info["name"].toString()), QMessageBox::NoButton, parent);
    box.setInformativeText(isWeb ? QStringLiteral("The URL looks invalid:\n%1").arg(info["target"].toString())
                                 : QStringLiteral("Nothing was found at:\n%1").arg(info["target"].toString()));
    QPushButton *locate = isWeb ? nullptr : box.addButton("Locate Again", QMessageBox::ActionRole);
    QPushButton *editBtn = box.addButton(isWeb ? "Edit URL" : "Edit Path", QMessageBox::ActionRole);
    QPushButton *del = box.addButton("Delete", QMessageBox::DestructiveRole);
    box.addButton("Cancel", QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == locate) {
        const QString f = QFileDialog::getOpenFileName(parent, "Locate " + info["name"].toString(), {}, core->fileFilter(cat));
        if (!f.isEmpty())
            core->updateItem(id, info["name"].toString(), f);
    } else if (box.clickedButton() == editBtn) {
        edit(parent, core, id);
    } else if (box.clickedButton() == del) {
        remove(parent, core, id);
    }
}

void menu(QWidget *parent, AppCore *core, const QString &id, const QPoint &globalPos, const QString &inCollection)
{
    const QVariantMap info = core->itemInfo(id);
    if (info.isEmpty())
        return;
    const auto cat = LH::Category(info["category"].toInt());
    const bool fav = info["favorite"].toBool();

    QMenu m(parent);
    m.addAction(cat == LH::Collection ? "Launch All" : "Open", [=] { launch(parent, core, id); });
    m.addAction(fav ? "Remove from Favorites" : "Add to Favorites", [=] { core->setFavorite(id, !fav); });
    m.addSeparator();
    m.addAction(cat == LH::Collection ? "Rename..." : "Edit...", [=] { edit(parent, core, id); });
    if (info["status"].toInt() != LH::Normal && cat != LH::Web && cat != LH::Collection) {
        m.addAction("Locate Again...", [=] {
            const QString f = QFileDialog::getOpenFileName(parent, "Locate " + info["name"].toString(), {}, core->fileFilter(cat));
            if (!f.isEmpty())
                core->updateItem(id, info["name"].toString(), f);
        });
    }
    if (cat != LH::Web && cat != LH::Collection)
        m.addAction("Show in Folder", [=] { core->showInFolder(id); });

    if (cat != LH::Collection) {
        QMenu *sub = m.addMenu("Add to Collection");
        const QVariantList cols = core->collectionsList();
        for (const QVariant &c : cols) {
            const QVariantMap cm = c.toMap();
            sub->addAction(cm["name"].toString(), [=] { core->addToCollection(cm["id"].toString(), {id}); });
        }
        if (!cols.isEmpty())
            sub->addSeparator();
        sub->addAction("New Collection...", [=] {
            const QString cid = addCollection(parent, core);
            if (!cid.isEmpty())
                core->addToCollection(cid, {id});
        });
    }
    if (!inCollection.isEmpty())
        m.addAction("Remove from this Collection", [=] { core->removeFromCollection(inCollection, id); });

    m.addSeparator();
    m.addAction("Delete", [=] { remove(parent, core, id); });
    m.exec(globalPos);
}

void connectView(ItemView *view, QWidget *parent, AppCore *core, std::function<QString()> collectionId,
                 bool deleteLeavesCollection)
{
    auto coll = [collectionId] { return collectionId ? collectionId() : QString(); };
    QObject::connect(view, &ItemView::launchRequested, parent, [=](const QString &id) { launch(parent, core, id); });
    QObject::connect(view, &ItemView::menuRequested, parent,
                     [=](const QString &id, const QPoint &p) { menu(parent, core, id, p, coll()); });
    QObject::connect(view, &ItemView::editRequested, parent, [=](const QString &id) { edit(parent, core, id); });
    QObject::connect(view, &ItemView::favoriteRequested, parent, [=](const QString &id) {
        core->setFavorite(id, !core->itemInfo(id)["favorite"].toBool());
    });
    QObject::connect(view, &ItemView::deleteRequested, parent, [=](const QString &id) {
        if (deleteLeavesCollection && !coll().isEmpty())
            core->removeFromCollection(coll(), id);
        else
            remove(parent, core, id);
    });
}

void addFiles(QWidget *parent, AppCore *core, LH::Category category)
{
    const QString title = category == LH::Document ? "Add Document" : "Add Media";
    const QStringList files = QFileDialog::getOpenFileNames(parent, title, {}, core->fileFilter(category));
    if (!files.isEmpty())
        core->addFiles(category, files);
}

void addSoftware(QWidget *parent, AppCore *core)
{
    PickSoftwareDialog dlg(core, parent);
    dlg.exec();
}

void addUrl(QWidget *parent, AppCore *core)
{
    EditItemDialog dlg(core, LH::Web, "Add URL", parent);
    if (dlg.exec() == QDialog::Accepted)
        core->addItem(LH::Web, dlg.name(), dlg.target());
}

QString addCollection(QWidget *parent, AppCore *core)
{
    EditItemDialog dlg(core, LH::Collection, "Create Collection", parent);
    if (dlg.exec() == QDialog::Accepted)
        return core->createCollection(dlg.name());
    return {};
}

void addExistingToCollection(QWidget *parent, AppCore *core, const QString &collectionId)
{
    PickItemsDialog dlg(core, collectionId, parent);
    if (dlg.exec() == QDialog::Accepted)
        core->addToCollection(collectionId, dlg.selectedIds());
}

} // namespace ItemActions
