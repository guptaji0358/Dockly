#include "nativebridge.h"

#include <cstddef>
#include <iterator>
#include <type_traits>

#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

// the same layout checks exist beside each #[repr(C)] struct in native/src
static_assert(sizeof(dn_system_info) == 48, "dn_system_info layout");
static_assert(offsetof(dn_system_info, total_physical_bytes) == 16, "dn_system_info layout");
static_assert(offsetof(dn_system_info, os_major) == 32, "dn_system_info layout");
static_assert(offsetof(dn_system_info, memory_load_percent) == 44, "dn_system_info layout");
static_assert(sizeof(dn_memory_info) == 136, "dn_memory_info layout");
static_assert(offsetof(dn_memory_info, working_set_bytes) == 8, "dn_memory_info layout");
static_assert(offsetof(dn_memory_info, committed_private_bytes) == 48, "dn_memory_info layout");
static_assert(offsetof(dn_memory_info, native_frees) == 128, "dn_memory_info layout");
static_assert(sizeof(dn_device) == 928, "dn_device layout");
static_assert(offsetof(dn_device, total_bytes) == 16, "dn_device layout");
static_assert(offsetof(dn_device, name) == 32, "dn_device layout");
static_assert(offsetof(dn_device, id) == 288, "dn_device layout");
static_assert(offsetof(dn_device, detail) == 800, "dn_device layout");
static_assert(sizeof(dn_watch_counts) == 24, "dn_watch_counts layout");
static_assert(offsetof(dn_watch_counts, arrivals) == 8, "dn_watch_counts layout");

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
    // all or nothing: a library that lacks one export is not used at all
    const auto take = [this](auto &slot, const char *name) {
        slot = reinterpret_cast<std::remove_reference_t<decltype(slot)>>(m_lib.resolve(name));
        if (!slot && m_error.isEmpty())
            m_error = QStringLiteral("%1 is missing").arg(QLatin1String(name));
    };
    take(m_api.systemInfo, "dn_system_info_read");
    take(m_api.memoryInfo, "dn_memory_info_read");
    take(m_api.deviceList, "dn_device_list");
    take(m_api.watchOpen, "dn_watch_open");
    take(m_api.watchRead, "dn_watch_read");
    take(m_api.watchClose, "dn_watch_close");
    m_available = m_error.isEmpty();
}

int NativeBridge::systemInfo(dn_system_info *out) const
{
    return m_available ? m_api.systemInfo(out) : DN_ERR_OS;
}

int NativeBridge::memoryInfo(dn_memory_info *out) const
{
    return m_available ? m_api.memoryInfo(out) : DN_ERR_OS;
}

int NativeBridge::deviceList(uint32_t kind, dn_device *items, uint32_t capacity, uint32_t itemSize,
                             uint32_t *count) const
{
    return m_available ? m_api.deviceList(kind, items, capacity, itemSize, count) : DN_ERR_OS;
}

int NativeBridge::devices(uint32_t kind, std::vector<dn_device> *out) const
{
    if (!out)
        return DN_ERR_ARG;
    // start with room for a typical list; grow when there are more devices (or more arrive meanwhile)
    std::vector<dn_device> buffer(32);
    for (int attempt = 0; attempt < 4; ++attempt) {
        uint32_t count = 0;
        const int status = deviceList(kind, buffer.data(), uint32_t(buffer.size()), sizeof(dn_device), &count);
        if (status == DN_OK) {
            buffer.resize(count);
            *out = std::move(buffer);
            return DN_OK;
        }
        if (status != DN_ERR_MORE)
            return status;
        buffer.resize(count + 8);
    }
    return DN_ERR_MORE;
}

int NativeBridge::watchOpen(uint64_t *handle) const
{
    return m_available ? m_api.watchOpen(handle) : DN_ERR_OS;
}

int NativeBridge::watchRead(uint64_t handle, dn_watch_counts *out) const
{
    return m_available ? m_api.watchRead(handle, out) : DN_ERR_OS;
}

int NativeBridge::watchClose(uint64_t handle) const
{
    return m_available ? m_api.watchClose(handle) : DN_ERR_OS;
}

QString NativeBridge::summary() const
{
    if (!isAvailable())
        return QStringLiteral("native layer unavailable: %1").arg(m_error);
    dn_system_info i{};
    i.struct_size = sizeof(i);
    if (const int status = systemInfo(&i); status != DN_OK)
        return QStringLiteral("native layer: system info failed (status %1)").arg(status);
    return QStringLiteral("native layer ABI %1: Windows %2.%3.%4, %5 cores / %6 logical, %7 MB RAM")
        .arg(DN_ABI_VERSION).arg(i.os_major).arg(i.os_minor).arg(i.os_build)
        .arg(i.physical_cores).arg(i.logical_processors).arg(i.total_physical_bytes / (1024 * 1024));
}

namespace {
// JSON numbers are doubles: exact up to 2^53, far beyond any byte count here
QJsonValue number(uint64_t v) { return double(v); }

// a fixed text field; never reads past the array even if the terminator were missing
template <size_t N>
QString text(const char (&field)[N]) { return QString::fromUtf8(field, int(qstrnlen(field, N))); }

QJsonObject systemJson(const dn_system_info &s)
{
    return {{"windows", QStringLiteral("%1.%2.%3").arg(s.os_major).arg(s.os_minor).arg(s.os_build)},
            {"logicalProcessors", int(s.logical_processors)},
            {"physicalCores", int(s.physical_cores)},
            {"processorPackages", int(s.processor_packages)},
            {"totalPhysicalBytes", number(s.total_physical_bytes)},
            {"availablePhysicalBytes", number(s.available_physical_bytes)},
            {"memoryLoadPercent", int(s.memory_load_percent)}};
}

QJsonObject memoryJson(const dn_memory_info &m)
{
    return {{"handles", int(m.handle_count)},
            {"workingSetBytes", number(m.working_set_bytes)},
            {"peakWorkingSetBytes", number(m.peak_working_set_bytes)},
            {"privateBytes", number(m.private_bytes)},
            {"peakCommitBytes", number(m.peak_commit_bytes)},
            {"pageFaults", number(m.page_faults)},
            {"committedPrivateBytes", number(m.committed_private_bytes)},
            {"committedMappedBytes", number(m.committed_mapped_bytes)},
            {"committedImageBytes", number(m.committed_image_bytes)},
            {"reservedBytes", number(m.reserved_bytes)},
            {"freeBytes", number(m.free_bytes)},
            {"largestFreeBlockBytes", number(m.largest_free_block_bytes)},
            {"freeBlockCount", number(m.free_block_count)},
            {"nativeHeapLiveBytes", number(m.native_live_bytes)},
            {"nativeHeapPeakBytes", number(m.native_peak_bytes)},
            {"nativeAllocations", number(m.native_allocations)},
            {"nativeFrees", number(m.native_frees)}};
}

QJsonObject deviceJson(const dn_device &d)
{
    QJsonObject o{{"name", text(d.name)}, {"id", text(d.id)}, {"detail", text(d.detail)},
                  {"started", bool(d.flags & DN_FLAG_STARTED)}};
    if (d.flags & DN_FLAG_PROBLEM)
        o["problemCode"] = int(d.problem);
    if (d.flags & DN_FLAG_REMOVABLE)
        o["removable"] = true;
    if (d.flags & DN_FLAG_REMOTE)
        o["remote"] = true;
    if (d.flags & DN_FLAG_HAS_SIZE) {
        o["totalBytes"] = number(d.total_bytes);
        o["freeBytes"] = number(d.free_bytes);
    }
    if (d.vendor_id || d.product_id) {
        o["vendorId"] = QStringLiteral("%1").arg(d.vendor_id, 4, 16, QLatin1Char('0'));
        o["productId"] = QStringLiteral("%1").arg(d.product_id, 4, 16, QLatin1Char('0'));
    }
    return o;
}
} // namespace

QJsonObject NativeBridge::report() const
{
    QJsonObject root{{"available", isAvailable()}};
    if (!isAvailable()) {
        root["error"] = m_error;
        return root;
    }
    root["abiVersion"] = int(DN_ABI_VERSION);

    // memory first, so that it does not include the device lists built below
    dn_memory_info m{};
    m.struct_size = sizeof(m);
    const int memoryStatus = memoryInfo(&m);
    root["processMemory"] = memoryStatus == DN_OK ? memoryJson(m) : QJsonObject{{"status", memoryStatus}};

    dn_system_info s{};
    s.struct_size = sizeof(s);
    const int systemStatus = systemInfo(&s);
    root["system"] = systemStatus == DN_OK ? systemJson(s) : QJsonObject{{"status", systemStatus}};

    static const std::pair<uint32_t, const char *> kinds[] = {
        {DN_DEVICE_STORAGE, "storage"}, {DN_DEVICE_USB, "usb"}, {DN_DEVICE_HID, "hid"},
        {DN_DEVICE_SERIAL, "serial"}, {DN_DEVICE_BLUETOOTH, "bluetooth"}, {DN_DEVICE_NETWORK, "network"},
        {DN_DEVICE_DISPLAY, "display"}, {DN_DEVICE_SENSOR, "sensor"}};
    QJsonObject byKind;
    for (const auto &[kind, name] : kinds) {
        std::vector<dn_device> found;
        if (const int status = devices(kind, &found); status != DN_OK) {
            byKind[QLatin1String(name)] = QJsonObject{{"status", status}};
            continue;
        }
        QJsonArray list;
        for (const dn_device &d : found)
            list.append(deviceJson(d));
        byKind[QLatin1String(name)] = list;
    }
    root["devices"] = byKind;
    return root;
}

DeviceWatch::DeviceWatch(const NativeBridge &bridge)
    : m_bridge(bridge)
{
    if (m_bridge.watchOpen(&m_handle) != DN_OK)
        m_handle = 0;
}

DeviceWatch::~DeviceWatch()
{
    if (m_handle)
        m_bridge.watchClose(m_handle);
}

bool DeviceWatch::read(uint64_t *arrivals, uint64_t *removals) const
{
    dn_watch_counts c{};
    c.struct_size = sizeof(c);
    if (!m_handle || m_bridge.watchRead(m_handle, &c) != DN_OK)
        return false;
    if (arrivals)
        *arrivals = c.arrivals;
    if (removals)
        *removals = c.removals;
    return true;
}
