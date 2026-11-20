/****************************************************************************
** Meta object code from reading C++ file 'dockguard.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../scripts/include/dockguard.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dockguard.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN9DockGuardE_t {};
} // unnamed namespace

template <> constexpr inline auto DockGuard::qt_create_metaobjectdata<qt_meta_tag_ZN9DockGuardE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "DockGuard",
        "enabledChanged",
        "",
        "windowChanged",
        "zonesChanged",
        "clickThroughChanged",
        "edgeDwelled",
        "idleAfterChanged",
        "idleChanged",
        "proximityChanged",
        "calmAppsChanged",
        "fullscreenChanged",
        "revealedChanged",
        "dockVisibleChanged",
        "enabled",
        "window",
        "QWindow*",
        "edgeZone",
        "QRectF",
        "dockZone",
        "window2",
        "edgeZone2",
        "dockZone2",
        "clickThrough",
        "idleAfter",
        "idle",
        "proximity",
        "calmApps",
        "fullscreen",
        "revealed",
        "dockVisible"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'enabledChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'windowChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'zonesChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'clickThroughChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'edgeDwelled'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'idleAfterChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'idleChanged'
        QtMocHelpers::SignalData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'proximityChanged'
        QtMocHelpers::SignalData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'calmAppsChanged'
        QtMocHelpers::SignalData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'fullscreenChanged'
        QtMocHelpers::SignalData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'revealedChanged'
        QtMocHelpers::SignalData<void()>(12, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dockVisibleChanged'
        QtMocHelpers::SignalData<void()>(13, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'enabled'
        QtMocHelpers::PropertyData<bool>(14, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 0),
        // property 'window'
        QtMocHelpers::PropertyData<QWindow*>(15, 0x80000000 | 16, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 1),
        // property 'edgeZone'
        QtMocHelpers::PropertyData<QRectF>(17, 0x80000000 | 18, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 2),
        // property 'dockZone'
        QtMocHelpers::PropertyData<QRectF>(19, 0x80000000 | 18, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 2),
        // property 'window2'
        QtMocHelpers::PropertyData<QWindow*>(20, 0x80000000 | 16, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 1),
        // property 'edgeZone2'
        QtMocHelpers::PropertyData<QRectF>(21, 0x80000000 | 18, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 2),
        // property 'dockZone2'
        QtMocHelpers::PropertyData<QRectF>(22, 0x80000000 | 18, QMC::DefaultPropertyFlags | QMC::Writable | QMC::EnumOrFlag | QMC::StdCppSet, 2),
        // property 'clickThrough'
        QtMocHelpers::PropertyData<bool>(23, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'idleAfter'
        QtMocHelpers::PropertyData<int>(24, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
        // property 'idle'
        QtMocHelpers::PropertyData<bool>(25, QMetaType::Bool, QMC::DefaultPropertyFlags, 6),
        // property 'proximity'
        QtMocHelpers::PropertyData<double>(26, QMetaType::Double, QMC::DefaultPropertyFlags, 7),
        // property 'calmApps'
        QtMocHelpers::PropertyData<QStringList>(27, QMetaType::QStringList, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 8),
        // property 'fullscreen'
        QtMocHelpers::PropertyData<bool>(28, QMetaType::Bool, QMC::DefaultPropertyFlags, 9),
        // property 'revealed'
        QtMocHelpers::PropertyData<bool>(29, QMetaType::Bool, QMC::DefaultPropertyFlags, 10),
        // property 'dockVisible'
        QtMocHelpers::PropertyData<bool>(30, QMetaType::Bool, QMC::DefaultPropertyFlags, 11),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DockGuard, qt_meta_tag_ZN9DockGuardE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject DockGuard::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9DockGuardE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9DockGuardE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN9DockGuardE_t>.metaTypes,
    nullptr
} };

void DockGuard::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DockGuard *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->enabledChanged(); break;
        case 1: _t->windowChanged(); break;
        case 2: _t->zonesChanged(); break;
        case 3: _t->clickThroughChanged(); break;
        case 4: _t->edgeDwelled(); break;
        case 5: _t->idleAfterChanged(); break;
        case 6: _t->idleChanged(); break;
        case 7: _t->proximityChanged(); break;
        case 8: _t->calmAppsChanged(); break;
        case 9: _t->fullscreenChanged(); break;
        case 10: _t->revealedChanged(); break;
        case 11: _t->dockVisibleChanged(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::enabledChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::windowChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::zonesChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::clickThroughChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::edgeDwelled, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::idleAfterChanged, 5))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::idleChanged, 6))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::proximityChanged, 7))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::calmAppsChanged, 8))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::fullscreenChanged, 9))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::revealedChanged, 10))
            return;
        if (QtMocHelpers::indexOfMethod<void (DockGuard::*)()>(_a, &DockGuard::dockVisibleChanged, 11))
            return;
    }
    if (_c == QMetaObject::RegisterPropertyMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 4:
        case 1:
            *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< QWindow* >(); break;
        }
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<bool*>(_v) = _t->enabled(); break;
        case 1: *reinterpret_cast<QWindow**>(_v) = _t->window(); break;
        case 2: *reinterpret_cast<QRectF*>(_v) = _t->edgeZone(); break;
        case 3: *reinterpret_cast<QRectF*>(_v) = _t->dockZone(); break;
        case 4: *reinterpret_cast<QWindow**>(_v) = _t->window2(); break;
        case 5: *reinterpret_cast<QRectF*>(_v) = _t->edgeZone2(); break;
        case 6: *reinterpret_cast<QRectF*>(_v) = _t->dockZone2(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->clickThrough(); break;
        case 8: *reinterpret_cast<int*>(_v) = _t->idleAfter(); break;
        case 9: *reinterpret_cast<bool*>(_v) = _t->idle(); break;
        case 10: *reinterpret_cast<double*>(_v) = _t->proximity(); break;
        case 11: *reinterpret_cast<QStringList*>(_v) = _t->calmApps(); break;
        case 12: *reinterpret_cast<bool*>(_v) = _t->fullscreen(); break;
        case 13: *reinterpret_cast<bool*>(_v) = _t->revealed(); break;
        case 14: *reinterpret_cast<bool*>(_v) = _t->dockVisible(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: _t->setEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 1: _t->setWindow(*reinterpret_cast<QWindow**>(_v)); break;
        case 2: _t->setEdgeZone(*reinterpret_cast<QRectF*>(_v)); break;
        case 3: _t->setDockZone(*reinterpret_cast<QRectF*>(_v)); break;
        case 4: _t->setWindow2(*reinterpret_cast<QWindow**>(_v)); break;
        case 5: _t->setEdgeZone2(*reinterpret_cast<QRectF*>(_v)); break;
        case 6: _t->setDockZone2(*reinterpret_cast<QRectF*>(_v)); break;
        case 7: _t->setClickThrough(*reinterpret_cast<bool*>(_v)); break;
        case 8: _t->setIdleAfter(*reinterpret_cast<int*>(_v)); break;
        case 11: _t->setCalmApps(*reinterpret_cast<QStringList*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *DockGuard::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *DockGuard::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN9DockGuardE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int DockGuard::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 12)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 12;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 12)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 12;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 15;
    }
    return _id;
}

// SIGNAL 0
void DockGuard::enabledChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void DockGuard::windowChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void DockGuard::zonesChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void DockGuard::clickThroughChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void DockGuard::edgeDwelled()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void DockGuard::idleAfterChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}

// SIGNAL 6
void DockGuard::idleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 6, nullptr);
}

// SIGNAL 7
void DockGuard::proximityChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 7, nullptr);
}

// SIGNAL 8
void DockGuard::calmAppsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 8, nullptr);
}

// SIGNAL 9
void DockGuard::fullscreenChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 9, nullptr);
}

// SIGNAL 10
void DockGuard::revealedChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 10, nullptr);
}

// SIGNAL 11
void DockGuard::dockVisibleChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 11, nullptr);
}
QT_WARNING_POP
