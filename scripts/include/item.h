#pragma once

#include "lhtypes.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>
#include <QStringList>

struct Item {
    QString id;
    LH::Category category = LH::Software;
    QString name;
    QString target; // file path, or URL for web shortcuts
    QString icon; // optional user-chosen icon image (a copy kept in the data folder); empty = automatic
    bool favorite = false;
    LH::Status status = LH::Normal; // runtime only, recomputed by validation
    QDateTime lastOpened;
    QStringList members; // collections only; filled for backup export, otherwise queried from the DB
    int memberCount = 0; // runtime: number of members (collections)

    QJsonObject toJson() const;
    static Item fromJson(const QJsonObject &o);

    static QString categoryKey(LH::Category c);
    static LH::Category categoryFromKey(const QString &key);
    static QString categoryLabel(LH::Category c);
    static QString normalizedTarget(const QString &target);
};
