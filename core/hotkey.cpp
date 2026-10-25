#include "hotkey.h"

#include <QCoreApplication>
#include <QDebug>
#include <QKeySequence>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
constexpr int kHotkeyId = 0x444B; // "DK"
}

GlobalHotkey::GlobalHotkey(QObject *parent)
    : QObject(parent)
{
    QCoreApplication::instance()->installNativeEventFilter(this);
}

GlobalHotkey::~GlobalHotkey()
{
    unregisterKey();
    QCoreApplication::instance()->removeNativeEventFilter(this);
}

void GlobalHotkey::unregisterKey()
{
#ifdef Q_OS_WIN
    if (m_registered)
        UnregisterHotKey(nullptr, kHotkeyId);
#endif
    m_registered = false;
}

bool GlobalHotkey::setSequence(const QString &text)
{
    unregisterKey();
#ifdef Q_OS_WIN
    if (text.trimmed().isEmpty())
        return true;
    const QKeyCombination kc = QKeySequence::fromString(text, QKeySequence::PortableText)[0];
    const Qt::Key key = kc.key();
    UINT vk = 0;
    if ((key >= Qt::Key_A && key <= Qt::Key_Z) || (key >= Qt::Key_0 && key <= Qt::Key_9))
        vk = UINT(key); // the virtual-key codes of letters and digits are their ASCII codes
    else if (key >= Qt::Key_F1 && key <= Qt::Key_F24)
        vk = VK_F1 + UINT(key - Qt::Key_F1);
    else if (key == Qt::Key_Space)
        vk = VK_SPACE;
    if (!vk) {
        qWarning() << "hotkey: can't use" << text;
        return false;
    }
    UINT mods = MOD_NOREPEAT;
    const Qt::KeyboardModifiers m = kc.keyboardModifiers();
    if (m & Qt::ControlModifier) mods |= MOD_CONTROL;
    if (m & Qt::AltModifier) mods |= MOD_ALT;
    if (m & Qt::ShiftModifier) mods |= MOD_SHIFT;
    if (m & Qt::MetaModifier) mods |= MOD_WIN;
    m_registered = RegisterHotKey(nullptr, kHotkeyId, mods, vk);
    if (!m_registered)
        qWarning() << "hotkey:" << text << "is already used by another program";
    return m_registered;
#else
    Q_UNUSED(text)
    return false;
#endif
}

bool GlobalHotkey::nativeEventFilter(const QByteArray &eventType, void *message, qintptr *)
{
#ifdef Q_OS_WIN
    if (eventType == "windows_generic_MSG") {
        const MSG *msg = static_cast<const MSG *>(message);
        if (msg->message == WM_HOTKEY && int(msg->wParam) == kHotkeyId) {
            emit triggered();
            return true;
        }
    }
#else
    Q_UNUSED(eventType)
    Q_UNUSED(message)
#endif
    return false;
}
