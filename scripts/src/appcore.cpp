#include "appcore.h"
#include "connectors.h"
#include "iconpack.h"

#include <QApplication>
#include <QColorDialog>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QLinearGradient>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPainter>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <QtConcurrent>

#include <QClipboard>
#include <QGuiApplication>
#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {
constexpr int kBackupFormatVersion = 2;
constexpr int kCacheCap = 400;

QString newId() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }

QString normalizeUrl(const QString &raw)
{
    QString t = raw.trimmed();
    if (t.isEmpty())
        return t;
    if (!t.contains(QLatin1String("://")) && !t.startsWith(QLatin1String("mailto:")))
        t.prepend(QStringLiteral("https://"));
    return t;
}

bool isValidWebUrl(const QString &target)
{
    const QUrl u(target, QUrl::StrictMode);
    const QString scheme = u.scheme().toLower();
    return u.isValid() && !u.host().isEmpty()
           && (scheme == "http" || scheme == "https" || scheme == "ftp" || scheme == "vscode");
}

QString autoName(LH::Category c, const QString &target)
{
    if (c == LH::Web) {
        QString host = QUrl(target).host();
        if (host.startsWith(QLatin1String("www.")))
            host.remove(0, 4);
        return host.isEmpty() ? target : host;
    }
    return QFileInfo(target).completeBaseName();
}
} // namespace

AppCore *AppCore::instance()
{
    static AppCore *s = new AppCore(qApp);
    return s;
}

AppCore::AppCore(QObject *parent)
    : QObject(parent)
    , m_settings(new AppSettings(this))
    , m_theme(new Theme(m_settings, this))
    , m_connectors(new Connectors(this))
    , m_sites(new Sites(this))
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(400);
    connect(&m_saveTimer, &QTimer::timeout, this, &AppCore::saveNow);
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this] {
        saveNow();
        m_closed = true; // no more database access (late signals from watchers etc.)
        m_db.close();
    });

    m_checkTimer.setInterval(5 * 60 * 1000);
    connect(&m_checkTimer, &QTimer::timeout, this, [this] {
        if (m_settings->boolean("checkMissingOnStartup"))
            checkItems();
    });

    connect(&m_scanWatcher, &QFutureWatcher<QVector<ScannedApp>>::finished, this, [this] {
        m_scanning = false;
        emit scanningChanged();
        if (m_closed)
            return;
        const int n = m_db.replaceCatalog(m_scanWatcher.result());
        emit catalogChanged();
        emit message(QStringLiteral("Found %1 installed program(s). Pick the ones you want from \"Add Software\".").arg(n));
    });
    connect(&m_checkWatcher, &QFutureWatcher<QHash<QString, int>>::finished, this, [this] {
        m_checking = false;
        if (!m_closed && m_db.applyStatuses(m_checkWatcher.result()))
            changed();
    });

    connect(m_settings, &AppSettings::changed, this, [this](const QString &key) {
        if (key == "startWithWindows") {
            applyStartWithWindows();
        } else if (key == "maxRecent" || key == "recentEnabled") {
            m_db.trimRecents(m_settings->integer("maxRecent"));
            changed();
        } else if (key == "checkMissingOnStartup" && m_settings->boolean(key)) {
            checkItems();
        }
        m_saveTimer.start();
    });
}

QString AppCore::version() const { return QStringLiteral(LH_VERSION); }

QIcon AppCore::appIcon()
{
    QPixmap pm(256, 256);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    QLinearGradient g(0, 0, 256, 256);
    g.setColorAt(0, QColor("#2f7cf6"));
    g.setColorAt(1, QColor("#101a33"));
    p.setBrush(g);
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(8, 8, 240, 240), 56, 56);
    p.end();
    // Our own rocket mark (custom SVG), white on the gradient tile.
    QPainter p2(&pm);
    p2.drawPixmap(QRect(48, 48, 160, 160), IconPack::pixmap(QStringLiteral("logo"), 160, Qt::white));
    p2.end();
    return QIcon(pm);
}

// ---------------------------------------------------------------------------
// Startup / persistence
// ---------------------------------------------------------------------------

namespace {
// Before the rename the data lived in <AppData>/LauncherHub/LauncherHub; carry it over once.
void copyDir(const QDir &from, const QDir &to)
{
    QDir().mkpath(to.absolutePath());
    for (const QFileInfo &fi : from.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot)) {
        const QString dst = to.filePath(fi.fileName());
        if (fi.isDir())
            copyDir(QDir(fi.absoluteFilePath()), QDir(dst));
        else
            QFile::copy(fi.absoluteFilePath(), dst);
    }
}

void migrateLegacyData(const QString &dataDir)
{
    if (QFile::exists(dataDir + "/settings.json") || QFile::exists(dataDir + "/library.db"))
        return;
    const QString legacy = QDir(dataDir).absoluteFilePath("../../LauncherHub/LauncherHub");
    if (QDir(legacy).exists())
        copyDir(QDir(QDir::cleanPath(legacy)), QDir(dataDir));
}
}

QString AppCore::settingsPath() const { return m_dataDir + "/settings.json"; }

void AppCore::init()
{
    // LH_DATA_DIR lets you run against a separate profile (testing / portable use).
    m_dataDir = qEnvironmentVariable("LH_DATA_DIR");
    if (m_dataDir.isEmpty())
        m_dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (qEnvironmentVariableIsEmpty("LH_DATA_DIR"))
        migrateLegacyData(m_dataDir);
    QDir().mkpath(m_dataDir);
    QDir().mkpath(m_dataDir + "/icons");
    QDir().mkpath(m_dataDir + "/backups");

    const bool firstRun = !QFile::exists(settingsPath());
    m_settings->load(settingsPath());
    if (firstRun)
        m_settings->save(settingsPath());

    if (!m_db.open(m_dataDir)) {
        emit message(QStringLiteral("Couldn't open the databases in %1").arg(m_dataDir));
        return;
    }
    if (m_db.libraryIsNew() && QFile::exists(m_dataDir + "/data.json"))
        migrateLegacyJson();

    m_sites->load(m_dataDir);
    applyStartWithWindows();
    m_connectors->start();
    if (m_settings->boolean("checkMissingOnStartup"))
        checkItems();
    m_checkTimer.start();
    changed();

    // a workspace the user asked for opens a few seconds after start, once the desktop has settled
    const QString startup = m_settings->text("startupWorkspace");
    if (!startup.isEmpty() && exists(startup))
        QTimer::singleShot(4000, this, [this, startup] {
            if (!m_closed)
                launchAll(startup);
        });
}

// Older builds kept everything in data.json and showed every detected program as a card.
// Import the user's own cards and move the auto-detected programs into the hidden catalog.
void AppCore::migrateLegacyJson()
{
    QFile f(m_dataDir + "/data.json");
    if (!f.open(QIODevice::ReadOnly))
        return;
    const QJsonObject old = QJsonDocument::fromJson(f.readAll()).object();
    f.close();

    QJsonArray keep;
    QVector<ScannedApp> catalog;
    for (const QJsonValue &v : old["items"].toArray()) {
        const QJsonObject o = v.toObject();
        if (o["autoDetected"].toBool() && !o["favorite"].toBool()) {
            catalog.append({o["name"].toString(), o["target"].toString()});
        } else {
            keep.append(o);
        }
    }
    QJsonObject data;
    data["items"] = keep;
    data["recent"] = old["recent"];
    m_db.importAll(data);
    if (!catalog.isEmpty())
        m_db.replaceCatalog(catalog);
    QFile::rename(m_dataDir + "/data.json", m_dataDir + "/data.json.migrated");
}

void AppCore::saveNow()
{
    if (m_dataDir.isEmpty())
        return;
    m_settings->save(settingsPath());
}

void AppCore::changed()
{
    if (m_closed)
        return;
    m_cache.clear();
    emit itemsChanged();
}

// ---------------------------------------------------------------------------
// Row access
// ---------------------------------------------------------------------------

Item AppCore::item(const QString &id) const
{
    if (id.isEmpty() || m_closed)
        return {};
    if (const auto c = m_cache.constFind(id); c != m_cache.cend())
        return c.value();
    Item it;
    if (!m_db.getItem(id, it))
        return {};
    if (it.category == LH::Collection)
        it.memberCount = m_db.memberCount(id);
    if (m_cache.size() >= kCacheCap)
        m_cache.clear();
    m_cache.insert(id, it);
    return it;
}

QStringList AppCore::queryIds(int mode, int category, const QString &search, const QString &collectionId) const
{
    if (m_closed)
        return {};
    if (mode == LH::ModeCategory && category >= LH::SiteViewBase)
        return m_sites->viewIds(category - LH::SiteViewBase, search);
    const int recentCap = m_settings->boolean("recentEnabled") ? m_settings->integer("maxRecent") : 0;
    QStringList ids = m_db.queryIds(mode, category, search, collectionId, recentCap);
    if (mode == LH::ModeCategory && category == LH::Collection) // site groups live on their site's page
        ids.removeIf([this](const QString &id) { return m_sites->isSiteGroup(id); });
    return ids;
}

void AppCore::select(const QString &id)
{
    if (m_selectedId == id)
        return;
    m_selectedId = id;
    emit selectionChanged();
}

// ---------------------------------------------------------------------------
// Validation
// ---------------------------------------------------------------------------

LH::Status AppCore::computeStatus(const Item &it)
{
    switch (it.category) {
    case LH::Web:
        return isValidWebUrl(it.target) ? LH::Normal : LH::Warning;
    case LH::Collection:
        return LH::Normal;
    default:
        return QFileInfo::exists(it.target) ? LH::Normal : LH::Missing;
    }
}

void AppCore::checkItems()
{
    if (m_checking)
        return;
    m_checking = true;
    const QVector<Item> rows = m_db.checkRows();
    m_checkWatcher.setFuture(QtConcurrent::run([rows] {
        QHash<QString, int> out;
        for (const Item &it : rows)
            out.insert(it.id, int(computeStatus(it)));
        return out;
    }));
}

// ---------------------------------------------------------------------------
// Item management
// ---------------------------------------------------------------------------

QString AppCore::addItem(int category, const QString &nameIn, const QString &targetIn)
{
    const auto cat = LH::Category(category);
    QString target = cat == LH::Web ? normalizeUrl(targetIn) : QDir::toNativeSeparators(targetIn.trimmed());
    if (cat == LH::Collection)
        target.clear();
    else if (target.isEmpty())
        return {};

    if (cat != LH::Collection) {
        const QString existing = m_db.findByNorm(cat, Item::normalizedTarget(target));
        if (!existing.isEmpty())
            return existing;
    }

    Item it;
    it.id = newId();
    it.category = cat;
    it.target = target;
    it.name = nameIn.trimmed().isEmpty() ? autoName(cat, target) : nameIn.trimmed();
    if (it.name.isEmpty())
        it.name = cat == LH::Collection ? QStringLiteral("New Collection") : target;
    it.status = computeStatus(it);
    if (!m_db.insertItem(it))
        return {};
    changed();
    if (cat == LH::Web && m_autoFile)
        m_sites->autoFile(it.id); // files it into its site's group when a site matches
    return it.id;
}

QString AppCore::findWeb(const QString &url) const
{
    return m_db.findByNorm(LH::Web, Item::normalizedTarget(normalizeUrl(url)));
}

QStringList AppCore::addFiles(int category, const QStringList &paths)
{
    QStringList ids;
    for (const QString &p : paths) {
        const QString id = addItem(category, {}, p);
        if (!id.isEmpty())
            ids << id;
    }
    return ids;
}

int AppCore::addSoftware(const QVariantList &entries)
{
    int added = 0;
    for (const QVariant &e : entries) {
        const QVariantMap m = e.toMap();
        const QString target = m["target"].toString();
        if (target.isEmpty())
            continue;
        const bool had = !m_db.findByNorm(LH::Software, Item::normalizedTarget(QDir::toNativeSeparators(target))).isEmpty();
        if (!addItem(LH::Software, m["name"].toString(), target).isEmpty() && !had)
            ++added;
    }
    if (added)
        emit message(QStringLiteral("Added %1 program(s).").arg(added));
    return added;
}

bool AppCore::updateItem(const QString &id, const QString &name, const QString &target)
{
    Item it = item(id);
    if (it.id.isEmpty())
        return false;
    QString t = it.category == LH::Web ? normalizeUrl(target) : QDir::toNativeSeparators(target.trimmed());
    if (it.category == LH::Collection)
        t.clear();
    else if (t.isEmpty())
        return false;
    it.target = t;
    it.name = name.trimmed().isEmpty() ? autoName(it.category, t) : name.trimmed();
    it.status = computeStatus(it);
    if (!m_db.updateItem(it))
        return false;
    m_pixCache.clear();
    ++m_iconVersion;
    changed(); // drop the cached row first, or views reload the icon from the old row
    emit iconsChanged();
    return true;
}

bool AppCore::setItemIcon(const QString &id, const QString &imagePath)
{
    const Item it = item(id);
    if (it.id.isEmpty() || !QImageReader(imagePath).canRead())
        return false;
    const QString dir = m_dataDir + "/customicons";
    QDir().mkpath(dir);
    // a fresh file name per change, so the icon cache never serves the previous image
    const QString dest = QStringLiteral("%1/%2_%3.%4")
                             .arg(dir, id)
                             .arg(QDateTime::currentMSecsSinceEpoch())
                             .arg(QFileInfo(imagePath).suffix().toLower());
    if (!QFile::copy(imagePath, dest))
        return false;
    if (!it.icon.isEmpty())
        QFile::remove(it.icon);
    m_db.setIcon(id, dest);
    m_pixCache.clear();
    ++m_iconVersion;
    changed(); // drop the cached row first, or views reload the icon from the old row
    emit iconsChanged();
    return true;
}

void AppCore::clearItemIcon(const QString &id)
{
    const Item it = item(id);
    if (it.id.isEmpty() || it.icon.isEmpty())
        return;
    QFile::remove(it.icon);
    m_db.setIcon(id, {});
    m_pixCache.clear();
    ++m_iconVersion;
    changed();
    emit iconsChanged();
}

QString AppCore::imageFilter() const
{
    QStringList exts;
    for (const QByteArray &f : QImageReader::supportedImageFormats())
        exts << QStringLiteral("*.") + QString::fromLatin1(f).toLower();
    for (const char *extra : {"*.ico", "*.svg", "*.jfif", "*.jpe", "*.tif"})
        if (!exts.contains(QLatin1String(extra)))
            exts << QLatin1String(extra);
    return QStringLiteral("Images (%1);;All files (*.*)").arg(exts.join(' '));
}

void AppCore::removeItem(const QString &id)
{
    const Item gone = item(id);
    if (gone.id.isEmpty())
        return;
    if (!gone.icon.isEmpty())
        QFile::remove(gone.icon);
    m_db.deleteItem(id);
    if (m_selectedId == id)
        select({});
    changed();
}

void AppCore::setFavorite(const QString &id, bool favorite)
{
    if (!exists(id))
        return;
    m_db.setFavorite(id, favorite);
    changed();
}

QVariantMap AppCore::itemInfo(const QString &id) const
{
    const Item it = item(id);
    if (it.id.isEmpty())
        return {};
    return {
        {"id", it.id},
        {"name", it.name},
        {"target", it.target},
        {"icon", it.icon},
        {"category", int(it.category)},
        {"categoryName", Item::categoryLabel(it.category)},
        {"favorite", it.favorite},
        {"status", int(it.status)},
        {"memberCount", it.memberCount},
    };
}

QString AppCore::subtitleFor(const Item &it) const
{
    if (it.status == LH::Missing)
        return QStringLiteral("Missing");
    if (it.status == LH::Warning)
        return QStringLiteral("Invalid URL");
    switch (it.category) {
    case LH::Web: {
        QString host = QUrl(it.target).host();
        if (host.startsWith(QLatin1String("www.")))
            host.remove(0, 4);
        return host;
    }
    case LH::Collection:
        return QStringLiteral("%1 item(s)").arg(it.memberCount);
    case LH::Software:
        return QStringLiteral("Application");
    default: {
        const QFileInfo fi(it.target);
        return fi.suffix().isEmpty() ? QStringLiteral("File") : fi.suffix().toUpper();
    }
    }
}

// ---------------------------------------------------------------------------
// Launching
// ---------------------------------------------------------------------------

namespace {
// Opens a URL in the chosen browser (an exe path), falling back to the system default.
bool openWeb(const QString &url, const QString &browserExe)
{
    if (!browserExe.isEmpty() && QFileInfo::exists(browserExe)
        && QProcess::startDetached(browserExe, {url}))
        return true;
    return QDesktopServices::openUrl(QUrl(url));
}
} // namespace

QVariantList AppCore::browsers() const
{
    QVariantList out;
    QSet<QString> seen;
    // Browsers register under StartMenuInternet in either hive.
    for (const char *root : {"HKEY_LOCAL_MACHINE", "HKEY_CURRENT_USER"}) {
        QSettings reg(QStringLiteral("%1\\SOFTWARE\\Clients\\StartMenuInternet").arg(root), QSettings::NativeFormat);
        for (const QString &key : reg.childGroups()) {
            const QString name = reg.value(key + "/Default", key).toString();
            QString cmd = reg.value(key + "/shell/open/command/Default").toString().trimmed();
            if (cmd.startsWith('"'))
                cmd = cmd.mid(1, cmd.indexOf('"', 1) - 1);
            const QString path = QDir::fromNativeSeparators(cmd);
            if (path.isEmpty() || !QFileInfo::exists(path) || seen.contains(path.toLower()))
                continue;
            seen.insert(path.toLower());
            out.append(QVariantMap{{"text", name.isEmpty() ? key : name}, {"value", path}});
        }
    }
    return out;
}

int AppCore::launch(const QString &id)
{
    Item it = item(id);
    qInfo() << "launch" << it.name << it.target;
    if (it.id.isEmpty())
        return LH::LaunchFailed;
    if (it.category == LH::Collection) {
        launchAll(id);
        return LH::Launched;
    }

    // Validation check before opening (Item Exists?).
    const LH::Status st = computeStatus(it);
    if (st != it.status) {
        m_db.applyStatuses({{id, int(st)}});
        changed();
    }
    if (st != LH::Normal)
        return LH::TargetMissing;

    bool ok = false;
    switch (it.category) {
    case LH::Web:
        ok = openWeb(it.target, m_settings->text("defaultBrowser"));
        break;
    case LH::Software:
        ok = QProcess::startDetached(it.target, {}, QFileInfo(it.target).absolutePath())
             || QDesktopServices::openUrl(QUrl::fromLocalFile(it.target));
        break;
    default:
        ok = QDesktopServices::openUrl(QUrl::fromLocalFile(it.target));
        break;
    }
    if (!ok) {
        emit message(QStringLiteral("Couldn't open \"%1\".").arg(it.name));
        return LH::LaunchFailed;
    }
    m_db.setLastOpened(id);
    if (m_settings->boolean("recentEnabled")) {
        m_db.touchRecent(id);
        m_db.trimRecents(m_settings->integer("maxRecent"));
    }
    changed();
    return LH::Launched;
}

// A collection works as a workspace: everything in it opens in order, spaced by the "workspaceDelay"
// setting so a dozen programs don't fight for the disk and the foreground at once.
// Returns how many items were queued (the first one opens right away).
int AppCore::launchAll(const QString &collectionId)
{
    const Item c = item(collectionId);
    if (c.category != LH::Collection || c.id.isEmpty())
        return 0;
    QStringList queue;
    int skipped = 0;
    for (const QString &m : m_db.members(collectionId)) {
        const Item it = item(m);
        if (it.id.isEmpty() || it.category == LH::Collection || computeStatus(it) != LH::Normal)
            ++skipped;
        else
            queue << m;
    }
    const int delay = qMax(0, m_settings->integer("workspaceDelay"));
    int ok = 0;
    for (int i = 0; i < queue.size(); ++i) {
        const QString id = queue.at(i);
        if (i == 0 || delay == 0) {
            if (launch(id) == LH::Launched)
                ++ok;
            else
                ++skipped;
        } else {
            QTimer::singleShot(i * delay, this, [this, id] {
                if (!m_closed)
                    launch(id);
            });
            ++ok;
        }
    }
    emit message(skipped ? QStringLiteral("Opening \"%1\": %2 item(s), %3 couldn't be opened.").arg(c.name).arg(ok).arg(skipped)
                         : QStringLiteral("Opening \"%1\": %2 item(s).").arg(c.name).arg(ok));
    return ok;
}

// ---------------------------------------------------------------------------
// Quick actions
// ---------------------------------------------------------------------------

bool AppCore::launchAsAdmin(const QString &id)
{
    const Item it = item(id);
    if (it.id.isEmpty() || it.category != LH::Software || computeStatus(it) != LH::Normal)
        return false;
#ifdef Q_OS_WIN
    const QString path = QDir::toNativeSeparators(it.target);
    const QString dir = QDir::toNativeSeparators(QFileInfo(it.target).absolutePath());
    // "runas" shows the UAC prompt; a result above 32 means Windows accepted it (declining the prompt is an error)
    const auto r = reinterpret_cast<quintptr>(ShellExecuteW(nullptr, L"runas", reinterpret_cast<LPCWSTR>(path.utf16()),
                                                            nullptr, reinterpret_cast<LPCWSTR>(dir.utf16()), SW_SHOWNORMAL));
    if (r <= 32) {
        emit message(QStringLiteral("\"%1\" wasn't started as administrator.").arg(it.name));
        return false;
    }
    m_db.setLastOpened(id);
    changed();
    return true;
#else
    return false;
#endif
}

bool AppCore::launchWithArgs(const QString &id, const QString &args)
{
    const Item it = item(id);
    if (it.id.isEmpty() || it.category != LH::Software || computeStatus(it) != LH::Normal)
        return false;
    const bool ok = QProcess::startDetached(it.target, QProcess::splitCommand(args), QFileInfo(it.target).absolutePath());
    if (!ok) {
        emit message(QStringLiteral("Couldn't start \"%1\" with those arguments.").arg(it.name));
        return false;
    }
    m_db.setLastOpened(id);
    changed();
    return true;
}

void AppCore::copyPath(const QString &id)
{
    const Item it = item(id);
    if (it.id.isEmpty() || it.category == LH::Collection)
        return;
    QGuiApplication::clipboard()->setText(it.category == LH::Web ? it.target : QDir::toNativeSeparators(it.target));
    emit message(it.category == LH::Web ? QStringLiteral("Link copied.") : QStringLiteral("Path copied."));
}

QVariantList AppCore::quickSearch(const QString &query, int limit) const
{
    if (m_closed)
        return {};
    const QString needle = query.trimmed();
    QStringList ids;
    if (needle.isEmpty()) {
        ids = queryIds(LH::ModeRecent, LH::AllCategories, {}, {});
        for (const QString &f : queryIds(LH::ModeFavorites, LH::AllCategories, {}, {}))
            if (!ids.contains(f))
                ids << f;
    } else {
        ids = queryIds(LH::ModeCategory, LH::AllCategories, needle, {});
        auto rank = [&](const Item &it) {
            const QString n = it.name.toLower(), q = needle.toLower();
            int r = n == q ? 0 : n.startsWith(q) ? 1 : n.contains(' ' + q) ? 2 : n.contains(q) ? 3 : 4;
            return r * 2 + (it.favorite ? 0 : 1);
        };
        QHash<QString, int> score;
        for (const QString &id : ids)
            score.insert(id, rank(item(id)));
        std::stable_sort(ids.begin(), ids.end(), [&](const QString &a, const QString &b) { return score.value(a) < score.value(b); });
    }
    QVariantList out;
    for (const QString &id : ids) {
        const Item it = item(id);
        if (it.id.isEmpty() || m_sites->isSiteGroup(id))
            continue;
        out.append(QVariantMap{
            {"id", it.id},
            {"name", it.name},
            {"subtitle", subtitleFor(it)},
            {"category", int(it.category)},
            {"categoryName", it.category == LH::Collection ? QStringLiteral("Workspace") : Item::categoryLabel(it.category)},
            {"iconSource", QStringLiteral("image://item/%1?v=%2").arg(it.id).arg(m_iconVersion)},
        });
        if (out.size() >= limit)
            break;
    }
    return out;
}

void AppCore::showInFolder(const QString &id)
{
    const Item it = item(id);
    if (it.id.isEmpty() || it.category == LH::Web || it.category == LH::Collection)
        return;
#ifdef Q_OS_WIN
    QProcess::startDetached("explorer.exe", {"/select,", QDir::toNativeSeparators(it.target)});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(it.target).absolutePath()));
#endif
}

// ---------------------------------------------------------------------------
// Drag & drop
// ---------------------------------------------------------------------------

QString AppCore::addDroppedItem(const QString &raw, int hintCategory, bool *isNew)
{
    static const QSet<QString> programs = {"exe", "lnk", "bat", "cmd"};
    static const QSet<QString> media = {"mp4", "mkv", "avi", "mov", "webm", "mp3", "wav", "flac", "ogg", "m4a",
                                        "png", "jpg", "jpeg", "gif", "bmp", "webp", "svg"};
    *isNew = false;
    const QString s = raw.trimmed();
    if (s.isEmpty())
        return {};
    const QUrl url(s);
    LH::Category cat = LH::Web;
    QString norm;
    if (url.scheme() == "http" || url.scheme() == "https") {
        norm = Item::normalizedTarget(normalizeUrl(s));
    } else {
        const QString path = url.isLocalFile() ? url.toLocalFile() : s;
        const QFileInfo fi(path);
        if (!fi.exists())
            return {};
        const QString ext = fi.suffix().toLower();
        cat = LH::Document;
        if (programs.contains(ext))
            cat = LH::Software;
        else if (media.contains(ext))
            cat = LH::Entertainment;
        else if (hintCategory >= 0 && hintCategory != LH::Collection && hintCategory != LH::Web)
            cat = LH::Category(hintCategory);
        norm = Item::normalizedTarget(QDir::toNativeSeparators(path));
    }
    *isNew = m_db.findByNorm(cat, norm).isEmpty();
    return addItem(cat, {}, url.isLocalFile() ? url.toLocalFile() : s); // an existing item comes back as itself
}

int AppCore::addDropped(const QStringList &dropped, int hintCategory, bool favorite)
{
    int added = 0;
    for (const QString &raw : dropped) {
        bool isNew = false;
        const QString id = addDroppedItem(raw, hintCategory, &isNew);
        if (id.isEmpty() || !isNew)
            continue;
        ++added;
        if (favorite)
            setFavorite(id, true); // dropped on the dock: it should show up there
    }
    emit message(added ? QStringLiteral("Added %1 item(s) from drop.").arg(added)
                       : QStringLiteral("Nothing new to add from that drop."));
    return added;
}

int AppCore::addDroppedTo(const QString &collectionId, const QStringList &dropped)
{
    if (item(collectionId).category != LH::Collection)
        return 0;
    QStringList ids;
    for (const QString &raw : dropped) {
        bool isNew = false;
        const QString id = addDroppedItem(raw, -1, &isNew);
        if (!id.isEmpty())
            ids << id;
    }
    addToCollection(collectionId, ids);
    emit message(ids.isEmpty() ? QStringLiteral("Nothing usable in that drop.")
                               : QStringLiteral("Put %1 item(s) into \"%2\".").arg(ids.size()).arg(item(collectionId).name));
    return ids.size();
}

void AppCore::moveMember(const QString &collectionId, const QString &itemId, int toRow)
{
    m_db.moveMember(collectionId, itemId, toRow);
    changed();
}

// ---------------------------------------------------------------------------
// Collections
// ---------------------------------------------------------------------------

QString AppCore::createCollection(const QString &name)
{
    return addItem(LH::Collection, name.trimmed().isEmpty() ? QStringLiteral("New Collection") : name, {});
}

void AppCore::addToCollection(const QString &collectionId, const QStringList &itemIds)
{
    if (item(collectionId).category != LH::Collection)
        return;
    bool any = false;
    for (const QString &id : itemIds) {
        const Item it = item(id);
        if (it.id.isEmpty() || it.category == LH::Collection)
            continue;
        any = m_db.addMember(collectionId, id) || any;
    }
    if (any)
        changed();
}

void AppCore::removeFromCollection(const QString &collectionId, const QString &itemId)
{
    m_db.removeMember(collectionId, itemId);
    changed();
}

QVariantList AppCore::collectionsList() const { return m_closed ? QVariantList() : m_db.collections(); }

QVariantList AppCore::addableItems(const QString &collectionId) const
{
    if (m_closed)
        return {};
    return m_db.addable(collectionId, 1000);
}

// ---------------------------------------------------------------------------
// Software catalog
// ---------------------------------------------------------------------------

void AppCore::refreshSoftware()
{
    if (m_scanning)
        return;
    m_scanning = true;
    emit scanningChanged();
    m_scanWatcher.setFuture(QtConcurrent::run(&scanInstalledSoftware));
}

QVariantList AppCore::catalogSearch(const QString &search, int limit) const
{
    if (m_closed)
        return {};
    return m_db.catalog(search, limit);
}

// ---------------------------------------------------------------------------
// Icons
// ---------------------------------------------------------------------------

QString AppCore::faviconPath(const QString &host) const
{
    QString safe = host;
    safe.replace(QRegularExpression("[^A-Za-z0-9._-]"), "_");
    return m_dataDir + "/icons/" + safe + ".png";
}

// Looks up a site's real icon: the page's own <link rel="icon"> / apple-touch-icon first,
// then /favicon.ico. The result is cached on disk; until then (or if the site has none) the
// card shows a coloured letter tile.
void AppCore::requestFavicon(const QString &host)
{
    if (host.isEmpty() || m_faviconRequested.contains(host))
        return;
    m_faviconRequested.insert(host);
    fetchFavicon(host, QUrl(QStringLiteral("https://%1/").arg(host)));
}

void AppCore::fetchFavicon(const QString &host, const QUrl &pageUrl)
{
    if (!m_net)
        m_net = new QNetworkAccessManager(this);
    QNetworkRequest req(pageUrl);
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Dockly/%1").arg(version()));
    req.setTransferTimeout(8000);
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, host, pageUrl] {
        reply->deleteLater();
        QList<QUrl> candidates;
        if (reply->error() == QNetworkReply::NoError) {
            const QString html = QString::fromUtf8(reply->read(256 * 1024));
            const QUrl base = reply->url().isValid() ? reply->url() : pageUrl;
            static const QRegularExpression linkTag("<link\\b[^>]*>", QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression relAttr("\\brel\\s*=\\s*[\"']([^\"']*)[\"']", QRegularExpression::CaseInsensitiveOption);
            static const QRegularExpression hrefAttr("\\bhref\\s*=\\s*[\"']([^\"']+)[\"']", QRegularExpression::CaseInsensitiveOption);
            QList<QUrl> touch, plain;
            auto it = linkTag.globalMatch(html);
            while (it.hasNext()) {
                const QString tag = it.next().captured(0);
                const auto rel = relAttr.match(tag);
                const auto href = hrefAttr.match(tag);
                if (!rel.hasMatch() || !href.hasMatch())
                    continue;
                const QString r = rel.captured(1).toLower();
                const QUrl u = base.resolved(QUrl(href.captured(1).trimmed()));
                if (!u.isValid())
                    continue;
                if (r.contains("apple-touch-icon"))
                    touch << u;
                else if (r.contains("icon"))
                    plain << u;
            }
            candidates = touch + plain;
        }
        candidates << QUrl(QStringLiteral("%1://%2/apple-touch-icon.png").arg(pageUrl.scheme(), host));
        candidates << QUrl(QStringLiteral("%1://%2/favicon.ico").arg(pageUrl.scheme(), host));
        tryIcon(host, candidates);
    });
}

// Tries the candidates in order and keeps the first one that is at least 64px, since a 16px or
// 32px favicon turns soft when it is scaled up to a card. If none is that large, the biggest one
// seen is kept.
void AppCore::tryIcon(const QString &host, const QList<QUrl> &candidates, const QImage &best)
{
    if (candidates.isEmpty()) {
        if (!best.isNull())
            storeIcon(host, best);
        return; // otherwise keep the letter tile
    }
    QNetworkRequest req(candidates.first());
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setTransferTimeout(8000);
    QNetworkReply *reply = m_net->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, host, candidates, best] {
        reply->deleteLater();
        QImage img;
        if (reply->error() == QNetworkReply::NoError)
            img = QImage::fromData(reply->readAll());
        if (img.width() >= 64) {
            storeIcon(host, img);
            return;
        }
        const bool bigger = !img.isNull() && (best.isNull() || img.width() > best.width());
        tryIcon(host, candidates.mid(1), bigger ? img : best);
    });
}

void AppCore::storeIcon(const QString &host, const QImage &img)
{
    img.save(faviconPath(host), "PNG");
    m_pixCache.clear();
    ++m_iconVersion;
    emit iconsChanged();
}

// Stand-in icon for sites without a (fetched) icon: rounded tile in a colour derived from the
// site name, with its first letter.
QPixmap AppCore::letterTile(const QString &seed, const QString &label, int size) const
{
    QPixmap pm(size, size);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    const uint h = qHash(seed.toLower(), 0x9e3779b9);
    const QColor base = QColor::fromHsl(int(h % 360), 150, 120);
    QLinearGradient g(0, 0, size, size);
    g.setColorAt(0, base.lighter(125));
    g.setColorAt(1, base.darker(115));
    p.setPen(Qt::NoPen);
    p.setBrush(g);
    p.drawRoundedRect(QRectF(size * 0.06, size * 0.06, size * 0.88, size * 0.88), size * 0.24, size * 0.24);
    QString letter = label.trimmed();
    if (letter.startsWith(QLatin1String("www.")))
        letter.remove(0, 4);
    letter = letter.left(1).toUpper();
    QFont f(QStringLiteral("Segoe UI"));
    f.setBold(true);
    f.setPixelSize(int(size * 0.5));
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(QRectF(0, 0, size, size), Qt::AlignCenter, letter);
    return pm;
}

QPixmap AppCore::pixmapForFile(const QString &path, int size)
{
    if (path.isEmpty() || size <= 0)
        return {};
    const QString key = QStringLiteral("file:") + path + QLatin1Char('|') + QString::number(size);
    if (const auto c = m_pixCache.constFind(key); c != m_pixCache.cend())
        return c.value();
    if (m_pixCache.size() > 300)
        m_pixCache.clear();
    QPixmap pm = m_iconProvider.icon(QFileInfo(path)).pixmap(size);
    if (pm.isNull())
        pm = m_iconProvider.icon(QFileIconProvider::File).pixmap(size);
    m_pixCache.insert(key, pm);
    return pm;
}

QPixmap AppCore::pixmapFor(const QString &id, int size)
{
    const Item it = item(id);
    if (it.id.isEmpty() || size <= 0)
        return {};
    if (it.icon.isEmpty())
        return pixmapOriginal(id, size);
    const QString key = id + QLatin1Char('|') + QString::number(size);
    if (const auto c = m_pixCache.constFind(key); c != m_pixCache.cend())
        return c.value();
    if (m_pixCache.size() > 300)
        m_pixCache.clear();

    QPixmap pm;
    QImageReader reader(it.icon);
    reader.setAutoTransform(true);
    const QSize s = reader.size();
    if (s.isValid())
        reader.setScaledSize(s.scaled(size, size, Qt::KeepAspectRatio));
    const QImage img = reader.read();
    if (!img.isNull())
        pm = QPixmap::fromImage(img);
    else
        pm = pixmapOriginal(id, size); // custom file went missing or unreadable
    m_pixCache.insert(key, pm);
    return pm;
}

QPixmap AppCore::pixmapOriginal(const QString &id, int size)
{
    const Item it = item(id);
    if (it.id.isEmpty() || size <= 0)
        return {};
    const QString key = QStringLiteral("o:") + id + QLatin1Char('|') + QString::number(size);
    if (const auto c = m_pixCache.constFind(key); c != m_pixCache.cend())
        return c.value();
    if (m_pixCache.size() > 300)
        m_pixCache.clear();
    const QPixmap pm = originalPixmap(it, size);
    m_pixCache.insert(key, pm);
    return pm;
}

QPixmap AppCore::pixmapForNew(int category, const QString &target, const QString &name, int size)
{
    Item it;
    it.category = LH::Category(category);
    it.name = name;
    it.target = it.category == LH::Web ? normalizeUrl(target) : QDir::toNativeSeparators(target.trimmed());
    if (size <= 0 || (it.target.isEmpty() && it.category != LH::Collection))
        return {};
    return originalPixmap(it, size);
}

QPixmap AppCore::originalPixmap(const Item &it, int size)
{
    QPixmap pm;
    switch (it.category) {
    case LH::Web: {
        const QString host = QUrl(it.target).host();
        const QString path = faviconPath(host);
        const bool cached = QFile::exists(path);
        if (cached)
            pm = QPixmap(path).scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        // a small cached icon (from before larger icons were preferred) is fetched again
        if ((!cached || QImageReader(path).size().width() < 64) && QUrl(it.target).scheme().startsWith("http"))
            requestFavicon(host);
        if (pm.isNull())
            pm = letterTile(QUrl(it.target).host(), it.name, size);
        break;
    }
    case LH::Collection:
        pm = m_iconProvider.icon(QFileIconProvider::Folder).pixmap(size);
        break;
    default: {
        const QFileInfo fi(it.target);
        static const QSet<QString> thumbs = {"png", "jpg", "jpeg", "bmp", "gif", "webp"};
        if (it.category == LH::Entertainment && fi.exists() && thumbs.contains(fi.suffix().toLower())) {
            QImageReader reader(it.target);
            reader.setAutoTransform(true);
            const QSize s = reader.size();
            if (s.isValid())
                reader.setScaledSize(s.scaled(size, size, Qt::KeepAspectRatio));
            const QImage img = reader.read();
            if (!img.isNull())
                pm = QPixmap::fromImage(img);
        }
        if (pm.isNull()) {
            pm = m_iconProvider.icon(fi).pixmap(size);
            if (pm.isNull())
                pm = m_iconProvider.icon(QFileIconProvider::File).pixmap(size);
        }
        break;
    }
    }
    return pm;
}

// ---------------------------------------------------------------------------
// Settings & data actions
// ---------------------------------------------------------------------------

void AppCore::applyStartWithWindows()
{
#ifdef Q_OS_WIN
    QSettings run(QStringLiteral("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run"),
                  QSettings::NativeFormat);
    const QString name = QStringLiteral("Dockly");
    run.remove(QStringLiteral("LauncherHub")); // entry written by pre-rename builds
    if (m_settings->boolean("startWithWindows"))
        run.setValue(name, QStringLiteral("\"%1\" --minimized")
                               .arg(QDir::toNativeSeparators(QCoreApplication::applicationFilePath())));
    else
        run.remove(name);
#endif
}

QString AppCore::defaultBackupPath() const
{
    return m_dataDir + "/backups/Dockly-"
           + QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss") + ".json";
}

bool AppCore::backupTo(const QString &path)
{
    QJsonObject o;
    o["app"] = QStringLiteral("Dockly");
    o["version"] = kBackupFormatVersion;
    o["settings"] = m_settings->toJson();
    o["data"] = m_db.exportAll();
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        emit message(QStringLiteral("Couldn't write the backup file."));
        return false;
    }
    f.write(QJsonDocument(o).toJson(QJsonDocument::Indented));
    if (!f.commit()) {
        emit message(QStringLiteral("Couldn't write the backup file."));
        return false;
    }
    emit message(QStringLiteral("Backup saved to %1").arg(QDir::toNativeSeparators(path)));
    return true;
}

bool AppCore::restoreFrom(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        emit message(QStringLiteral("Couldn't open the backup file."));
        return false;
    }
    const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    const QString appTag = o["app"].toString();
    // Backups made before the rename carry the old "LauncherHub" tag.
    if ((appTag != QLatin1String("Dockly") && appTag != QLatin1String("LauncherHub")) || !o["data"].isObject()) {
        emit message(QStringLiteral("That file isn't a Dockly backup."));
        return false;
    }
    // Older (v1) backups flagged detected programs as autoDetected: keep those out of the cards.
    QJsonObject data = o["data"].toObject();
    QJsonArray keep;
    for (const QJsonValue &v : data["items"].toArray()) {
        const QJsonObject io = v.toObject();
        if (!(io["autoDetected"].toBool() && !io["favorite"].toBool()))
            keep.append(io);
    }
    data["items"] = keep;
    if (!m_db.importAll(data)) {
        emit message(QStringLiteral("Restore failed; your data was left unchanged."));
        return false;
    }
    m_settings->fromJson(o["settings"].toObject());
    select({});
    m_pixCache.clear();
    ++m_iconVersion;
    emit iconsChanged();
    changed();
    saveNow();
    checkItems();
    emit message(QStringLiteral("Data restored from backup."));
    return true;
}

void AppCore::resetSettings()
{
    m_settings->resetToDefaults();
    saveNow();
    emit message(QStringLiteral("Settings restored to defaults."));
}

void AppCore::openBackupFolder()
{
    QDir().mkpath(m_dataDir + "/backups");
    QDesktopServices::openUrl(QUrl::fromLocalFile(m_dataDir + "/backups"));
}

void AppCore::clearCache()
{
    QDir icons(m_dataDir + "/icons");
    const QStringList files = icons.entryList(QDir::Files);
    for (const QString &f : files)
        icons.remove(f);
    m_pixCache.clear();
    m_cache.clear();
    m_faviconRequested.clear();
    ++m_iconVersion;
    emit iconsChanged();
    emit message(QStringLiteral("Cache cleared (%1 file(s)).").arg(files.size()));
}

QString AppCore::checkForUpdates() const
{
    return QStringLiteral("You're running Dockly %1.\nNo update server is configured for this build.")
        .arg(version());
}

QString AppCore::aboutText() const
{
    return QStringLiteral("Dockly %1\nA launcher / hub / shortcut plate built with C++ and Qt %2.\n\n"
                          "Robin Gupta Studios (RGS™)\nAuthor: Robin Gupta\n"
                          "Copyright © Robin Gupta\n\n"
                          "Data folder:\n%3")
        .arg(version(), QString::fromLatin1(qVersion()), QDir::toNativeSeparators(m_dataDir));
}

// ---------------------------------------------------------------------------
// Native dialogs
// ---------------------------------------------------------------------------

QString AppCore::fileFilter(int category) const
{
    switch (category) {
    case LH::Software:
        return QStringLiteral("Programs (*.exe *.lnk *.bat *.cmd);;All files (*.*)");
    case LH::Document:
        return QStringLiteral("Documents (*.pdf *.doc *.docx *.xls *.xlsx *.ppt *.pptx *.txt *.md *.rtf *.csv *.odt);;All files (*.*)");
    case LH::Entertainment:
        return QStringLiteral("Media (*.mp4 *.mkv *.avi *.mov *.webm *.mp3 *.wav *.flac *.png *.jpg *.jpeg *.gif *.bmp *.webp);;All files (*.*)");
    default:
        return QStringLiteral("All files (*.*)");
    }
}

QStringList AppCore::pickFiles(const QString &title, const QString &filter, bool multiple)
{
    if (multiple)
        return QFileDialog::getOpenFileNames(nullptr, title, {}, filter);
    const QString f = QFileDialog::getOpenFileName(nullptr, title, {}, filter);
    return f.isEmpty() ? QStringList() : QStringList{f};
}

QString AppCore::pickSaveFile(const QString &title, const QString &path, const QString &filter)
{
    return QFileDialog::getSaveFileName(nullptr, title, path, filter);
}

QString AppCore::pickColor(const QString &initial)
{
    const QColor c = QColorDialog::getColor(QColor(initial), nullptr, QStringLiteral("Accent colour"));
    return c.isValid() ? c.name() : QString();
}
