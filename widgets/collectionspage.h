#pragma once

#include <QWidget>

class AppCore;
class ItemModel;
class ItemView;
class QLabel;
class QPushButton;

// Custom hub: list of collections on the left, the selected collection's cards on the right.
class CollectionsPage : public QWidget
{
    Q_OBJECT
public:
    explicit CollectionsPage(AppCore *core, QWidget *parent = nullptr);

private:
    void selectionChanged();
    void select(const QString &id);

    AppCore *m_core;
    ItemModel *m_collections;
    ItemModel *m_members;
    ItemView *m_collectionView;
    ItemView *m_memberView;
    QLabel *m_title;
    QPushButton *m_launchAll;
    QPushButton *m_addCards;
    QPushButton *m_rename;
    QPushButton *m_delete;
    QString m_current;
};
