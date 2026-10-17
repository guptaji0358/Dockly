#pragma once

#include <QFrame>

class AppCore;
class QLabel;
class QLineEdit;
class QPushButton;

// Side panel for the selected card: full editing, open, favorite, locate, delete.
class DetailPanel : public QFrame
{
    Q_OBJECT
public:
    explicit DetailPanel(AppCore *core, QWidget *parent = nullptr);

private:
    void load();
    void refreshState();

    AppCore *m_core;
    QString m_id;
    bool m_dirty = false;
    int m_category = 0;

    QLabel *m_icon;
    QLabel *m_tag;
    QLineEdit *m_name;
    QLabel *m_targetLabel;
    QLineEdit *m_target;
    QPushButton *m_browse;
    QLabel *m_status;
    QPushButton *m_open;
    QPushButton *m_save;
    QPushButton *m_fav;
    QPushButton *m_locate;
    QPushButton *m_delete;
};
