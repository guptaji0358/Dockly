#pragma once

#include "lhtypes.h"

#include <QPoint>
#include <QString>
#include <QStringList>
#include <functional>

class AppCore;
class ItemView;
class QWidget;

// Shared UI flows used by every page: open, edit, delete, context menu, dialogs.
namespace ItemActions {

void launch(QWidget *parent, AppCore *core, const QString &id);
void edit(QWidget *parent, AppCore *core, const QString &id);
void remove(QWidget *parent, AppCore *core, const QString &id);
void menu(QWidget *parent, AppCore *core, const QString &id, const QPoint &globalPos,
          const QString &inCollection = QString());

// Wires every view signal (open / menu / edit / favorite / delete) to the shared flows.
// collectionId: supplies the collection a member view shows (empty otherwise);
// deleteLeavesCollection: Delete key / button removes from that collection instead of deleting.
void connectView(ItemView *view, QWidget *parent, AppCore *core,
                 std::function<QString()> collectionId = {}, bool deleteLeavesCollection = false);

// Add flows
void addFiles(QWidget *parent, AppCore *core, LH::Category category);
void addUrl(QWidget *parent, AppCore *core);
void addSoftware(QWidget *parent, AppCore *core);
QString addCollection(QWidget *parent, AppCore *core);
void addExistingToCollection(QWidget *parent, AppCore *core, const QString &collectionId);

} // namespace ItemActions
