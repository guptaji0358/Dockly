#include "guardstartup.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shlobj.h>
#endif

namespace Guard {

QString exeOfValue(const QString &v)
{
    const QString s = v.trimmed();
    if (s.isEmpty() || s.contains(QLatin1Char('%'))) return {}; // unexpanded variables: not reliable
    QString exe;
    if (s.startsWith(QLatin1Char('"'))) {
        const int end = s.indexOf(QLatin1Char('"'), 1);
        if (end < 0) return {};
        exe = s.mid(1, end - 1);
    } else {
        const int i = s.indexOf(QStringLiteral(".exe"), 0, Qt::CaseInsensitive);
        if (i < 0) return {};
        exe = s.left(i + 4); // everything after .exe (the arguments) is discarded here, never kept
    }
    return exe.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive) ? QDir::toNativeSeparators(exe) : QString();
}

#ifdef Q_OS_WIN
namespace {
// Qt resolves .lnk files like symlinks, so QFile::exists() is false for a shortcut whose target is gone.
bool rawExists(const QString &p) { return GetFileAttributesW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(p).utf16())) != INVALID_FILE_ATTRIBUTES; }
const wchar_t *kDefaultRun = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";

class SysBackend : public StartupBackend
{
public:
    SysBackend(const QString &sub, const QString &folder)
        : m_sub(sub.isEmpty() ? QString::fromWCharArray(kDefaultRun) : sub)
    {
        if (folder.isEmpty()) {
            PWSTR p = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &p))) { m_folder = QString::fromWCharArray(p); CoTaskMemFree(p); }
        } else {
            m_folder = folder;
        }
    }
    QVector<StartupEntry> list() override
    {
        QVector<StartupEntry> out;
        HKEY k;
        if (RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(m_sub.utf16()), 0, KEY_READ, &k) == ERROR_SUCCESS) {
            wchar_t nm[16384];
            for (DWORD i = 0;; ++i) {
                DWORD n = DWORD(std::size(nm));
                if (RegEnumValueW(k, i, nm, &n, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
                StartupEntry e;
                if (read(StartupSource::RunKeyHkcu, QString::fromWCharArray(nm, int(n)), &e) && e.present) out << e;
            }
            RegCloseKey(k);
        }
        const QDir d(m_folder);
        for (const QFileInfo &fi : d.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
            if (fi.suffix().compare(QStringLiteral("lnk"), Qt::CaseInsensitive) == 0) {
                StartupEntry e;
                if (read(StartupSource::StartupFolder, fi.fileName(), &e) && e.present) out << e;
            }
        return out;
    }
    bool read(StartupSource s, const QString &name, StartupEntry *out) override
    {
        *out = StartupEntry();
        out->source = s; out->name = name; out->present = false;
        if (s == StartupSource::RunKeyHkcu) {
            out->location = QStringLiteral("HKCU\\") + m_sub;
            HKEY k;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(m_sub.utf16()), 0, KEY_QUERY_VALUE, &k) != ERROR_SUCCESS)
                return true; // key absent => entry absent
            DWORD type = 0, size = 0;
            const LONG r = RegQueryValueExW(k, reinterpret_cast<LPCWSTR>(name.utf16()), nullptr, &type, nullptr, &size);
            if (r == ERROR_SUCCESS && (type == REG_SZ || type == REG_EXPAND_SZ)) {
                QByteArray buf(int(size), 0);
                if (RegQueryValueExW(k, reinterpret_cast<LPCWSTR>(name.utf16()), nullptr, &type,
                                     reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS) {
                    out->value = QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buf.constData()), int(size / 2)).section(QChar(0), 0, 0);
                    out->meta = QStringLiteral("type=%1").arg(type);
                    out->exeHint = exeOfValue(out->value);
                    out->present = true;
                }
            }
            RegCloseKey(k);
            return true;
        }
        if (s == StartupSource::StartupFolder) {
            out->location = QDir::toNativeSeparators(m_folder);
            if (name.contains(QLatin1Char('/')) || name.contains(QLatin1Char('\\'))) return false;
            QFile f(m_folder + QLatin1Char('/') + name);
            if (!rawExists(m_folder + QLatin1Char('/') + name)) return true;
            if (!f.open(QIODevice::ReadOnly) || f.size() > 256 * 1024) return false;
            out->value = QString::fromLatin1(f.readAll().toBase64());
            out->meta = QStringLiteral("mtime=%1").arg(QFileInfo(f).lastModified().toMSecsSinceEpoch());
            out->exeHint = QDir::toNativeSeparators(QFileInfo(f).symLinkTarget()); // target only; shortcut arguments are not read
            out->present = true;
            return true;
        }
        return false;
    }
    bool remove(StartupSource s, const QString &name, QString *err) override
    {
        if (s == StartupSource::RunKeyHkcu) {
            HKEY k;
            if (RegOpenKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(m_sub.utf16()), 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS) {
                if (err) *err = QStringLiteral("cannot open the Run key"); return false;
            }
            const LONG r = RegDeleteValueW(k, reinterpret_cast<LPCWSTR>(name.utf16()));
            RegCloseKey(k);
            if (r != ERROR_SUCCESS) { if (err) *err = QStringLiteral("delete failed (%1)").arg(r); return false; }
            return true;
        }
        if (s == StartupSource::StartupFolder && !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'))) {
            if (!QFile::remove(m_folder + QLatin1Char('/') + name)) { if (err) *err = QStringLiteral("could not delete the shortcut"); return false; }
            return true;
        }
        if (err) *err = QStringLiteral("unsupported source");
        return false;
    }
    bool write(StartupSource s, const QString &name, const QString &value, const QString &meta, QString *err) override
    {
        if (s == StartupSource::RunKeyHkcu) {
            HKEY k;
            if (RegCreateKeyExW(HKEY_CURRENT_USER, reinterpret_cast<LPCWSTR>(m_sub.utf16()), 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) != ERROR_SUCCESS) {
                if (err) *err = QStringLiteral("cannot open the Run key"); return false;
            }
            const DWORD type = meta.contains(QStringLiteral("type=2")) ? REG_EXPAND_SZ : REG_SZ;
            const LONG r = RegSetValueExW(k, reinterpret_cast<LPCWSTR>(name.utf16()), 0, type,
                                          reinterpret_cast<const BYTE *>(value.utf16()), DWORD((value.size() + 1) * 2));
            RegCloseKey(k);
            if (r != ERROR_SUCCESS) { if (err) *err = QStringLiteral("write failed (%1)").arg(r); return false; }
            return true;
        }
        if (s == StartupSource::StartupFolder && !name.contains(QLatin1Char('/')) && !name.contains(QLatin1Char('\\'))) {
            QFile f(m_folder + QLatin1Char('/') + name);
            if (rawExists(m_folder + QLatin1Char('/') + name)) { if (err) *err = QStringLiteral("a file already exists"); return false; }
            if (!f.open(QIODevice::WriteOnly) || f.write(QByteArray::fromBase64(value.toLatin1())) < 0) {
                if (err) *err = QStringLiteral("could not write the shortcut"); return false;
            }
            bool ok = false;
            const qint64 ms = meta.section(QLatin1Char('='), 1).toLongLong(&ok);
            if (ok) f.setFileTime(QDateTime::fromMSecsSinceEpoch(ms), QFileDevice::FileModificationTime); // must precede close
            f.close();
            return true;
        }
        if (err) *err = QStringLiteral("unsupported source");
        return false;
    }
private:
    QString m_sub, m_folder;
};
}

std::unique_ptr<StartupBackend> makeSystemBackend(const QString &sub, const QString &folder)
{
    return std::make_unique<SysBackend>(sub, folder);
}
#else
std::unique_ptr<StartupBackend> makeSystemBackend(const QString &, const QString &) { return nullptr; }
#endif

}
