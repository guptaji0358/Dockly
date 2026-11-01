#pragma once

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>
#include <QTimer>

// Knows which of the user's programs have a window open, so the UI can mark them and a click can bring the
// existing window forward instead of starting a second copy.
// Matching is by executable: a program item's target (a .exe, or a .lnk resolved to its .exe) is compared with
// the image path of every ordinary top-level window. Polled every couple of seconds, only while enabled.
class RunningApps : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(QStringList ids READ ids NOTIFY changed)
public:
    explicit RunningApps(QObject *parent = nullptr);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool on);
    QStringList ids() const { return m_running.values(); }
    bool isRunning(const QString &id) const { return m_enabled && m_running.contains(id); }

    // The programs to look for: item id -> target path (any non-program entries are ignored).
    void setTargets(const QHash<QString, QString> &targets);
    // Brings the item's window to the front (cycling when it has several). False when it has no window.
    bool focus(const QString &id);
    void refresh(); // scan now
    void setPollInterval(int ms) { m_timer.setInterval(ms); }

signals:
    void changed();
    void enabledChanged();

private:
    QString exeKey(const QString &target);

    bool m_enabled = false;
    QTimer m_timer;
    QHash<QString, QString> m_keys;     // item id -> lower-case exe path
    QHash<QString, QString> m_resolved; // target -> lower-case exe path (.lnk resolved once)
    QSet<QString> m_running;
    QHash<QString, QList<quintptr>> m_windows; // item id -> its windows, top-most first
};
