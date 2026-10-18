#pragma once

#include <QHash>
#include <QObject>
#include <QTcpServer>
#include <QVariantList>

class AppCore;
class QNetworkAccessManager;
class QTcpSocket;

// Connectors link Dockly with other applications:
//  - Browser bridge: a loopback-only HTTP endpoint the Chrome / Edge extension posts to, so a page
//    or link can be added as a web shortcut straight from the browser.
//  - Importers: browser bookmarks, VS Code recent projects, GitHub repositories.
// Every importer returns plain {name, target, sub} entries; the UI lets the user pick which become cards.
class Connectors : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool bridgeEnabled READ bridgeEnabled WRITE setBridgeEnabled NOTIFY bridgeChanged)
    Q_PROPERTY(bool bridgeRunning READ bridgeRunning NOTIFY bridgeChanged)
    Q_PROPERTY(int bridgePort READ bridgePort NOTIFY bridgeChanged)
    Q_PROPERTY(QString pairingCode READ pairingCode NOTIFY bridgeChanged)
    Q_PROPERTY(int received READ received NOTIFY receivedChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
    explicit Connectors(AppCore *core);

    void start(); // after settings are loaded

    bool bridgeEnabled() const;
    void setBridgeEnabled(bool on);
    bool bridgeRunning() const { return m_server.isListening(); }
    int bridgePort() const { return m_server.isListening() ? m_server.serverPort() : 0; }
    QString pairingCode() const;
    int received() const { return m_received; }
    bool busy() const { return m_busy; }

    Q_INVOKABLE void regenerateToken();
    Q_INVOKABLE void copyPairingCode();
    Q_INVOKABLE QString extensionFolder() const;
    Q_INVOKABLE void openExtensionFolder();

    Q_INVOKABLE QStringList installedBrowsers() const;                 // "chrome", "edge", "brave"
    Q_INVOKABLE QVariantList browserBookmarks(const QString &browser) const;
    Q_INVOKABLE QVariantList bookmarksFromFile(const QString &path) const; // Chrome-format Bookmarks file
    Q_INVOKABLE QVariantList vscodeRecent() const;
    Q_INVOKABLE void fetchGithub(const QString &user);                 // async -> githubReady
    Q_INVOKABLE int addEntries(const QVariantList &entries);           // [{name, target}] as web shortcuts

signals:
    void bridgeChanged();
    void receivedChanged();
    void busyChanged();
    void githubReady(const QVariantList &entries, const QString &error);

private:
    QString tokenPath() const;
    void loadToken();
    bool listen();
    void onConnection();
    void onReadyRead(QTcpSocket *sock);
    void respond(QTcpSocket *sock, int status, const QByteArray &json, const QByteArray &origin = {});
    void handle(QTcpSocket *sock, const QByteArray &method, const QByteArray &path,
                const QHash<QByteArray, QByteArray> &headers, const QByteArray &body);
    int addFromJson(const QVariantMap &o);

    AppCore *m_core;
    QTcpServer m_server;
    QHash<QTcpSocket *, QByteArray> m_buffers;
    QNetworkAccessManager *m_net = nullptr;
    QString m_token;
    int m_received = 0;
    bool m_busy = false;
};
