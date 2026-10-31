#include "dockguard.h"

#include <QCursor>
#include <QLineF>
#include <QGuiApplication>
#include <QScreen>

#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
constexpr int kTickMs = 100;   // pointer polling: fast enough for the dwell to feel immediate
constexpr int kFullEvery = 3;  // the (costlier) full-screen check runs on every 3rd tick while nothing is full-screen
constexpr double kOpenScore = 300;    // "evidence" (ms of calm resting on the edge) needed to open
constexpr double kSweepSpeed = 900;   // px/s: faster than this along the edge is a pass-by, not a visit
constexpr double kBrowseSpeed = 250;  // px/s: between this and a sweep the pointer is working along the edge
constexpr double kLeaveSpeed = 700;   // px/s: leaving the dock at this speed means "I'm done"
constexpr int kAwayMs = 500;          // how long a hesitant pointer may stay off the dock before it hides
constexpr int kLeaveMs = 150;         // ... and a pointer that is clearly heading away
constexpr double kWall = 1.5;         // px from the screen boundary that counts as pushing against it
constexpr double kHeadingPx = 160;    // "heading for the dock" only counts this close; further out the pointer is just roaming
constexpr double kFarPx = 350;        // this far from the open dock the pointer is plainly elsewhere: hide at once
constexpr int kFarHideMs = 500;       // the line tucks away this long after the pointer leaves its area, moving or not
constexpr double kTouchPx = 3;        // pointer movement that counts as touching the mouse or trackpad
constexpr double kNearPx = 90;        // hidden line: it starts to peek out when the pointer is this close
constexpr double kWakePx = 30;        // ... and is fully back (countdown restarts) once the pointer is this close

// The real button state from Windows: Qt only knows about presses its own windows received, and the
// collapsed dock is click-through, so a press elsewhere (or one that never got its release) can look stuck.
bool buttonHeld()
{
#ifdef Q_OS_WIN
    return (GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON) | GetAsyncKeyState(VK_MBUTTON)) & 0x8000;
#else
    return QGuiApplication::mouseButtons() != Qt::NoButton;
#endif
}

double distanceTo(const QRectF &r, const QPointF &p)
{
    const double dx = qMax(qMax(r.left() - p.x(), 0.0), p.x() - r.right());
    const double dy = qMax(qMax(r.top() - p.y(), 0.0), p.y() - r.bottom());
    return std::hypot(dx, dy);
}
}

DockGuard::DockGuard(QObject *parent)
    : QObject(parent)
{
    m_clock.start();
    m_last = QCursor::pos();
    m_idlePos = m_last;
    m_timer.setInterval(kTickMs);
    connect(&m_timer, &QTimer::timeout, this, &DockGuard::tick);
    m_timer.start();
}

void DockGuard::setEnabled(bool on)
{
    if (m_enabled == on)
        return;
    m_enabled = on;
    if (!on)
        update(false, false);
    emit enabledChanged();
}

void DockGuard::setWindow(QWindow *w)
{
    if (m_window == w)
        return;
    m_window = w;
    emit windowChanged();
}

void DockGuard::setEdgeZone(const QRectF &r)
{
    if (m_edge == r)
        return;
    m_edge = r;
    emit zonesChanged();
}

void DockGuard::setDockZone(const QRectF &r)
{
    if (m_dock == r)
        return;
    m_dock = r;
    emit zonesChanged();
}

void DockGuard::update(bool fullscreen, bool revealed)
{
    if (!fullscreen)
        revealed = false;
    const bool before = dockVisible();
    if (m_fullscreen != fullscreen) {
        m_fullscreen = fullscreen;
        emit fullscreenChanged();
    }
    if (m_revealed != revealed) {
        m_revealed = revealed;
        emit revealedChanged();
    }
    if (before != dockVisible())
        emit dockVisibleChanged();
}

void DockGuard::setIdleAfter(int ms)
{
    ms = qMax(ms, 0);
    if (m_idleAfter == ms)
        return;
    m_idleAfter = ms;
    emit idleAfterChanged();
}

// Idle = the pointer has not moved (or been clicked) for idleAfter ms. Any touch ends it at once.
void DockGuard::trackIdle(const QPointF &p)
{
    const qint64 now = m_clock.elapsed();
    // Once the line has hidden, moving the pointer around the screen leaves it hidden: only reaching for the
    // edge (or a click) brings it back. Before that, any movement counts and restarts the countdown.
    const double d = distanceTo(m_edge, p);
    const bool closeEnough = d <= (m_idle ? kWakePx : kNearPx);
    if ((QLineF(m_idlePos, p).length() > kTouchPx || buttonHeld()) && closeEnough) {
        m_idlePos = p;
        m_touchedMs = now;
    } else if (QLineF(m_idlePos, p).length() > kTouchPx) {
        m_idlePos = p; // moved elsewhere while hidden: not a touch, but track from here
    }
    // pointer away from the line: tuck it in after a moment even while the mouse keeps moving
    if (d <= kNearPx)
        m_farSince = -1;
    else if (m_farSince < 0)
        m_farSince = now;
    const bool away = m_farSince >= 0 && now - m_farSince >= kFarHideMs;
    if (away && m_idleAfter > 0)
        m_touchedMs = qMin(m_touchedMs, now - m_idleAfter); // stays hidden until the pointer returns close
    const bool idle = m_idleAfter > 0 && (now - m_touchedMs >= m_idleAfter || away);
    // while hidden, the line peeks out in proportion to how close the pointer is (0 = far, 1 = at the wake distance)
    const double prox = idle ? qBound(0.0, (kNearPx - d) / (kNearPx - kWakePx), 1.0) : 0.0;
    if (std::abs(prox - m_proximity) > 0.01) {
        m_proximity = prox;
        emit proximityChanged();
    }
    if (m_idle != idle) {
        m_idle = idle;
        emit idleChanged();
    }
}

// Pointer speed and heading, smoothed over a couple of ticks.
void DockGuard::track(const QPointF &p)
{
    const qint64 now = m_clock.elapsed();
    const double dt = qMax<qint64>(now - m_lastMs, 1) / 1000.0;
    m_speed = m_speed * 0.5 + QLineF(m_last, p).length() / dt * 0.5;
    const double d = distanceTo(m_dock, p);
    m_approaching = d < m_lastDist - 1;
    m_lastDist = d;
    m_last = p;
    m_lastMs = now;
}

// Pushing against the screen boundary the dock lives on (the pointer cannot go further).
bool DockGuard::atWall(const QPointF &p) const
{
    const QScreen *s = QGuiApplication::screenAt(m_edge.center().toPoint());
    if (!s)
        return false;
    const QRectF g = s->geometry();
    const QPointF c = m_edge.center();
    const double l = std::abs(c.x() - g.left()), r = std::abs(g.right() - c.x());
    const double t = std::abs(c.y() - g.top()), b = std::abs(g.bottom() - c.y());
    const double m = qMin(qMin(l, r), qMin(t, b));
    if (m == l) return p.x() <= g.left() + kWall;
    if (m == r) return p.x() >= g.right() - kWall;
    if (m == t) return p.y() <= g.top() + kWall;
    return p.y() >= g.bottom() - kWall;
}

// Does the pointer's behaviour on the edge say "I want the dock"? Every tick adds or removes evidence:
//  - resting on the edge builds it steadily; pushing against the screen boundary builds it twice as fast;
//  - working along the edge (slow scroll-bar or tab-strip use) builds it slowly;
//  - sweeping past, or holding a mouse button (dragging a scroll bar or window), builds none and drains it;
//  - leaving the edge drains it quickly rather than wiping it, so a jittery hand does not lose its progress.
bool DockGuard::wantsOpen(const QPointF &p)
{
    if (!m_edge.contains(p) || buttonHeld()) {
        m_score = qMax(0.0, m_score - 2 * kTickMs);
        return false;
    }
    double gain = kTickMs;
    if (m_speed > kSweepSpeed)
        gain = -kTickMs;
    else if (m_speed > kBrowseSpeed)
        gain = kTickMs / 3.0;
    else if (atWall(p))
        gain = 2.0 * kTickMs;
    m_score = qMax(0.0, m_score + gain);
    if (m_score < kOpenScore)
        return false;
    m_score = 0;
    return true;
}

void DockGuard::tick()
{
    applyClickThrough();
    const QPointF p = QCursor::pos();
    track(p);
    trackIdle(p);
    const bool intent = wantsOpen(p);
    // while the dock is collapsed it is click-through, so the pointer resting on it is what opens it
    if (m_clickThrough && !m_fullscreen && intent)
        emit edgeDwelled();
    if (!m_enabled)
        return;
    const bool full = (m_fullscreen || ++m_ticks % kFullEvery == 0) ? foregroundIsFullscreen() : m_fullscreen;
    bool revealed = m_revealed;
    if (full) {
        if (!revealed) {
            if (intent)
                revealed = true;
            m_away.invalidate();
        } else {
            // revealed: stay while the pointer is on the dock, or heading for it; go sooner when it clearly left
            m_score = 0;
            const double dist = distanceTo(m_dock, p);
            // a button held while the pointer is away (dragging something elsewhere) never keeps the dock up
            const bool held = buttonHeld();
            if (m_dock.contains(p) || m_edge.contains(p) || (!held && m_approaching && dist < kHeadingPx)) {
                m_away.invalidate();
            } else if (held && dist > kNearPx) {
                revealed = false;
            } else {
                if (!m_away.isValid())
                    m_away.start();
                else if (m_away.elapsed() >= (m_speed > kLeaveSpeed || dist > kFarPx ? kLeaveMs : kAwayMs))
                    revealed = false;
            }
        }
    } else {
        m_away.invalidate();
    }
    update(full, revealed);
    if (!full)
        liftWindow();
}

bool DockGuard::foregroundIsFullscreen()
{
#ifdef Q_OS_WIN
    const HWND fg = GetForegroundWindow();
    if (!fg)
        return false;

    // Direct3D exclusive full screen or presentation mode: Windows tells us directly.
    QUERY_USER_NOTIFICATION_STATE state;
    if (SUCCEEDED(SHQueryUserNotificationState(&state))
        && (state == QUNS_RUNNING_D3D_FULL_SCREEN || state == QUNS_PRESENTATION_MODE))
        return true;

    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == GetCurrentProcessId())
        return false; // never hide the dock because of Dockly's own windows
    if (!IsWindowVisible(fg) || IsIconic(fg))
        return false;

    wchar_t cls[64] = {};
    GetClassNameW(fg, cls, 64);
    const QString name = QString::fromWCharArray(cls);
    if (name == QLatin1String("Progman") || name == QLatin1String("WorkerW")
        || name == QLatin1String("Shell_TrayWnd") || name == QLatin1String("Shell_SecondaryTrayWnd"))
        return false; // the desktop and the taskbar always span the screen

    // only a full-screen app on the screen the dock lives on matters
    const HMONITOR primary = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
    if (MonitorFromWindow(fg, MONITOR_DEFAULTTONEAREST) != primary)
        return false;
    MONITORINFO mi = {sizeof(mi)};
    RECT r;
    if (!GetMonitorInfoW(primary, &mi) || !GetWindowRect(fg, &r))
        return false;
    const bool covers = r.left <= mi.rcMonitor.left && r.top <= mi.rcMonitor.top
                        && r.right >= mi.rcMonitor.right && r.bottom >= mi.rcMonitor.bottom;
    if (!covers)
        return false;
    // an ordinary maximised window (with a title bar) is not full screen, even with an auto-hidden taskbar
    const LONG style = GetWindowLongW(fg, GWL_STYLE);
    return (style & WS_CAPTION) != WS_CAPTION;
#else
    return false;
#endif
}

void DockGuard::setClickThrough(bool on)
{
    if (m_clickThrough == on)
        return;
    m_clickThrough = on;
    m_score = 0;
    applyClickThrough();
    emit clickThroughChanged();
}

// WS_EX_TRANSPARENT makes Windows route mouse input to whatever is beneath the window.
void DockGuard::applyClickThrough()
{
#ifdef Q_OS_WIN
    if (!m_window || !m_window->isVisible())
        return;
    const HWND h = reinterpret_cast<HWND>(m_window->winId());
    const LONG_PTR ex = GetWindowLongPtrW(h, GWL_EXSTYLE);
    const LONG_PTR want = m_clickThrough ? (ex | WS_EX_TRANSPARENT | WS_EX_LAYERED) : (ex & ~LONG_PTR(WS_EX_TRANSPARENT));
    if (want != ex)
        SetWindowLongPtrW(h, GWL_EXSTYLE, want);
#endif
}

// A program that went top-most after the dock can sit above it; put the dock back on top when the
// foreground app changes. SWP_NOACTIVATE keeps focus where the user is working.
void DockGuard::liftWindow()
{
#ifdef Q_OS_WIN
    const HWND fg = GetForegroundWindow();
    if (reinterpret_cast<quintptr>(fg) == m_lastForeground)
        return;
    m_lastForeground = reinterpret_cast<quintptr>(fg);
    if (!m_window || !m_window->isVisible())
        return;
    SetWindowPos(reinterpret_cast<HWND>(m_window->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
#endif
}
