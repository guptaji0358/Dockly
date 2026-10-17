#pragma once

#include "lhtypes.h"

#include <QWidget>

class AppCore;
class ItemModel;
class ItemView;
class QLabel;

// One hub: Software, Documents, Entertainment or Web Shortcuts.
class CategoryPage : public QWidget
{
    Q_OBJECT
public:
    CategoryPage(AppCore *core, LH::Category category, QWidget *parent = nullptr);

private:
    AppCore *m_core;
    LH::Category m_category;
    ItemModel *m_model;
    ItemView *m_view;
    QLabel *m_count;
};
