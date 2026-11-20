/****************************************************************************
** Meta object code from reading C++ file 'connectors.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../scripts/include/connectors.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'connectors.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN10ConnectorsE_t {};
} // unnamed namespace

template <> constexpr inline auto Connectors::qt_create_metaobjectdata<qt_meta_tag_ZN10ConnectorsE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Connectors",
        "bridgeChanged",
        "",
        "receivedChanged",
        "busyChanged",
        "githubReady",
        "QVariantList",
        "entries",
        "error",
        "regenerateToken",
        "copyPairingCode",
        "extensionFolder",
        "openExtensionFolder",
        "installedBrowsers",
        "browserBookmarks",
        "browser",
        "bookmarksFromFile",
        "path",
        "vscodeRecent",
        "fetchGithub",
        "user",
        "addEntries",
        "addEntriesToNewCollection",
        "name",
        "bridgeEnabled",
        "bridgeRunning",
        "bridgePort",
        "pairingCode",
        "received",
        "busy"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'bridgeChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'receivedChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'busyChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'githubReady'
        QtMocHelpers::SignalData<void(const QVariantList &, const QString &)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 }, { QMetaType::QString, 8 },
        }}),
        // Method 'regenerateToken'
        QtMocHelpers::MethodData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'copyPairingCode'
        QtMocHelpers::MethodData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'extensionFolder'
        QtMocHelpers::MethodData<QString() const>(11, 2, QMC::AccessPublic, QMetaType::QString),
        // Method 'openExtensionFolder'
        QtMocHelpers::MethodData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'installedBrowsers'
        QtMocHelpers::MethodData<QStringList() const>(13, 2, QMC::AccessPublic, QMetaType::QStringList),
        // Method 'browserBookmarks'
        QtMocHelpers::MethodData<QVariantList(const QString &) const>(14, 2, QMC::AccessPublic, 0x80000000 | 6, {{
            { QMetaType::QString, 15 },
        }}),
        // Method 'bookmarksFromFile'
        QtMocHelpers::MethodData<QVariantList(const QString &) const>(16, 2, QMC::AccessPublic, 0x80000000 | 6, {{
            { QMetaType::QString, 17 },
        }}),
        // Method 'vscodeRecent'
        QtMocHelpers::MethodData<QVariantList() const>(18, 2, QMC::AccessPublic, 0x80000000 | 6),
        // Method 'fetchGithub'
        QtMocHelpers::MethodData<void(const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::QString, 20 },
        }}),
        // Method 'addEntries'
        QtMocHelpers::MethodData<int(const QVariantList &)>(21, 2, QMC::AccessPublic, QMetaType::Int, {{
            { 0x80000000 | 6, 7 },
        }}),
        // Method 'addEntriesToNewCollection'
        QtMocHelpers::MethodData<int(const QVariantList &, const QString &)>(22, 2, QMC::AccessPublic, QMetaType::Int, {{
            { 0x80000000 | 6, 7 }, { QMetaType::QString, 23 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'bridgeEnabled'
        QtMocHelpers::PropertyData<bool>(24, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'bridgeRunning'
        QtMocHelpers::PropertyData<bool>(25, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'bridgePort'
        QtMocHelpers::PropertyData<int>(26, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'pairingCode'
        QtMocHelpers::PropertyData<QString>(27, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'received'
        QtMocHelpers::PropertyData<int>(28, QMetaType::Int, QMC::DefaultPropertyFlags, 1),
        // property 'busy'
        QtMocHelpers::PropertyData<bool>(29, QMetaType::Bool, QMC::DefaultPropertyFlags, 2),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Connectors, qt_meta_tag_ZN10ConnectorsE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Connectors::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ConnectorsE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ConnectorsE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN10ConnectorsE_t>.metaTypes,
    nullptr
} };

void Connectors::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Connectors *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->bridgeChanged(); break;
        case 1: _t->receivedChanged(); break;
        case 2: _t->busyChanged(); break;
        case 3: _t->githubReady((*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 4: _t->regenerateToken(); break;
        case 5: _t->copyPairingCode(); break;
        case 6: { QString _r = _t->extensionFolder();
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->openExtensionFolder(); break;
        case 8: { QStringList _r = _t->installedBrowsers();
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 9: { QVariantList _r = _t->browserBookmarks((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 10: { QVariantList _r = _t->bookmarksFromFile((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 11: { QVariantList _r = _t->vscodeRecent();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->fetchGithub((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 13: { int _r = _t->addEntries((*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[1])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        case 14: { int _r = _t->addEntriesToNewCollection((*reinterpret_cast<std::add_pointer_t<QVariantList>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])));
            if (_a[0]) *reinterpret_cast<int*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Connectors::*)()>(_a, &Connectors::bridgeChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Connectors::*)()>(_a, &Connectors::receivedChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Connectors::*)()>(_a, &Connectors::busyChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Connectors::*)(const QVariantList & , const QString & )>(_a, &Connectors::githubReady, 3))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->bridgeEnabled(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->bridgeRunning(); break;
        case 2: *reinterpret_cast<int*>(_v) = _t->bridgePort(); break;
        case 3: *reinterpret_cast<QString*>(_v) = _t->pairingCode(); break;
        case 4: *reinterpret_cast<int*>(_v) = _t->received(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->busy(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setBridgeEnabled(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *Connectors::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Connectors::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN10ConnectorsE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Connectors::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 15)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 15)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 15;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void Connectors::bridgeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void Connectors::receivedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void Connectors::busyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void Connectors::githubReady(const QVariantList & _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}
QT_WARNING_POP
