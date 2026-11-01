#include "gamepad.h"

#include <algorithm>
#include <cmath>

#ifdef Q_OS_WIN
#include <windows.h>

namespace {
// the parts of xinput.h we need (it isn't always in the MinGW headers)
struct PadState {
    WORD buttons;
    BYTE leftTrigger, rightTrigger;
    SHORT lx, ly, rx, ry;
};
struct PadPacket {
    DWORD packet;
    PadState gamepad;
};
using GetStateFn = DWORD(WINAPI *)(DWORD, PadPacket *);

constexpr WORD kUp = 0x0001, kDown = 0x0002, kLeft = 0x0004, kRight = 0x0008, kStart = 0x0010;
constexpr WORD kLb = 0x0100, kRb = 0x0200, kA = 0x1000, kB = 0x2000;

GetStateFn loadXInput()
{
    static GetStateFn fn = [] {
        for (const wchar_t *dll : {L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll"})
            if (HMODULE m = LoadLibraryW(dll))
                if (auto f = reinterpret_cast<GetStateFn>(GetProcAddress(m, "XInputGetState")))
                    return f;
        return GetStateFn(nullptr);
    }();
    return fn;
}
} // namespace
#endif

Gamepad::Gamepad(QObject *parent)
    : QObject(parent)
{
    m_timer.setInterval(33); // ~30 polls a second is plenty for menus
    connect(&m_timer, &QTimer::timeout, this, &Gamepad::poll);
}

void Gamepad::setEnabled(bool on)
{
    if (m_enabled == on)
        return;
    m_enabled = on;
    if (on) {
        m_scanIn = 0;
        m_timer.start();
    } else {
        m_timer.stop();
        if (m_connected) {
            m_connected = false;
            emit connectedChanged();
        }
    }
    emit enabledChanged();
}

void Gamepad::poll()
{
#ifdef Q_OS_WIN
    const GetStateFn getState = loadXInput();
    if (!getState) {
        m_timer.stop(); // no xinput on this PC
        return;
    }
    // looking for a controller is slowish, so while none is plugged in only check about once a second
    if (m_user < 0) {
        if (m_scanIn-- > 0)
            return;
        m_scanIn = 30;
        PadPacket p{};
        for (int u = 0; u < 4; ++u)
            if (getState(DWORD(u), &p) == ERROR_SUCCESS) {
                m_user = u;
                break;
            }
        if (m_user < 0)
            return;
        m_buttons = p.gamepad.buttons;
        m_connected = true;
        emit connectedChanged();
    }

    PadPacket p{};
    if (getState(DWORD(m_user), &p) != ERROR_SUCCESS) { // unplugged
        m_user = -1;
        m_buttons = 0;
        m_connected = false;
        emit connectedChanged();
        return;
    }
    const WORD now = p.gamepad.buttons;
    const WORD fresh = now & ~m_buttons;

    // left stick with a dead zone
    constexpr double dead = 7849.0, range = 32767.0 - 7849.0;
    double sx = p.gamepad.lx, sy = -double(p.gamepad.ly);
    const double mag = std::hypot(sx, sy);
    if (mag > dead) {
        const double k = std::min(1.0, (mag - dead) / range) / mag;
        emit stick(sx * k, sy * k);
        m_stickActive = true;
    } else if (m_stickActive) {
        emit stick(0, 0);
        m_stickActive = false;
    }

    auto fire = [this](const char *name) { emit pressed(QLatin1String(name)); };
    if (fresh & kA) fire("accept");
    if (fresh & kB) fire("back");
    if (fresh & kStart) fire("menu");
    if (fresh & kLb) fire("prev");
    if (fresh & kRb) fire("next");
    if ((now & kLb) && (now & kRb) && (fresh & (kLb | kRb)))
        emit chord();

    // d-pad: press, then repeat after a short hold
    const WORD dirs = now & (kUp | kDown | kLeft | kRight);
    if (!dirs) {
        m_hold = 0;
    } else {
        const bool again = (fresh & dirs) || (++m_hold > 12 && m_hold % 4 == 0);
        if (fresh & dirs)
            m_hold = 0;
        if (again) {
            if (dirs & kUp) fire("up");
            if (dirs & kDown) fire("down");
            if (dirs & kLeft) fire("left");
            if (dirs & kRight) fire("right");
        }
    }
    m_buttons = now;
#endif
}
