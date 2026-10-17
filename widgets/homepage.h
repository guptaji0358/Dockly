#pragma once

#include <QWidget>

class AppCore;
class ItemModel;
class ItemView;
class QLabel;
class QStackedWidget;

// Home: favorites + recent. While searching, shows matches across every hub instead.
class HomePage : public QWidget
{
    Q_OBJECT
public:
    explicit HomePage(AppCore *core, QWidget *parent = nullptr);

    void setSearch(const QString &text);

private:
    AppCore *m_core;
    QStackedWidget *m_stack;
    ItemModel *m_favModel;
    ItemModel *m_recentModel;
    ItemModel *m_searchModel;
    ItemView *m_favView;
    ItemView *m_recentView;
    ItemView *m_searchView;
    QLabel *m_recentHeader;
    QLabel *m_resultsHeader;
};
