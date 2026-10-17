#pragma once

#include <QMainWindow>
#include <QTimer>

class AppCore;
class HomePage;
class QFrame;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class TrayController;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(AppCore *core, QWidget *parent = nullptr);

    void goTo(int index);
    bool trayActive() const;

public slots:
    void showFromTray();

protected:
    void closeEvent(QCloseEvent *e) override;
    void changeEvent(QEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;
    void dragEnterEvent(QDragEnterEvent *e) override;
    void dragLeaveEvent(QDragLeaveEvent *e) override;
    void dropEvent(QDropEvent *e) override;

private:
    void applyTheme();
    void applyNavIcons();
    void toast(const QString &text);
    static bool acceptsDrop(const class QMimeData *m);

    AppCore *m_core;
    QStackedWidget *m_stack;
    QList<QPushButton *> m_nav;      // icon-rail buttons
    QList<QPushButton *> m_dockBtns; // floating-dock buttons
    QFrame *m_sidebar;
    QFrame *m_dock;
    QLabel *m_logo;
    QLabel *m_brand;                 // shown next to the search bar in dock mode
    QLineEdit *m_search;
    HomePage *m_home;
    QLabel *m_toast;
    QLabel *m_dropOverlay;
    QTimer m_toastTimer;
    TrayController *m_tray;
    bool m_quitting = false;
};
