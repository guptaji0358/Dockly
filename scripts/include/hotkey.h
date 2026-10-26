#pragma once

#include <QAbstractNativeEventFilter>
#include <QObject>

// One system-wide keyboard shortcut (Windows RegisterHotKey), e.g. "Ctrl+Alt+D".
class GlobalHotkey : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    explicit GlobalHotkey(QObject *parent = nullptr);
    ~GlobalHotkey() override;

    // Replaces the current shortcut; an empty text just unregisters. False when the text isn't
    // understood or another program already owns the combination.
    bool setSequence(const QString &text);

    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;

signals:
    void triggered();

private:
    void unregisterKey();
    bool m_registered = false;
};
