/****************************************************************************
** Meta object code from reading C++ file 'lhtypes.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.12.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../scripts/include/lhtypes.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'lhtypes.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN2LHE_t {};
} // unnamed namespace

template <> constexpr inline auto LH::qt_create_metaobjectdata<qt_meta_tag_ZN2LHE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "LH",
        "Category",
        "AllCategories",
        "Software",
        "Document",
        "Entertainment",
        "Web",
        "Collection",
        "Status",
        "Normal",
        "Warning",
        "Missing",
        "Mode",
        "ModeCategory",
        "ModeFavorites",
        "ModeRecent",
        "ModeMembers",
        "LaunchResult",
        "Launched",
        "TargetMissing",
        "LaunchFailed"
    };

    QtMocHelpers::UintData qt_methods {
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
        // enum 'Category'
        QtMocHelpers::EnumData<Category>(1, 1, QMC::EnumFlags{}).add({
            {    2, Category::AllCategories },
            {    3, Category::Software },
            {    4, Category::Document },
            {    5, Category::Entertainment },
            {    6, Category::Web },
            {    7, Category::Collection },
        }),
        // enum 'Status'
        QtMocHelpers::EnumData<Status>(8, 8, QMC::EnumFlags{}).add({
            {    9, Status::Normal },
            {   10, Status::Warning },
            {   11, Status::Missing },
        }),
        // enum 'Mode'
        QtMocHelpers::EnumData<Mode>(12, 12, QMC::EnumFlags{}).add({
            {   13, Mode::ModeCategory },
            {   14, Mode::ModeFavorites },
            {   15, Mode::ModeRecent },
            {   16, Mode::ModeMembers },
        }),
        // enum 'LaunchResult'
        QtMocHelpers::EnumData<LaunchResult>(17, 17, QMC::EnumFlags{}).add({
            {   18, LaunchResult::Launched },
            {   19, LaunchResult::TargetMissing },
            {   20, LaunchResult::LaunchFailed },
        }),
    };
    return QtMocHelpers::metaObjectData<void, qt_meta_tag_ZN2LHE_t>(QMC::PropertyAccessInStaticMetaCall, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}

static constexpr auto qt_staticMetaObjectContent_ZN2LHE =
    LH::qt_create_metaobjectdata<qt_meta_tag_ZN2LHE_t>();
static constexpr auto qt_staticMetaObjectStaticContent_ZN2LHE =
    qt_staticMetaObjectContent_ZN2LHE.staticData;
static constexpr auto qt_staticMetaObjectRelocatingContent_ZN2LHE =
    qt_staticMetaObjectContent_ZN2LHE.relocatingData;

Q_CONSTINIT const QMetaObject LH::staticMetaObject = { {
    nullptr,
    qt_staticMetaObjectStaticContent_ZN2LHE.stringdata,
    qt_staticMetaObjectStaticContent_ZN2LHE.data,
    nullptr,
    nullptr,
    qt_staticMetaObjectRelocatingContent_ZN2LHE.metaTypes,
    nullptr
} };

QT_WARNING_POP
