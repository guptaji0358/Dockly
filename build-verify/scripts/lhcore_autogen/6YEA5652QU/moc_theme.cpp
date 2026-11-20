/****************************************************************************
** Meta object code from reading C++ file 'theme.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../scripts/include/theme.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'theme.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN5ThemeE_t {};
} // unnamed namespace

template <> constexpr inline auto Theme::qt_create_metaobjectdata<qt_meta_tag_ZN5ThemeE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "Theme",
        "changed",
        "",
        "categoryColor",
        "QColor",
        "category",
        "cardColor",
        "hover",
        "accentOf",
        "palettes",
        "QVariantList",
        "style",
        "dark",
        "dock",
        "radius",
        "borderWidth",
        "glow",
        "tinted",
        "hardShadow",
        "headingFont",
        "accent",
        "bg",
        "surface",
        "card",
        "cardHover",
        "text",
        "subtext",
        "border",
        "sidebar",
        "sidebarText",
        "warning",
        "missing",
        "favorite"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'changed'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'categoryColor'
        QtMocHelpers::MethodData<QColor(int) const>(3, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::Int, 5 },
        }}),
        // Method 'cardColor'
        QtMocHelpers::MethodData<QColor(int, bool) const>(6, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::Int, 5 }, { QMetaType::Bool, 7 },
        }}),
        // Method 'accentOf'
        QtMocHelpers::MethodData<QColor(int) const>(8, 2, QMC::AccessPublic, 0x80000000 | 4, {{
            { QMetaType::Int, 5 },
        }}),
        // Method 'palettes'
        QtMocHelpers::MethodData<QVariantList() const>(9, 2, QMC::AccessPublic, 0x80000000 | 10),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'style'
        QtMocHelpers::PropertyData<QString>(11, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'dark'
        QtMocHelpers::PropertyData<bool>(12, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'dock'
        QtMocHelpers::PropertyData<bool>(13, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'radius'
        QtMocHelpers::PropertyData<int>(14, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'borderWidth'
        QtMocHelpers::PropertyData<int>(15, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'glow'
        QtMocHelpers::PropertyData<bool>(16, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'tinted'
        QtMocHelpers::PropertyData<bool>(17, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'hardShadow'
        QtMocHelpers::PropertyData<bool>(18, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'headingFont'
        QtMocHelpers::PropertyData<QString>(19, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'accent'
        QtMocHelpers::PropertyData<QColor>(20, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'bg'
        QtMocHelpers::PropertyData<QColor>(21, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'surface'
        QtMocHelpers::PropertyData<QColor>(22, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'card'
        QtMocHelpers::PropertyData<QColor>(23, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'cardHover'
        QtMocHelpers::PropertyData<QColor>(24, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'text'
        QtMocHelpers::PropertyData<QColor>(25, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'subtext'
        QtMocHelpers::PropertyData<QColor>(26, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'border'
        QtMocHelpers::PropertyData<QColor>(27, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'sidebar'
        QtMocHelpers::PropertyData<QColor>(28, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'sidebarText'
        QtMocHelpers::PropertyData<QColor>(29, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'warning'
        QtMocHelpers::PropertyData<QColor>(30, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'missing'
        QtMocHelpers::PropertyData<QColor>(31, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
        // property 'favorite'
        QtMocHelpers::PropertyData<QColor>(32, 0x80000000 | 4, QMC::DefaultPropertyFlags | QMC::EnumOrFlag, 0),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Theme, qt_meta_tag_ZN5ThemeE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject Theme::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN5ThemeE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN5ThemeE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN5ThemeE_t>.metaTypes,
    nullptr
} };

void Theme::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Theme *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->changed(); break;
        case 1: { QColor _r = _t->categoryColor((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QColor*>(_a[0]) = std::move(_r); }  break;
        case 2: { QColor _r = _t->cardColor((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2])));
            if (_a[0]) *reinterpret_cast<QColor*>(_a[0]) = std::move(_r); }  break;
        case 3: { QColor _r = _t->accentOf((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast<QColor*>(_a[0]) = std::move(_r); }  break;
        case 4: { QVariantList _r = _t->palettes();
            if (_a[0]) *reinterpret_cast<QVariantList*>(_a[0]) = std::move(_r); }  break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Theme::*)()>(_a, &Theme::changed, 0))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<QString*>(_v) = _t->style(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->dark(); break;
        case 2: *reinterpret_cast<bool*>(_v) = _t->dock(); break;
        case 3: *reinterpret_cast<int*>(_v) = _t->radius(); break;
        case 4: *reinterpret_cast<int*>(_v) = _t->borderWidth(); break;
        case 5: *reinterpret_cast<bool*>(_v) = _t->glow(); break;
        case 6: *reinterpret_cast<bool*>(_v) = _t->tinted(); break;
        case 7: *reinterpret_cast<bool*>(_v) = _t->hardShadow(); break;
        case 8: *reinterpret_cast<QString*>(_v) = _t->headingFont(); break;
        case 9: *reinterpret_cast<QColor*>(_v) = _t->accent(); break;
        case 10: *reinterpret_cast<QColor*>(_v) = _t->bg(); break;
        case 11: *reinterpret_cast<QColor*>(_v) = _t->surface(); break;
        case 12: *reinterpret_cast<QColor*>(_v) = _t->card(); break;
        case 13: *reinterpret_cast<QColor*>(_v) = _t->cardHover(); break;
        case 14: *reinterpret_cast<QColor*>(_v) = _t->text(); break;
        case 15: *reinterpret_cast<QColor*>(_v) = _t->subtext(); break;
        case 16: *reinterpret_cast<QColor*>(_v) = _t->border(); break;
        case 17: *reinterpret_cast<QColor*>(_v) = _t->sidebar(); break;
        case 18: *reinterpret_cast<QColor*>(_v) = _t->sidebarText(); break;
        case 19: *reinterpret_cast<QColor*>(_v) = _t->warning(); break;
        case 20: *reinterpret_cast<QColor*>(_v) = _t->missing(); break;
        case 21: *reinterpret_cast<QColor*>(_v) = _t->favorite(); break;
        default: break;
        }
    }
}

const QMetaObject *Theme::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *Theme::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN5ThemeE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int Theme::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 22;
    }
    return _id;
}

// SIGNAL 0
void Theme::changed()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}
QT_WARNING_POP
