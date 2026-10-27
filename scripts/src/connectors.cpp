#include "connectors.h"

#include "appcore.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QUrl>


namespace {
constexpr int kDefaultPort = 47821;
constexpr int kMaxRequest = 256 * 1024;

QVariantMap entry(const QString &name, const QString &target, const QString &sub = {})
{
    return {{"name", name}, {"target", target}, {"sub", sub}};
}

bool isHttpUrl(const QString &s)
{
    const QUrl u(s, QUrl::StrictMode);
    const QString scheme = u.scheme().toLower();
    return u.isValid() && !u.host().isEmpty() && (scheme == "http" || scheme == "https");
}

QString bookmarksFile(const QString &browser)
{
    const QString local = qEnvironmentVariable("LOCALAPPDATA");
    if (local.isEmpty())
        return {};
    QString rel;
    if (browser == "chrome")
        rel = "/Google/Chrome/User Data/Default/Bookmarks";
    else if (browser == "edge")
        rel = "/Microsoft/Edge/User Data/Default/Bookmarks";
    else if (browser == "brave")
        rel = "/BraveSoftware/Brave-Browser/User Data/Default/Bookmarks";
    else
        return {};
    return local + rel;
}

void walkBookmarks(const QJsonObject &node, const QString &folder, QVariantList &out)
{
    if (node.value("type").toString() == "url") {
        const QString url = node.value("url").toString();
        if (isHttpUrl(url))
            out.append(entry(node.value("name").toString(), url, folder));
        return;
    }
    const QString name = node.value("name").toString();
    const QString here = folder.isEmpty() ? name : folder + " / " + name;
    for (const QJsonValue &c : node.value("children").toArray())
        walkBookmarks(c.toObject(), here, out);
}

// "file:///c%3A/Users/me/proj" -> "C:/Users/me/proj"
QString localPathFromUri(const QString &uri)
{
    const QUrl u(uri);
    if (u.scheme() != "file")
        return {};
    return u.toLocalFile();
}
} // namespace

Connectors::Connectors(AppCore *core)
    : QObject(core), m_core(core)
{
    connect(&m_server, &QTcpServer::newConnection, this, &Connectors::onConnection);
}

// ---------------------------------------------------------------------------
// Bridge server
// ---------------------------------------------------------------------------

QString Connectors::tokenPath() const { return m_core->dataFolder() + "/bridge.token"; }

void Connectors::loadToken()
{
    QFile f(tokenPath());
    if (f.open(QIODevice::ReadOnly))
        m_token = QString::fromUtf8(f.readAll()).trimmed();
    if (m_token.size() >= 24)
        return;
    regenerateToken();
}

void Connectors::regenerateToken()
{
    QByteArray raw(16, 0);
    QRandomGenerator::securelySeeded().fillRange(reinterpret_cast<quint32 *>(raw.data()), raw.size() / 4);
    m_token = QString::fromLatin1(raw.toHex());
    QFile f(tokenPath());
    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        f.write(m_token.toUtf8());
    emit bridgeChanged();
}

QString Connectors::pairingCode() const
{
    return m_token.isEmpty() ? QString() : QString::number(bridgePort() ? bridgePort() : m_core->settings()->integer("bridgePort")) + "." + m_token;
}

void Connectors::start()
{
    loadToken();
    if (bridgeEnabled())
        listen();
}

bool Connectors::bridgeEnabled() const { return m_core->settings()->boolean("bridgeEnabled"); }

void Connectors::setBridgeEnabled(bool on)
{
    m_core->settings()->set("bridgeEnabled", on);
    if (on) {
        if (m_token.isEmpty())
            loadToken();
        listen();
    } else {
        m_server.close();
    }
    emit bridgeChanged();
}

bool Connectors::listen()
{
    if (m_server.isListening())
        return true;
    const int wanted = m_core->settings()->integer("bridgePort");
    // loopback only: nothing outside this PC can reach it
    if (!m_server.listen(QHostAddress::LocalHost, quint16(wanted > 0 ? wanted : kDefaultPort)))
        m_server.listen(QHostAddress::LocalHost, 0); // port taken: let the OS pick one
    emit bridgeChanged();
    return m_server.isListening();
}

void Connectors::onConnection()
{
    while (QTcpSocket *sock = m_server.nextPendingConnection()) {
        m_buffers.insert(sock, {});
        connect(sock, &QTcpSocket::readyRead, this, [this, sock] { onReadyRead(sock); });
        connect(sock, &QTcpSocket::disconnected, this, [this, sock] {
            m_buffers.remove(sock);
            sock->deleteLater();
        });
    }
}

void Connectors::onReadyRead(QTcpSocket *sock)
{
    QByteArray &buf = m_buffers[sock];
    buf += sock->readAll();
    if (buf.size() > kMaxRequest) {
        respond(sock, 413, R"({"ok":false,"error":"too large"})");
        return;
    }
    const int headEnd = buf.indexOf("\r\n\r\n");
    if (headEnd < 0)
        return;

    const QList<QByteArray> lines = buf.left(headEnd).split('\n');
    const QList<QByteArray> reqLine = lines.value(0).trimmed().split(' ');
    QHash<QByteArray, QByteArray> headers;
    for (int i = 1; i < lines.size(); ++i) {
        const int c = lines[i].indexOf(':');
        if (c > 0)
            headers.insert(lines[i].left(c).trimmed().toLower(), lines[i].mid(c + 1).trimmed());
    }
    const int length = headers.value("content-length").toInt();
    if (buf.size() < headEnd + 4 + length)
        return; // body still arriving

    const QByteArray body = buf.mid(headEnd + 4, length);
    buf.clear();
    handle(sock, reqLine.value(0), reqLine.value(1), headers, body);
}

void Connectors::respond(QTcpSocket *sock, int status, const QByteArray &json, const QByteArray &origin)
{
    static const QHash<int, QByteArray> reasons = {{200, "OK"}, {204, "No Content"}, {400, "Bad Request"},
                                                   {401, "Unauthorized"}, {403, "Forbidden"}, {404, "Not Found"},
                                                   {413, "Payload Too Large"}};
    QByteArray out = "HTTP/1.1 " + QByteArray::number(status) + " " + reasons.value(status, "OK") + "\r\n";
    out += "Content-Type: application/json\r\nConnection: close\r\nCache-Control: no-store\r\n";
    // only browser extensions may read answers; ordinary web pages never get CORS access
    if (origin.startsWith("chrome-extension://") || origin.startsWith("moz-extension://")) {
        out += "Access-Control-Allow-Origin: " + origin + "\r\n";
        out += "Access-Control-Allow-Headers: Authorization, Content-Type\r\n";
        out += "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n";
        out += "Access-Control-Allow-Private-Network: true\r\nVary: Origin\r\n";
    }
    out += "Content-Length: " + QByteArray::number(json.size()) + "\r\n\r\n" + json;
    sock->write(out);
    sock->disconnectFromHost();
}

void Connectors::handle(QTcpSocket *sock, const QByteArray &method, const QByteArray &path,
                        const QHash<QByteArray, QByteArray> &headers, const QByteArray &body)
{
    const QByteArray origin = headers.value("origin");

    // DNS-rebinding guard: the Host header must name the loopback address.
    const QByteArray host = headers.value("host").toLower();
    if (!(host.startsWith("127.0.0.1") || host.startsWith("localhost"))) {
        respond(sock, 403, R"({"ok":false,"error":"bad host"})");
        return;
    }
    if (method == "OPTIONS") {
        respond(sock, 204, {}, origin);
        return;
    }
    if (headers.value("authorization") != QByteArray("Bearer ") + m_token.toUtf8()) {
        respond(sock, 401, R"({"ok":false,"error":"not paired - paste the pairing code from Dockly"})", origin);
        return;
    }

    if (method == "GET" && path == "/ping") {
        QJsonArray cols;
        for (const QVariant &v : m_core->collectionsList()) {
            const QVariantMap m = v.toMap();
            cols.append(QJsonObject{{"id", m["id"].toString()}, {"name", m["name"].toString()}});
        }
        respond(sock, 200,
                QJsonDocument(QJsonObject{{"ok", true}, {"app", "Dockly"}, {"version", m_core->version()},
                                          {"collections", cols}}).toJson(QJsonDocument::Compact), origin);
        return;
    }
    if (method == "POST" && path == "/add") {
        const QJsonObject o = QJsonDocument::fromJson(body).object();
        int added = 0;
        if (o.contains("items")) {
            for (const QJsonValue &v : o.value("items").toArray())
                added += addFromJson(v.toObject().toVariantMap());
        } else {
            added = addFromJson(o.toVariantMap());
        }
        if (added == 0) {
            respond(sock, 400, R"({"ok":false,"error":"only http(s) links can be added"})", origin);
            return;
        }
        m_received += added;
        emit receivedChanged();
        m_core->message(added == 1 ? QStringLiteral("Added a shortcut from your browser.")
                                   : QStringLiteral("Added %1 shortcuts from your browser.").arg(added));
        respond(sock, 200, QJsonDocument(QJsonObject{{"ok", true}, {"added", added}}).toJson(QJsonDocument::Compact), origin);
        return;
    }
    respond(sock, 404, R"({"ok":false,"error":"not found"})", origin);
}

int Connectors::addFromJson(const QVariantMap &o)
{
    const QString url = o.value("url").toString().trimmed();
    if (!isHttpUrl(url))
        return 0;
    const QString title = o.value("title").toString().trimmed().left(120);
    const QString id = m_core->addItem(LH::Web, title, url);
    if (id.isEmpty())
        return 0;
    const QString col = o.value("collection").toString();
    if (!col.isEmpty() && m_core->exists(col))
        m_core->addToCollection(col, {id});

    // optional custom icon, sent by the extension as a small PNG data URL
    const QString icon = o.value("icon").toString();
    static const QString prefix = QStringLiteral("data:image/png;base64,");
    if (icon.startsWith(prefix)) {
        const QByteArray png = QByteArray::fromBase64(icon.mid(prefix.size()).toLatin1());
        const QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                            + QStringLiteral("/dockly_ext_icon.png");
        QFile f(tmp);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(png);
            f.close();
            m_core->setItemIcon(id, tmp);
            QFile::remove(tmp);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// Extension helpers
// ---------------------------------------------------------------------------

void Connectors::copyPairingCode()
{
    if (QClipboard *cb = QGuiApplication::clipboard())
        cb->setText(pairingCode());
    m_core->message(QStringLiteral("Pairing code copied."));
}

QString Connectors::extensionFolder() const
{
    const QString app = QCoreApplication::applicationDirPath();
    for (const QString &rel : {"/resources/connectors/browser", "/connectors/browser", "/../connectors/browser", "/../../connectors/browser"}) {
        const QString p = QDir::cleanPath(app + rel);
        if (QFile::exists(p + "/manifest.json"))
            return QDir::toNativeSeparators(p);
    }
    return {};
}

void Connectors::openExtensionFolder()
{
    const QString p = extensionFolder();
    if (p.isEmpty())
        m_core->message(QStringLiteral("The browser extension folder wasn't found next to Dockly."));
    else
        QDesktopServices::openUrl(QUrl::fromLocalFile(p));
}

// ---------------------------------------------------------------------------
// Importers
// ---------------------------------------------------------------------------

QStringList Connectors::installedBrowsers() const
{
    QStringList out;
    for (const QString &b : {"chrome", "edge", "brave"})
        if (QFile::exists(bookmarksFile(b)))
            out << b;
    return out;
}

QVariantList Connectors::browserBookmarks(const QString &browser) const
{
    return bookmarksFromFile(bookmarksFile(browser));
}

QVariantList Connectors::bookmarksFromFile(const QString &path) const
{
    QVariantList out;
    if (path.isEmpty())
        return out;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return out;
    const QJsonObject roots = QJsonDocument::fromJson(f.readAll()).object().value("roots").toObject();
    for (const QString &key : {"bookmark_bar", "other", "synced"})
        if (roots.contains(key))
            walkBookmarks(roots.value(key).toObject(), {}, out);
    return out;
}

QVariantList Connectors::vscodeRecent() const
{
    QVariantList out;
    const QString db = qEnvironmentVariable("APPDATA") + "/Code/User/globalStorage/state.vscdb";
    if (!QFile::exists(db))
        return out;
    // work on a copy: VS Code may be running and holding the file
    const QString tmp = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/dockly_vscode.vscdb";
    QFile::remove(tmp);
    if (!QFile::copy(db, tmp))
        return out;

    QString json;
    {
        QSqlDatabase conn = QSqlDatabase::addDatabase("QSQLITE", "dockly_vscode");
        conn.setDatabaseName(tmp);
        conn.setConnectOptions("QSQLITE_OPEN_READONLY");
        if (conn.open()) {
            QSqlQuery q(conn);
            q.exec("SELECT value FROM ItemTable WHERE key='history.recentlyOpenedPathsList'");
            if (q.next())
                json = q.value(0).toString();
        }
    }
    QSqlDatabase::removeDatabase("dockly_vscode");
    QFile::remove(tmp);

    for (const QJsonValue &v : QJsonDocument::fromJson(json.toUtf8()).object().value("entries").toArray()) {
        const QJsonObject e = v.toObject();
        QString path = localPathFromUri(e.value("folderUri").toString());
        if (path.isEmpty())
            path = localPathFromUri(e.value("workspace").toObject().value("configPath").toString());
        if (path.isEmpty())
            continue;
        const QFileInfo fi(path);
        const QString name = path.endsWith(".code-workspace") ? fi.completeBaseName() : (fi.fileName().isEmpty() ? path : fi.fileName());
        // vscode://file/<path> opens the folder / workspace in VS Code itself
        out.append(entry(name, "vscode://file/" + path, QDir::toNativeSeparators(path)));
    }
    return out;
}

void Connectors::fetchGithub(const QString &userIn)
{
    const QString user = userIn.trimmed();
    static const QRegularExpression valid("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,38})$");
    if (!valid.match(user).hasMatch()) {
        emit githubReady({}, QStringLiteral("That doesn't look like a GitHub username."));
        return;
    }
    if (!m_net)
        m_net = new QNetworkAccessManager(this);
    m_busy = true;
    emit busyChanged();

    QNetworkRequest req(QUrl(QStringLiteral("https://api.github.com/users/%1/repos?per_page=100&sort=updated").arg(user)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Dockly/%1").arg(m_core->version()));
    req.setRawHeader("Accept", "application/vnd.github+json");
    req.setTransferTimeout(15000);
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, user] {
        reply->deleteLater();
        m_busy = false;
        emit busyChanged();
        const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (code == 404) {
            emit githubReady({}, QStringLiteral("No GitHub user called \"%1\".").arg(user));
            return;
        }
        if (reply->error() != QNetworkReply::NoError) {
            emit githubReady({}, code == 403 ? QStringLiteral("GitHub rate limit reached - try again later.")
                                             : QStringLiteral("Couldn't reach GitHub."));
            return;
        }
        QVariantList out;
        for (const QJsonValue &v : QJsonDocument::fromJson(reply->readAll()).array()) {
            const QJsonObject r = v.toObject();
            const QString url = r.value("html_url").toString();
            if (!isHttpUrl(url))
                continue;
            const QString desc = r.value("description").toString();
            out.append(entry(r.value("name").toString(), url, desc.isEmpty() ? user + " / " + r.value("name").toString() : desc));
        }
        emit githubReady(out, out.isEmpty() ? QStringLiteral("\"%1\" has no public repositories.").arg(user) : QString());
    });
}

int Connectors::addEntries(const QVariantList &entries)
{
    QSet<QString> ids;
    for (const QVariant &v : entries) {
        const QVariantMap m = v.toMap();
        const QString id = m_core->addItem(LH::Web, m.value("name").toString(), m.value("target").toString());
        if (!id.isEmpty())
            ids.insert(id);
    }
    if (!ids.isEmpty())
        m_core->message(QStringLiteral("Added %1 shortcut(s).").arg(ids.size()));
    return ids.size();
}

// http(s)://[www.]github.com/<something>; Database::queryIds uses the same rule in SQL
static bool isGithubUrl(const QString &target)
{
    const QUrl u(target);
    const QString host = u.host().toLower();
    return (u.scheme() == QLatin1String("https") || u.scheme() == QLatin1String("http"))
        && (host == QLatin1String("github.com") || host == QLatin1String("www.github.com"))
        && u.path().startsWith('/');
}

QVariantList Connectors::githubGroups() const
{
    QVariantList out;
    for (const QVariant &cv : m_core->collectionsList()) {
        const QVariantMap c = cv.toMap();
        const QString cid = c.value("id").toString();
        const QStringList ids = m_core->queryIds(LH::ModeMembers, LH::AllCategories, QString(), cid);
        QVariantList repos;
        bool allGithub = !ids.isEmpty();
        for (const QString &id : ids) {
            const Item it = m_core->item(id);
            if (!isGithubUrl(it.target)) {
                allGithub = false;
                break;
            }
            QString path = QUrl(it.target).path();
            if (path.startsWith('/'))
                path.remove(0, 1);
            repos.append(QVariantMap{{"id", id}, {"name", it.name}, {"path", path}});
        }
        if (allGithub)
            out.append(QVariantMap{{"id", cid}, {"name", c.value("name")}, {"repos", repos}});
    }
    return out;
}

// owner -> ids of the loose github.com/<owner>/<repo> shortcuts
static QMap<QString, QStringList> looseGithubByOwner(const AppCore *core)
{
    QMap<QString, QStringList> byOwner;
    for (const QString &id : core->queryIds(LH::ModeCategory, LH::Web, QString(), QString())) {
        if (!isGithubUrl(core->item(id).target))
            continue;
        const QUrl u(core->item(id).target);
        const QStringList seg = u.path().split('/', Qt::SkipEmptyParts);
        if (seg.size() >= 2)
            byOwner[seg.first()] << id;
    }
    return byOwner;
}

int Connectors::looseGithubCount() const
{
    int n = 0;
    for (const QStringList &ids : looseGithubByOwner(m_core))
        n += ids.size();
    return n;
}

int Connectors::organizeGithub()
{
    int moved = 0;
    const QMap<QString, QStringList> byOwner = looseGithubByOwner(m_core);
    for (auto it = byOwner.cbegin(); it != byOwner.cend(); ++it) {
        const QString name = QStringLiteral("GitHub · ") + it.key();
        QString col;
        for (const QVariant &cv : m_core->collectionsList()) {
            const QVariantMap c = cv.toMap();
            if (c.value("name").toString().compare(name, Qt::CaseInsensitive) == 0) {
                col = c.value("id").toString();
                break;
            }
        }
        if (col.isEmpty())
            col = m_core->createCollection(name);
        if (col.isEmpty())
            continue;
        m_core->addToCollection(col, it.value());
        moved += it.value().size();
    }
    if (moved)
        m_core->message(QStringLiteral("Organized %1 repo(s) by owner.").arg(moved));
    return moved;
}

int Connectors::addEntriesToNewCollection(const QVariantList &entries, const QString &name)
{
    QStringList ids;
    for (const QVariant &v : entries) {
        const QVariantMap m = v.toMap();
        const QString id = m_core->addItem(LH::Web, m.value("name").toString(), m.value("target").toString());
        if (!id.isEmpty())
            ids << id;
    }
    if (ids.isEmpty())
        return 0;
    // a category with this name already exists: add to it instead of making a twin
    const QString cname = name.trimmed();
    QString col;
    for (const QVariant &cv : m_core->collectionsList()) {
        const QVariantMap c = cv.toMap();
        if (c.value("name").toString().compare(cname, Qt::CaseInsensitive) == 0) {
            col = c.value("id").toString();
            break;
        }
    }
    if (col.isEmpty())
        col = m_core->createCollection(cname);
    if (!col.isEmpty())
        m_core->addToCollection(col, ids);
    m_core->message(QStringLiteral("Added %1 repo(s) to \"%2\".").arg(ids.size()).arg(cname));
    return ids.size();
}
