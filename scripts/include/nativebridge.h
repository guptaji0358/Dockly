#pragma once

#include <QLibrary>
#include <QString>

#include "dockly_native.h"

// The only C++ code that talks to the Rust DLL (native/). The DLL is loaded at run time, so a
// missing or mismatched one switches the native features off instead of stopping Dockly.
class NativeBridge
{
public:
    // Loads the DLL from the given full path; empty = beside the module this code is linked into.
    explicit NativeBridge(const QString &path = QString());

    NativeBridge(const NativeBridge &) = delete;
    NativeBridge &operator=(const NativeBridge &) = delete;

    bool isAvailable() const { return m_read != nullptr; }
    QString error() const { return m_error; } // why it is unavailable

    // A dn status; DN_ERR_OS when the library is unavailable. *out is changed only on DN_OK.
    int systemInfo(dn_system_info *out) const;

    // One line for the log.
    QString summary() const;

    static QString defaultPath();

private:
    QLibrary m_lib;
    decltype(&dn_system_info_read) m_read = nullptr;
    QString m_error;
};
