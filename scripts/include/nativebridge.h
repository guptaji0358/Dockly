#pragma once

#include <vector>

#include <QJsonObject>
#include <QLibrary>
#include <QString>

#include "dockly_native.h"

// The only C++ code that talks to the Rust DLL (native/). The DLL is loaded at run time, so a
// missing or mismatched one switches the native features off instead of stopping Dockly.
// All methods are const and safe to call from any thread.
class NativeBridge
{
public:
    // Loads the DLL from the given full path; empty = beside the module this code is linked into.
    explicit NativeBridge(const QString &path = QString());

    NativeBridge(const NativeBridge &) = delete;
    NativeBridge &operator=(const NativeBridge &) = delete;

    bool isAvailable() const { return m_available; }
    QString error() const { return m_error; } // why it is unavailable

    // Each returns a dn status; DN_ERR_OS when the library is unavailable.
    // *out is changed only on DN_OK.
    int systemInfo(dn_system_info *out) const;
    int memoryInfo(dn_memory_info *out) const;
    // Replaces *out with the present devices of one DN_DEVICE_* kind.
    int devices(uint32_t kind, std::vector<dn_device> *out) const;

    // The raw array call, as declared in the header.
    int deviceList(uint32_t kind, dn_device *items, uint32_t capacity, uint32_t itemSize, uint32_t *count) const;

    // Prefer DeviceWatch, which closes the handle itself.
    int watchOpen(uint64_t *handle) const;
    int watchRead(uint64_t handle, dn_watch_counts *out) const;
    int watchClose(uint64_t handle) const;

    // One line for the log.
    QString summary() const;

    // Everything the library can report, for diagnostics. Device ids can hold serial numbers.
    QJsonObject report() const;

    static QString defaultPath();

private:
    struct Api {
        decltype(&dn_system_info_read) systemInfo = nullptr;
        decltype(&dn_memory_info_read) memoryInfo = nullptr;
        decltype(&dn_device_list) deviceList = nullptr;
        decltype(&dn_watch_open) watchOpen = nullptr;
        decltype(&dn_watch_read) watchRead = nullptr;
        decltype(&dn_watch_close) watchClose = nullptr;
    };

    QLibrary m_lib;
    Api m_api;
    bool m_available = false;
    QString m_error;
};

// Owns one device watch and closes it on destruction. The bridge must outlive it.
class DeviceWatch
{
public:
    explicit DeviceWatch(const NativeBridge &bridge);
    ~DeviceWatch();

    DeviceWatch(const DeviceWatch &) = delete;
    DeviceWatch &operator=(const DeviceWatch &) = delete;

    bool isOpen() const { return m_handle != 0; }
    // Totals since construction. False when the watch is not open or the call failed.
    bool read(uint64_t *arrivals, uint64_t *removals) const;

private:
    const NativeBridge &m_bridge;
    uint64_t m_handle = 0;
};
