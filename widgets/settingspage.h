#pragma once

#include <QWidget>

class AppCore;

// Every option from the Settings tab of the flowchart.
class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(AppCore *core, QWidget *parent = nullptr);

private:
    AppCore *m_core;
};
