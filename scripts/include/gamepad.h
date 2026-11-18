#pragma once

#include <QObject>
#include <QPointF>
#include <QTimer>

// First connected XInput controller (Xbox and most PC pads), polled only while enabled.
// Turns the sticks and buttons into the few signals the interface needs: a direction (left stick or d-pad),
// accept / back, shoulder buttons and a "wake" chord (both shoulders at once).
// xinput is loaded at run time, so Dockly still starts on a PC without it.
class Gamepad : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
public:
    explicit Gamepad(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    bool connected() const { return m_connected; }

signals:
    void enabledChanged();
    void connectedChanged();
    // left stick, -1..1 on both axes (y grows downwards, like the screen); sent while it is away from centre and once more at rest
    void stick(double x, double y);
    // "up" | "down" | "left" | "right" (d-pad, repeats while held) and "accept" (A) | "back" (B) | "menu" (Start) | "prev" (LB) | "next" (RB)
    void pressed(const QString &button);
    void chord(); // both shoulder buttons pressed together

private:
    void poll();

    bool m_enabled = false;
    bool m_connected = false;
    int m_user = -1;       // controller slot in use
    quint16 m_buttons = 0; // last state
    bool m_stickActive = false;
    int m_hold = 0;        // polls the current d-pad direction has been held (for repeat)
    int m_scanIn = 0;      // polls until the next look for a controller while none is connected
    QTimer m_timer;
};
