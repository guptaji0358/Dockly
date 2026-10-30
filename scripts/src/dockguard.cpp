#include "dockguard.h"

#include <QCursor>
#include <QGuiApplication>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
constexpr int kTickMs = 100;   // pointer polling: fast enough for the dwell to feel immediate
constexpr int kFullEvery = 3;  // the (costlier) full-screen check runs on every 3rd tick while nothing is full-screen
constexpr int kDwellMs = 400;  // how long the pointer must rest on the edge
constexpr int kAwayMs = 500;   // how long it may stay off the dock before it hides again
}

DockGuard::DockGuard(QObject *parent)
    : QObject(parent)
{
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

void DockGuard::tick()
{
    applyClickThrough();
    // while the dock is collapsed it is click-through, so the pointer resting on it is what opens it
    if (m_clickThrough && !m_fullscreen) {
        if (m_edge.contains(QPointF(QCursor::pos()))) {
            if (!m_dwell.isValid())
                m_dwell.start();
            else if (m_dwell.elapsed() >= kDwellMs) {
                m_dwell.invalidate();
                emit edgeDwelled();
            }
        } else {
            m_dwell.invalidate();
        }
    }
    if (!m_enabled)
        return;
    const bool full = (m_fullscreen || ++m_ticks % kFullEvery == 0) ? foregroundIsFullscreen() : m_fullscreen;
    bool revealed = m_revealed;
    if (full) {
        const QPointF p = QCursor::pos();
        if (!revealed) {
            // resting on the edge for a moment opens the dock; passing over it does nothing
            if (m_edge.contains(p)) {
                if (!m_dwell.isValid())
                    m_dwell.start();
                else if (m_dwell.elapsed() >= kDwellMs)
                    revealed = true;
            } else {
                m_dwell.invalidate();
            }
            m_away.invalidate();
        } else {
            // revealed: stay until the pointer has really left the dock (and the edge it came from)
            if (m_dock.contains(p) || m_edge.contains(p)) {
                m_away.invalidate();
            } else {
                if (!m_away.isValid())
                    m_away.start();
                else if (m_away.elapsed() >= kAwayMs) {
                    revealed = false;
                    m_dwell.invalidate();
                }
            }
        }
    } else {
        m_dwell.invalidate();
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
    m_dwell.invalidate();
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
