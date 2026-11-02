#pragma once

#include <QString>
#include <QVector>

struct ScannedApp {
    QString name;
    QString target; // resolved .exe path
};

// Scans the Start Menu shortcuts (all users + current user), then the registry
// (Uninstall entries and App Paths), for installed programs.
// Safe to call from a worker thread.
QVector<ScannedApp> scanInstalledSoftware();

// Human-readable name of an executable (version-info FileDescription/ProductName),
// or an empty string when it has none.
QString executableDisplayName(const QString &exePath);
