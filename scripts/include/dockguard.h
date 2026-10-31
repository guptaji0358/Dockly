#pragma once

#include <QElapsedTimer>
#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QTimer>
#include <QWindow>

// Keeps the edge dock out of the way of heavy software, the way the macOS dock behaves:
//  - while a full-screen app (game, video, presentation, F11 browser) is in front, the dock is gone
//    completely - no edge line to block scroll bars or buttons;
//  - the pointer's behaviour on the edge (resting, pushing, sweeping past, dragging, heading away) decides
//    whether the dock slides over the app and how soon it goes away again;
//  - the dock window is lifted back above other top-most windows whenever the foreground app changes.
class DockGuard : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QWindow *window READ window WRITE setWindow NOTIFY windowChanged)
    Q_PROPERTY(QRectF edgeZone READ edgeZone WRITE setEdgeZone NOTIFY zonesChanged)
    Q_PROPERTY(QRectF dockZone READ dockZone WRITE setDockZone NOTIFY zonesChanged)
    // while true the dock window lets every click and hover through to the program underneath
    Q_PROPERTY(bool clickThrough READ clickThrough WRITE setClickThrough NOTIFY clickThroughChanged)
    // ms the pointer must stay untouched before `idle` turns true; 0 = never
    Q_PROPERTY(int idleAfter READ idleAfter WRITE setIdleAfter NOTIFY idleAfterChanged)
    Q_PROPERTY(bool idle READ idle NOTIFY idleChanged)
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(bool revealed READ revealed NOTIFY revealedChanged)
    // true while the dock should be on screen: no full-screen app, or the pointer asked for it
    Q_PROPERTY(bool dockVisible READ dockVisible NOTIFY dockVisibleChanged)
public:
    explicit DockGuard(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    QWindow *window() const { return m_window; }
    void setWindow(QWindow *w);
    QRectF edgeZone() const { return m_edge; }
    void setEdgeZone(const QRectF &r);
    QRectF dockZone() const { return m_dock; }
    void setDockZone(const QRectF &r);
    bool clickThrough() const { return m_clickThrough; }
    void setClickThrough(bool on);
    int idleAfter() const { return m_idleAfter; }
    void setIdleAfter(int ms);
    bool idle() const { return m_idle; }
    bool fullscreen() const { return m_fullscreen; }
    bool revealed() const { return m_revealed; }
    bool dockVisible() const { return !m_fullscreen || m_revealed; }

signals:
    void enabledChanged();
    void windowChanged();
    void zonesChanged();
    void clickThroughChanged();
    // the pointer rested on the collapsed dock long enough: open it
    void edgeDwelled();
    void idleAfterChanged();
    void idleChanged();
    void fullscreenChanged();
    void revealedChanged();
    void dockVisibleChanged();

private:
    void tick();
    void track(const QPointF &p);
    void trackIdle(const QPointF &p);
    bool wantsOpen(const QPointF &p);
    bool atWall(const QPointF &p) const;
    bool foregroundIsFullscreen();
    void liftWindow();
    void applyClickThrough();
    void update(bool fullscreen, bool revealed);

    bool m_enabled = true;
    QPointer<QWindow> m_window;
    QRectF m_edge, m_dock;
    bool m_clickThrough = false;
    int m_ticks = 0;
    bool m_fullscreen = false;
    bool m_revealed = false;
    QTimer m_timer;
    QElapsedTimer m_clock;
    QPointF m_last;            // pointer intent: where it was, how fast it moves, whether it nears the dock
    qint64 m_lastMs = 0;
    double m_speed = 0;
    double m_lastDist = 0;
    bool m_approaching = false;
    double m_score = 0;        // accumulated evidence that the pointer wants the dock open
    QElapsedTimer m_away;      // how long the pointer has been off the open dock
    quintptr m_lastForeground = 0;
    int m_idleAfter = 0;
    bool m_idle = false;
    QPointF m_idlePos;         // where the pointer last was when it counted as "touched"
    qint64 m_touchedMs = 0;
};
