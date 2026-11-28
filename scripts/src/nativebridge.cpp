#include "nativebridge.h"

#include <cstddef>
#include <iterator>

#include <QCoreApplication>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// the same layout checks exist in native/src/sysinfo.rs
static_assert(sizeof(dn_system_info) == 48, "dn_system_info layout");
static_assert(offsetof(dn_system_info, total_physical_bytes) == 16, "dn_system_info layout");
static_assert(offsetof(dn_system_info, os_major) == 32, "dn_system_info layout");
static_assert(offsetof(dn_system_info, memory_load_percent) == 44, "dn_system_info layout");

// Always a full path, so the DLL search order never decides which file gets loaded.
QString NativeBridge::defaultPath()
{
#ifdef Q_OS_WIN
    HMODULE self = nullptr;
    wchar_t buf[32768];
    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           reinterpret_cast<LPCWSTR>(&NativeBridge::defaultPath), &self)) {
        const DWORD n = GetModuleFileNameW(self, buf, DWORD(std::size(buf)));
        if (n > 0 && n < std::size(buf)) {
            const QString module = QDir::fromNativeSeparators(QString::fromWCharArray(buf, int(n)));
            return module.section(QLatin1Char('/'), 0, -2) + QStringLiteral("/dockly_native.dll");
        }
    }
#endif
    return QCoreApplication::applicationDirPath() + QStringLiteral("/dockly_native.dll");
}

