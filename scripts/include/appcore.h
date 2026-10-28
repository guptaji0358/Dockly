#pragma once

#include "appsettings.h"
#include "connectors.h"
#include "database.h"
#include "item.h"
#include "sites.h"
#include "softwarescanner.h"
#include "theme.h"

#include <QFileIconProvider>
#include <QFutureWatcher>
#include <QHash>
#include <QIcon>
#include <QObject>
#include <QPixmap>
#include <QSet>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class QNetworkAccessManager;

// Owns settings, the SQLite databases, launching and validation.
// Both frontends talk only to this class and ItemModel. Only a small row cache lives in
// RAM; everything else is queried from SQLite on demand.
class AppCore : public QObject
{
    Q_OBJECT
    Q_PROPERTY(AppSettings *settings READ settings CONSTANT)
    Q_PROPERTY(Theme *theme READ theme CONSTANT)
    Q_PROPERTY(int iconVersion READ iconVersion NOTIFY iconsChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString selectedId READ selectedId NOTIFY selectionChanged)
    Q_PROPERTY(int catalogCount READ catalogCount NOTIFY catalogChanged)
    Q_PROPERTY(Connectors *connectors READ connectors CONSTANT)
    Q_PROPERTY(Sites *sites READ sites CONSTANT)

public:
    static AppCore *instance();
    static QIcon appIcon();

    // Loads settings, opens the databases (creating them on first run), starts background checks.
    void init();

    AppSettings *settings() const { return m_settings; }
    Theme *theme() const { return m_theme; }
    Connectors *connectors() const { return m_connectors; }
    Sites *sites() const { return m_sites; }
    // new web shortcuts are filed into their site's group; importers that pick their own group switch it off
    void setAutoFile(bool on) { m_autoFile = on; }
    bool autoFile() const { return m_autoFile; }
    QString findWeb(const QString &url) const; // id of an existing web shortcut for this link, or empty
    int iconVersion() const { return m_iconVersion; }
    bool scanning() const { return m_scanning; }
    QString version() const;
    QString selectedId() const { return m_selectedId; }
    int catalogCount() const { return m_closed ? 0 : m_db.catalogCount(); }

    // Row access (cached). Returns an Item with an empty id when it does not exist.
    Item item(const QString &id) const;
    bool exists(const QString &id) const { return !item(id).id.isEmpty(); }
    QStringList queryIds(int mode, int category, const QString &search, const QString &collectionId) const;
    QString subtitleFor(const Item &it) const;
    QPixmap pixmapFor(const QString &id, int size);          // what the card shows (custom icon if set)
    QPixmap pixmapOriginal(const QString &id, int size);     // the automatic icon, ignoring any custom one
    QPixmap pixmapForNew(int category, const QString &target, const QString &name, int size); // add dialog preview
    QPixmap pixmapForFile(const QString &path, int size); // shell icon of any file (catalog entries)

    // --- item management -------------------------------------------------
    Q_INVOKABLE QString addItem(int category, const QString &name, const QString &target);
    Q_INVOKABLE QStringList addFiles(int category, const QStringList &paths);
    Q_INVOKABLE bool setItemIcon(const QString &id, const QString &imagePath); // copies the image in
    Q_INVOKABLE void clearItemIcon(const QString &id);                         // back to the automatic icon
    Q_INVOKABLE QString imageFilter() const;                                   // every readable image type
    Q_INVOKABLE bool updateItem(const QString &id, const QString &name, const QString &target);
    Q_INVOKABLE void removeItem(const QString &id);
    Q_INVOKABLE void setFavorite(const QString &id, bool favorite);
    Q_INVOKABLE QVariantMap itemInfo(const QString &id) const;
    Q_INVOKABLE void select(const QString &id);

    // --- launching -------------------------------------------------------
    Q_INVOKABLE int launch(const QString &id); // returns LH::LaunchResult
    Q_INVOKABLE int launchAll(const QString &collectionId);
    Q_INVOKABLE void showInFolder(const QString &id);
    Q_INVOKABLE QVariantList browsers() const; // installed browsers: [{text, value = exe path}]

    // --- drag & drop ------------------------------------------------------
    // Files, folders or URLs (as paths or url strings) dropped on the window; each goes to the
    // matching hub. hintCategory is used for files with an unknown type (-1 = documents).
    Q_INVOKABLE int addDropped(const QStringList &items, int hintCategory = -1, bool favorite = false);
    Q_INVOKABLE void moveMember(const QString &collectionId, const QString &itemId, int toRow);

    // --- collections -----------------------------------------------------
    Q_INVOKABLE QString createCollection(const QString &name);
    Q_INVOKABLE void addToCollection(const QString &collectionId, const QStringList &itemIds);
    Q_INVOKABLE void removeFromCollection(const QString &collectionId, const QString &itemId);
    Q_INVOKABLE QVariantList collectionsList() const;
    Q_INVOKABLE QVariantList addableItems(const QString &collectionId) const;

    // --- software catalog (hidden backend; never shown unless the user picks) ---
    Q_INVOKABLE void refreshSoftware();                               // on-demand Start Menu scan
    Q_INVOKABLE QVariantList catalogSearch(const QString &search, int limit = 200) const;
    Q_INVOKABLE int addSoftware(const QVariantList &entries);          // [{name, target}], returns added

    // --- validation --------------------------------------------------------
    Q_INVOKABLE void checkItems();

    // --- data & settings actions ----------------------------------------
    Q_INVOKABLE QString defaultBackupPath() const;
    Q_INVOKABLE bool backupTo(const QString &path);
    Q_INVOKABLE bool restoreFrom(const QString &path);
    Q_INVOKABLE void resetSettings();
    Q_INVOKABLE void openBackupFolder();
    Q_INVOKABLE void clearCache();
    Q_INVOKABLE QString checkForUpdates() const;
    Q_INVOKABLE QString aboutText() const;
    Q_INVOKABLE QString dataFolder() const { return m_dataDir; }

    // --- native dialogs (used by QML; widgets use their own with a parent)
    Q_INVOKABLE QString fileFilter(int category) const;
    Q_INVOKABLE QStringList pickFiles(const QString &title, const QString &filter, bool multiple);
    Q_INVOKABLE QString pickSaveFile(const QString &title, const QString &path, const QString &filter);
    Q_INVOKABLE QString pickColor(const QString &initial);

    static LH::Status computeStatus(const Item &it);

public slots:
    void saveNow();

signals:
    void itemsChanged();
    void iconsChanged();
    void scanningChanged();
    void catalogChanged();
    void selectionChanged();
    void message(const QString &text);

private:
    explicit AppCore(QObject *parent = nullptr);

    QString settingsPath() const;
    void changed();
    void migrateLegacyJson();
    void applyStartWithWindows();
    void requestFavicon(const QString &host);
    QString faviconPath(const QString &host) const;
    QPixmap originalPixmap(const Item &it, int size);
    QPixmap letterTile(const QString &seed, const QString &label, int size) const;
    void fetchFavicon(const QString &host, const QUrl &pageUrl);
    void tryIcon(const QString &host, const QList<QUrl> &candidates, const QImage &best = {});
    void storeIcon(const QString &host, const QImage &img);

    AppSettings *m_settings;
    Theme *m_theme;
    Connectors *m_connectors = nullptr;
    Sites *m_sites = nullptr;
    bool m_autoFile = true;
    Database m_db;
    mutable QHash<QString, Item> m_cache;
    QString m_selectedId;
    QString m_dataDir;
    QTimer m_saveTimer;
    QTimer m_checkTimer;
    int m_iconVersion = 0;
    bool m_scanning = false;
    bool m_checking = false;
    bool m_closed = false;

    QFileIconProvider m_iconProvider;
    QHash<QString, QPixmap> m_pixCache;
    QNetworkAccessManager *m_net = nullptr;
    QSet<QString> m_faviconRequested;

    QFutureWatcher<QVector<ScannedApp>> m_scanWatcher;
    QFutureWatcher<QHash<QString, int>> m_checkWatcher;
};
