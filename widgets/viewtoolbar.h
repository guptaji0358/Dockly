#pragma once

#include <QFrame>

class AppCore;
class QSlider;
class QToolButton;

// Grid / List / Compact switch + card-size slider, shown on every page.
class ViewToolbar : public QFrame
{
    Q_OBJECT
public:
    explicit ViewToolbar(AppCore *core, QWidget *parent = nullptr);

private:
    void applyIcons();
    void syncFromSettings();

    AppCore *m_core;
    QToolButton *m_buttons[3];
    QSlider *m_size;
};
