#pragma once

#include <QObject>
#include <QSet>
#include <QStringList>
#include <QVariantList>
#include <QVector>

class AppCore;

// A "site" is a user-editable rule: a name, the hosts it covers (chatgpt.com, github.com, ...), and how
// its links are grouped. Web shortcuts that match a site are filed into that site's groups
// automatically, and each site gets its own page and dock icon. GitHub is just one preset.
// Rules live in sites.json next to the databases.
class Sites : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList list READ list NOTIFY changed)

public:
    explicit Sites(AppCore *core);

    void load(const QString &dir); // reads sites.json, seeding the presets on first run
    void autoFile(const QString &itemId); // called for every newly added web shortcut

    // dock / page view of one site: the links of every group, ordered by group name
    QStringList viewIds(int siteIndex, const QString &search) const;
    bool isSiteGroup(const QString &collectionId) const; // a collection holding only links of one site

    QVariantList list() const;
    QString findOrCreateGroup(const QString &name);
    QVariantMap match(const QString &url) const;   // {exists, host, site?, group?, autoFile?} for the browser extension
    QVariantList overview() const;                 // [{id, name, hosts, groupBy, autoFile, groups: [{id, name, links: [{name, url}]}]}]
    Q_INVOKABLE QVariantList groups(int siteIndex) const; // [{id, name, links: [{id, name, path}]}]
    Q_INVOKABLE QVariantList dockEntries() const;         // sites that have groups: [{name, cat, icon, col}]

    Q_INVOKABLE bool saveSite(const QVariantMap &site);   // {id?, name, hosts, groupBy, autoFile, icon, color}
    Q_INVOKABLE void removeSite(const QString &id);
    Q_INVOKABLE void resetPresets();                      // adds back any missing preset

    // loose web links that match a site but sit in no group yet, and a one-click tidy-up
    Q_INVOKABLE int looseCount() const;
    Q_INVOKABLE int organizeAll();

    // hosts you have many loose links on but no site for yet: [{host, name, count}]
    Q_INVOKABLE QVariantList suggestions() const;
    Q_INVOKABLE bool acceptSuggestion(const QString &host);
    Q_INVOKABLE void dismissSuggestion(const QString &host);

signals:
    void changed();

private:
    struct Site
    {
        QString id, name, groupBy, icon, color;
        QStringList hosts;
        bool autoFile = true;
    };

    void save() const;
    void seedPresets(bool onlyMissing);
    int siteForUrl(const QString &url, QString *matchedHost = nullptr) const;
    QString groupNameFor(const Site &s, const QString &url, const QString &matchedHost) const;
    int siteOfCollection(const QString &collectionId) const;
    QMultiMap<QString, QString> looseByGroup() const; // group name -> item ids
    static QString normalizeHost(const QString &s);

    AppCore *m_core;
    QString m_path;
    QVector<Site> m_sites;
    QSet<QString> m_dismissed;
};
