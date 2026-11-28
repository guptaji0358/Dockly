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

NativeBridge::NativeBridge(const QString &path)
    : m_lib(path.isEmpty() ? defaultPath() : path)
{
    if (!m_lib.load()) {
        m_error = m_lib.errorString();
        return;
    }
    const auto version = reinterpret_cast<decltype(&dn_abi_version)>(m_lib.resolve("dn_abi_version"));
    if (!version) {
        m_error = QStringLiteral("not a Dockly native library");
        return;
    }
    if (const uint32_t v = version(); v != DN_ABI_VERSION) {
        m_error = QStringLiteral("ABI version %1, expected %2").arg(v).arg(DN_ABI_VERSION);
        return;
    }
    m_read = reinterpret_cast<decltype(&dn_system_info_read)>(m_lib.resolve("dn_system_info_read"));
    if (!m_read)
        m_error = QStringLiteral("dn_system_info_read is missing");
}

int NativeBridge::systemInfo(dn_system_info *out) const
{
    return m_read ? m_read(out) : DN_ERR_OS;
}

