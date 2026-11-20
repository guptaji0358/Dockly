/****************************************************************************
** Meta object code from reading C++ file 'appcore.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../scripts/include/appcore.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'appcore.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.12.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN7AppCoreE_t {};
} // unnamed namespace

template <> constexpr inline auto AppCore::qt_create_metaobjectdata<qt_meta_tag_ZN7AppCoreE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "AppCore",
        "processInspection",
        "",
        "id",
        "QVariantMap",
        "result",
        "guardActions",
        "QVariantList",
        "records",
        "itemsChanged",
        "iconsChanged",
        "scanningChanged",
        "catalogChanged",
        "selectionChanged",
        "launcherRequested",
        "message",
        "text",
        "memoryTrimmed",
        "saveNow",
        "addItem",
        "category",
        "name",
        "target",
        "addFiles",
        "paths",
        "setItemIcon",
        "imagePath",
        "clearItemIcon",
        "imageFilter",
        "updateItem",
        "autoRelocate",
        "removeItem",
        "setFavorite",
        "favorite",
        "setItemTags",
        "tagsText",
        "tagList",
        "setGuard",
        "on",
        "itemInfo",
        "inspectProcesses",
        "guardInfo",
        "guardSetAllowed",
        "exePath",
        "mode",
        "guardSetClosedPolicy",
        "graceMs",
        "allowForce",
        "guardSetOpenPolicy",
        "enabled",
        "forceAcknowledged",
        "guardRunClosed",
        "guardRunOpen",
        "guardRecover",
        "startupRows",
        "startupDisable",
        "source",
        "startupRestore",
        "entryId",
        "startupRestoreAll",
        "startupLedger",
        "startupCheck",
        "select",
        "launch",
        "launchNew",
        "launchAll",
        "collectionId",
        "showInFolder",
        "launchAsAdmin",
        "launchWithArgs",
        "args",
        "copyPath",
        "quickSearch",
        "query",
        "limit",
        "showLauncher",
        "browsers",
        "addDropped",
        "items",
        "hintCategory",
        "moveMember",
        "itemId",
        "toRow",
        "addDroppedTo",
        "createCollection",
        "addToCollection",
        "itemIds",
        "removeFromCollection",
        "collectionsList",
        "addableItems",
        "refreshSoftware",
        "catalogSearch",
        "search",
        "addSoftware",
        "entries",
        "checkItems",
        "defaultBackupPath",
        "backupTo",
        "path",
        "restoreFrom",
        "resetSettings",
        "openBackupFolder",
        "clearCache",
        "trimMemory",
        "memoryUsageMB",
        "checkForUpdates",
        "aboutText",
        "dataFolder",
        "fileFilter",
        "pickFiles",
        "title",
        "filter",
        "multiple",
        "pickSaveFile",
        "pickColor",
        "initial",
        "settings",
        "AppSettings*",
        "theme",
        "Theme*",
        "iconVersion",
        "scanning",
        "version",
        "selectedId",
        "catalogCount",
        "connectors",
        "Connectors*",
        "sites",
        "Sites*",
        "running",
        "RunningApps*",
        "gamepad",
        "Gamepad*",
        "perf",
        "PerfMonitor*"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'processInspection'
        QtMocHelpers::SignalData<void(const QString &, const QVariantMap &)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { 0x80000000 | 4, 5 },
        }}),
        // Signal 'guardActions'
        QtMocHelpers::SignalData<void(const QString &, const QVariantList &)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { 0x80000000 | 7, 8 },
        }}),
        // Signal 'itemsChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'iconsChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'scanningChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'catalogChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'selectionChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'launcherRequested'
        QtMocHelpers::SignalData<void()>(14, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'message'
        QtMocHelpers::SignalData<void(const QString &)>(15, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 16 },
        }}),
        // Signal 'memoryTrimmed'
        QtMocHelpers::SignalData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'saveNow'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'addItem'
        QtMocHelpers::MethodData<QString(int, const QString &, const QString &)>(19, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 20 }, { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Method 'addFiles'
        QtMocHelpers::MethodData<QStringList(int, const QStringList &)>(23, 2, QMC::AccessPublic, QMetaType::QStringList, {{
            { QMetaType::Int, 20 }, { QMetaType::QStringList, 24 },
        }}),
        // Method 'setItemIcon'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(25, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 26 },
        }}),
        // Method 'clearItemIcon'
        QtMocHelpers::MethodData<void(const QString &)>(27, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'imageFilter'
        QtMocHelpers::MethodData<QString() const>(28, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'updateItem'
        QtMocHelpers::MethodData<bool(const QString &, const QString &, const QString &)>(29, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 21 }, { QMetaType::QString, 22 },
        }}),
        // Method 'autoRelocate'
        QtMocHelpers::MethodData<bool(const QString &)>(30, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'removeItem'
        QtMocHelpers::MethodData<void(const QString &)>(31, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'setFavorite'
        QtMocHelpers::MethodData<void(const QString &, bool)>(32, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Bool, 33 },
        }}),
        // Method 'setItemTags'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(34, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 35 },
        }}),
        // Method 'tagList'
        QtMocHelpers::MethodData<QVariantList(int) const>(36, 2, QMC::AccessPublic, 0x80000000 | 7, {{
            { QMetaType::Int, 20 },
        }}),
        // Method 'tagList'
        QtMocHelpers::MethodData<QVariantList() const>(36, 2, QMC::AccessPublic | QMC::MethodCloned, 0x80000000 | 7),
        // Method 'setGuard'
        QtMocHelpers::MethodData<void(const QString &, bool)>(37, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Bool, 38 },
        }}),
        // Method 'itemInfo'
        QtMocHelpers::MethodData<QVariantMap(const QString &) const>(39, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'inspectProcesses'
        QtMocHelpers::MethodData<void(const QString &)>(40, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'guardInfo'
        QtMocHelpers::MethodData<QVariantMap(const QString &)>(41, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'guardSetAllowed'
        QtMocHelpers::MethodData<void(const QString &, const QString &, const QString &, bool)>(42, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 43 }, { QMetaType::QString, 44 }, { QMetaType::Bool, 38 },
        }}),
        // Method 'guardSetClosedPolicy'
        QtMocHelpers::MethodData<void(const QString &, int, bool)>(45, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Int, 46 }, { QMetaType::Bool, 47 },
        }}),
        // Method 'guardSetOpenPolicy'
        QtMocHelpers::MethodData<void(const QString &, bool, bool)>(48, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 }, { QMetaType::Bool, 49 }, { QMetaType::Bool, 50 },
        }}),
        // Method 'guardRunClosed'
        QtMocHelpers::MethodData<void(const QString &)>(51, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'guardRunOpen'
        QtMocHelpers::MethodData<void(const QString &)>(52, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'guardRecover'
        QtMocHelpers::MethodData<QStringList()>(53, 2, QMC::AccessPublic, QMetaType::QStringList),
        // Method 'startupRows'
        QtMocHelpers::MethodData<QVariantList()>(54, 2, QMC::AccessPublic, 0x80000000 | 7),
        // Method 'startupDisable'
        QtMocHelpers::MethodData<QVariantMap(int, const QString &)>(55, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::Int, 56 }, { QMetaType::QString, 21 },
        }}),
        // Method 'startupRestore'
        QtMocHelpers::MethodData<QVariantMap(double)>(57, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::Double, 58 },
        }}),
        // Method 'startupRestoreAll'
        QtMocHelpers::MethodData<QVariantList()>(59, 2, QMC::AccessPublic, 0x80000000 | 7),
        // Method 'startupLedger'
        QtMocHelpers::MethodData<QVariantList()>(60, 2, QMC::AccessPublic, 0x80000000 | 7),
        // Method 'startupCheck'
        QtMocHelpers::MethodData<QStringList()>(61, 2, QMC::AccessPublic, QMetaType::QStringList),
        // Method 'select'
        QtMocHelpers::MethodData<void(const QString &)>(62, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'launch'
        QtMocHelpers::MethodData<int(const QString &)>(63, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'launchNew'
        QtMocHelpers::MethodData<int(const QString &)>(64, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'launchAll'
        QtMocHelpers::MethodData<int(const QString &)>(65, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 66 },
        }}),
        // Method 'showInFolder'
        QtMocHelpers::MethodData<void(const QString &)>(67, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'launchAsAdmin'
        QtMocHelpers::MethodData<bool(const QString &)>(68, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'launchWithArgs'
        QtMocHelpers::MethodData<bool(const QString &, const QString &)>(69, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 3 }, { QMetaType::QString, 70 },
        }}),
        // Method 'copyPath'
        QtMocHelpers::MethodData<void(const QString &)>(71, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 3 },
        }}),
        // Method 'quickSearch'
        QtMocHelpers::MethodData<QVariantList(const QString &, int) const>(72, 2, QMC::AccessPublic, 0x80000000 | 7, {{
            { QMetaType::QString, 73 }, { QMetaType::Int, 74 },
        }}),
        // Method 'quickSearch'
        QtMocHelpers::MethodData<QVariantList(const QString &) const>(72, 2, QMC::AccessPublic | QMC::MethodCloned, 0x80000000 | 7, {{
            { QMetaType::QString, 73 },
        }}),
        // Method 'showLauncher'
        QtMocHelpers::MethodData<void()>(75, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'browsers'
        QtMocHelpers::MethodData<QVariantList() const>(76, 2, QMC::AccessPublic, 0x80000000 | 7),
        // Method 'addDropped'
        QtMocHelpers::MethodData<int(const QStringList &, int, bool)>(77, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QStringList, 78 }, { QMetaType::Int, 79 }, { QMetaType::Bool, 33 },
        }}),
        // Method 'addDropped'
        QtMocHelpers::MethodData<int(const QStringList &, int)>(77, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Int, {{
            { QMetaType::QStringList, 78 }, { QMetaType::Int, 79 },
        }}),
        // Method 'addDropped'
        QtMocHelpers::MethodData<int(const QStringList &)>(77, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Int, {{
            { QMetaType::QStringList, 78 },
        }}),
        // Method 'moveMember'
        QtMocHelpers::MethodData<void(const QString &, const QString &, int)>(80, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 66 }, { QMetaType::QString, 81 }, { QMetaType::Int, 82 },
        }}),
        // Method 'addDroppedTo'
        QtMocHelpers::MethodData<int(const QString &, const QStringList &)>(83, 2, QMC::AccessPublic, QMetaType::Int, {{
            { QMetaType::QString, 66 }, { QMetaType::QStringList, 78 },
        }}),
        // Method 'createCollection'
        QtMocHelpers::MethodData<QString(const QString &)>(84, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 21 },
        }}),
        // Method 'addToCollection'
        QtMocHelpers::MethodData<void(const QString &, const QStringList &)>(85, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 66 }, { QMetaType::QStringList, 86 },
        }}),
        // Method 'removeFromCollection'
        QtMocHelpers::MethodData<void(const QString &, const QString &)>(87, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 66 }, { QMetaType::QString, 81 },
        }}),
        // Method 'collectionsList'
        QtMocHelpers::MethodData<QVariantList() const>(88, 2, QMC::AccessPublic, 0x80000000 | 7),
        // Method 'addableItems'
        QtMocHelpers::MethodData<QVariantList(const QString &) const>(89, 2, QMC::AccessPublic, 0x80000000 | 7, {{
            { QMetaType::QString, 66 },
        }}),
        // Method 'refreshSoftware'
        QtMocHelpers::MethodData<void()>(90, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'catalogSearch'
        QtMocHelpers::MethodData<QVariantList(const QString &, int) const>(91, 2, QMC::AccessPublic, 0x80000000 | 7, {{
            { QMetaType::QString, 92 }, { QMetaType::Int, 74 },
        }}),
        // Method 'catalogSearch'
        QtMocHelpers::MethodData<QVariantList(const QString &) const>(91, 2, QMC::AccessPublic | QMC::MethodCloned, 0x80000000 | 7, {{
            { QMetaType::QString, 92 },
        }}),
        // Method 'addSoftware'
        QtMocHelpers::MethodData<int(const QVariantList &)>(93, 2, QMC::AccessPublic, QMetaType::Int, {{
            { 0x80000000 | 7, 94 },
        }}),
        // Method 'checkItems'
        QtMocHelpers::MethodData<void()>(95, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'defaultBackupPath'
        QtMocHelpers::MethodData<QString() const>(96, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'backupTo'
        QtMocHelpers::MethodData<bool(const QString &)>(97, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 98 },
        }}),
        // Method 'restoreFrom'
        QtMocHelpers::MethodData<bool(const QString &)>(99, 2, QMC::AccessPublic, QMetaType::Bool, {{
            { QMetaType::QString, 98 },
        }}),
        // Method 'resetSettings'
        QtMocHelpers::MethodData<void()>(100, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'openBackupFolder'
        QtMocHelpers::MethodData<void()>(101, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'clearCache'
        QtMocHelpers::MethodData<void()>(102, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'trimMemory'
        QtMocHelpers::MethodData<void()>(103, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'memoryUsageMB'
        QtMocHelpers::MethodData<double() const>(104, 2, QMC::AccessPublic, QMetaType::Double),
        // Method 'checkForUpdates'
        QtMocHelpers::MethodData<QString() const>(105, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'aboutText'
        QtMocHelpers::MethodData<QString() const>(106, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'dataFolder'
        QtMocHelpers::MethodData<QString() const>(107, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'fileFilter'
        QtMocHelpers::MethodData<QString(int) const>(108, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::Int, 20 },
        }}),
        // Method 'pickFiles'
        QtMocHelpers::MethodData<QStringList(const QString &, const QString &, bool)>(109, 2, QMC::AccessPublic, QMetaType::QStringList, {{
            { QMetaType::QString, 110 }, { QMetaType::QString, 111 }, { QMetaType::Bool, 112 },
        }}),
        // Method 'pickSaveFile'
        QtMocHelpers::MethodData<QString(const QString &, const QString &, const QString &)>(113, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 110 }, { QMetaType::QString, 98 }, { QMetaType::QString, 111 },
        }}),
        // Method 'pickColor'
        QtMocHelpers::MethodData<QString(const QString &)>(114, 2, QMC::AccessPublic, QMetaType::QString, {{
            { QMetaType::QString, 115 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'settings'
        QtMocHelpers::PropertyData<AppSettings*>(116, 0x80000000 | 117, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'theme'
        QtMocHelpers::PropertyData<Theme*>(118, 0x80000000 | 119, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'iconVersion'
        QtMocHelpers::PropertyData<int>(120, QMetaType::Int, QMC::DefaultPropertyFlags, 3),
        // property 'scanning'
        QtMocHelpers::PropertyData<bool>(121, QMetaType::Bool, QMC::DefaultPropertyFlags, 4),
        // property 'version'
        QtMocHelpers::PropertyData<QString>(122, QMetaType::QString, QMC::DefaultPropertyFlags | QMC::Constant),
        // property 'selectedId'
        QtMocHelpers::PropertyData<QString>(123, QMetaType::QString, QMC::DefaultPropertyFlags, 6),
        // property 'catalogCount'
        QtMocHelpers::PropertyData<int>(124, QMetaType::Int, QMC::DefaultPropertyFlags, 5),
        // property 'connectors'
        QtMocHelpers::PropertyData<Connectors*>(125, 0x80000000 | 126, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'sites'
        QtMocHelpers::PropertyData<Sites*>(127, 0x80000000 | 128, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'running'
        QtMocHelpers::PropertyData<RunningApps*>(129, 0x80000000 | 130, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'gamepad'
        QtMocHelpers::PropertyData<Gamepad*>(131, 0x80000000 | 132, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
        // property 'perf'
        QtMocHelpers::PropertyData<PerfMonitor*>(133, 0x80000000 | 134, QMC::DefaultPropertyFlags | QMC::EnumOrFlag | QMC::Constant),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<AppCore, qt_meta_tag_ZN7AppCoreE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject AppCore::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7AppCoreE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7AppCoreE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN7AppCoreE_t>.metaTypes,
    nullptr
} };

void AppCore::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<AppCore *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->processInspection((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QVariantMap>>(_a[2]))); break;
        case 1: _t->guardActions((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[2]))); break;
        case 2: _t->itemsChanged(); break;
        case 3: _t->iconsChanged(); break;
        case 4: _t->scanningChanged(); break;
        case 5: _t->catalogChanged(); break;
        case 6: _t->selectionChanged(); break;
        case 7: _t->launcherRequested(); break;
        case 8: _t->message((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->memoryTrimmed(); break;
        case 10: _t->saveNow(); break;
        case 11: { QString _r = _t->addItem((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 12: { QStringList _r = _t->addFiles((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 13: { bool _r = _t->setItemIcon((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 14: _t->clearItemIcon((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 15: { QString _r = _t->imageFilter();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 16: { bool _r = _t->updateItem((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 17: { bool _r = _t->autoRelocate((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 18: _t->removeItem((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 19: _t->setFavorite((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 20: _t->setItemTags((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 21: { QVariantList _r = _t->tagList((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 22: { QVariantList _r = _t->tagList();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 23: _t->setGuard((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 24: { QVariantMap _r = _t->itemInfo((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 25: _t->inspectProcesses((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 26: { QVariantMap _r = _t->guardInfo((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 27: _t->guardSetAllowed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[4]))); break;
        case 28: _t->guardSetClosedPolicy((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 29: _t->guardSetOpenPolicy((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3]))); break;
        case 30: _t->guardRunClosed((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 31: _t->guardRunOpen((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 32: { QStringList _r = _t->guardRecover();
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 33: { QVariantList _r = _t->startupRows();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 34: { QVariantMap _r = _t->startupDisable((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 35: { QVariantMap _r = _t->startupRestore((*reinterpret_cast<std::add_pointer_t<double>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantMap*>(_a[0]) = std::move(_r); }  break;
        case 36: { QVariantList _r = _t->startupRestoreAll();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 37: { QVariantList _r = _t->startupLedger();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 38: { QStringList _r = _t->startupCheck();
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 39: _t->select((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 40: { int _r = _t->launch((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 41: { int _r = _t->launchNew((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 42: { int _r = _t->launchAll((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 43: _t->showInFolder((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 44: { bool _r = _t->launchAsAdmin((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 45: { bool _r = _t->launchWithArgs((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 46: _t->copyPath((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 47: { QVariantList _r = _t->quickSearch((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 48: { QVariantList _r = _t->quickSearch((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 49: _t->showLauncher(); break;
        case 50: { QVariantList _r = _t->browsers();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 51: { int _r = _t->addDropped((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 52: { int _r = _t->addDropped((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 53: { int _r = _t->addDropped((*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 54: _t->moveMember((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[3]))); break;
        case 55: { int _r = _t->addDroppedTo((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 56: { QString _r = _t->createCollection((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 57: _t->addToCollection((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[2]))); break;
        case 58: _t->removeFromCollection((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 59: { QVariantList _r = _t->collectionsList();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 60: { QVariantList _r = _t->addableItems((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 61: _t->refreshSoftware(); break;
        case 62: { QVariantList _r = _t->catalogSearch((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 63: { QVariantList _r = _t->catalogSearch((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 64: { int _r = _t->addSoftware((*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 65: _t->checkItems(); break;
        case 66: { QString _r = _t->defaultBackupPath();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 67: { bool _r = _t->backupTo((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 68: { bool _r = _t->restoreFrom((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<bool*>(_a[0]) = std::move(_r); }  break;
        case 69: _t->resetSettings(); break;
        case 70: _t->openBackupFolder(); break;
        case 71: _t->clearCache(); break;
        case 72: _t->trimMemory(); break;
        case 73: { double _r = _t->memoryUsageMB();
            if (_a[0]) *reinterpret_cast<double*>(_a[0]) = std::move(_r); }  break;
        case 74: { QString _r = _t->checkForUpdates();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 75: { QString _r = _t->aboutText();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 76: { QString _r = _t->dataFolder();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 77: { QString _r = _t->fileFilter((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 78: { QStringList _r = _t->pickFiles((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[3])));
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 79: { QString _r = _t->pickSaveFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 80: { QString _r = _t->pickColor((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)(const QString & , const QVariantMap & )>(_a, &AppCore::processInspection, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)(const QString & , const QVariantList & )>(_a, &AppCore::guardActions, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::itemsChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::iconsChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::scanningChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::catalogChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::selectionChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::launcherRequested, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)(const QString & )>(_a, &AppCore::message, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (AppCore::*)()>(_a, &AppCore::memoryTrimmed, 9))
            return;
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< AppSettings* >(); break;
        case 7:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Connectors* >(); break;
        case 10:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Gamepad* >(); break;
        case 11:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< PerfMonitor* >(); break;
        case 9:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< RunningApps* >(); break;
        case 8:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Sites* >(); break;
        case 1:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< Theme* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<AppSettings**>(_v) = _t->settings(); break;
        case 1: *reinterpret_cast<Theme**>(_v) = _t->theme(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->iconVersion(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->scanning(); break;
        case 4: *reinterpret_cast<QString*>(_v) = _t->version(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->selectedId(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->catalogCount(); break;
        case 7: *reinterpret_cast<Connectors**>(_v) = _t->connectors(); break;
        case 8: *reinterpret_cast<Sites**>(_v) = _t->sites(); break;
        case 9: *reinterpret_cast<RunningApps**>(_v) = _t->running(); break;
        case 10: *reinterpret_cast<Gamepad**>(_v) = _t->gamepad(); break;
        case 11: *reinterpret_cast<PerfMonitor**>(_v) = _t->perf(); break;
        default: break;
        }
    }
}

const QMetaObject *AppCore::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *AppCore::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN7AppCoreE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int AppCore::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 81)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 81;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 81)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 81;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    return _id;
}

// SIGNAL 0
void AppCore::processInspection(const QString & _t1, const QVariantMap & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void AppCore::guardActions(const QString & _t1, const QVariantList & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void AppCore::itemsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void AppCore::iconsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void AppCore::scanningChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void AppCore::catalogChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void AppCore::selectionChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void AppCore::launcherRequested()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void AppCore::message(const QString & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 8, nullptr, _t1);
}

// SIGNAL 9
void AppCore::memoryTrimmed()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}
QT_WARNING_POP
