/****************************************************************************
** Meta object code from reading C++ file 'SystemStatus.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/SystemStatus.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'SystemStatus.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.10.2. It"
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
struct qt_meta_tag_ZN12SystemStatusE_t {};
} // unnamed namespace

template <> constexpr inline auto SystemStatus::qt_create_metaobjectdata<qt_meta_tag_ZN12SystemStatusE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "SystemStatus",
        "batteryChanged",
        "",
        "networkChanged",
        "bluetoothChanged",
        "audioChanged",
        "brightnessChanged",
        "dndChanged",
        "refresh",
        "batteryPercent",
        "batteryCharging",
        "batteryIcon",
        "wifiEnabled",
        "wifiConnected",
        "wifiSSID",
        "wifiStrength",
        "networkIcon",
        "bluetoothEnabled",
        "bluetoothIcon",
        "volume",
        "muted",
        "audioIcon",
        "brightness",
        "dndEnabled"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'batteryChanged'
        QtMocHelpers::SignalData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'networkChanged'
        QtMocHelpers::SignalData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'bluetoothChanged'
        QtMocHelpers::SignalData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'audioChanged'
        QtMocHelpers::SignalData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'brightnessChanged'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Signal 'dndChanged'
        QtMocHelpers::SignalData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Method 'refresh'
        QtMocHelpers::MethodData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'batteryPercent'
        QtMocHelpers::PropertyData<int>(9, QMetaType::Int, QMC::DefaultPropertyFlags, 0),
        // property 'batteryCharging'
        QtMocHelpers::PropertyData<bool>(10, QMetaType::Bool, QMC::DefaultPropertyFlags, 0),
        // property 'batteryIcon'
        QtMocHelpers::PropertyData<QString>(11, QMetaType::QString, QMC::DefaultPropertyFlags, 0),
        // property 'wifiEnabled'
        QtMocHelpers::PropertyData<bool>(12, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 1),
        // property 'wifiConnected'
        QtMocHelpers::PropertyData<bool>(13, QMetaType::Bool, QMC::DefaultPropertyFlags, 1),
        // property 'wifiSSID'
        QtMocHelpers::PropertyData<QString>(14, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'wifiStrength'
        QtMocHelpers::PropertyData<int>(15, QMetaType::Int, QMC::DefaultPropertyFlags, 1),
        // property 'networkIcon'
        QtMocHelpers::PropertyData<QString>(16, QMetaType::QString, QMC::DefaultPropertyFlags, 1),
        // property 'bluetoothEnabled'
        QtMocHelpers::PropertyData<bool>(17, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 2),
        // property 'bluetoothIcon'
        QtMocHelpers::PropertyData<QString>(18, QMetaType::QString, QMC::DefaultPropertyFlags, 2),
        // property 'volume'
        QtMocHelpers::PropertyData<int>(19, QMetaType::Int, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'muted'
        QtMocHelpers::PropertyData<bool>(20, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 3),
        // property 'audioIcon'
        QtMocHelpers::PropertyData<QString>(21, QMetaType::QString, QMC::DefaultPropertyFlags, 3),
        // property 'brightness'
        QtMocHelpers::PropertyData<double>(22, QMetaType::Double, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 4),
        // property 'dndEnabled'
        QtMocHelpers::PropertyData<bool>(23, QMetaType::Bool, QMC::DefaultPropertyFlags | QMC::Writable | QMC::StdCppSet, 5),
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SystemStatus, qt_meta_tag_ZN12SystemStatusE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject SystemStatus::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SystemStatusE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SystemStatusE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN12SystemStatusE_t>.metaTypes,
    nullptr
} };

void SystemStatus::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SystemStatus *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->batteryChanged(); break;
        case 1: _t->networkChanged(); break;
        case 2: _t->bluetoothChanged(); break;
        case 3: _t->audioChanged(); break;
        case 4: _t->brightnessChanged(); break;
        case 5: _t->dndChanged(); break;
        case 6: _t->refresh(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SystemStatus::*)()>(_a, &SystemStatus::batteryChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SystemStatus::*)()>(_a, &SystemStatus::networkChanged, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SystemStatus::*)()>(_a, &SystemStatus::bluetoothChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SystemStatus::*)()>(_a, &SystemStatus::audioChanged, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (SystemStatus::*)()>(_a, &SystemStatus::brightnessChanged, 4))
            return;
        if (QtMocHelpers::indexOfMethod<void (SystemStatus::*)()>(_a, &SystemStatus::dndChanged, 5))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->batteryPercent(); break;
        case 1: *reinterpret_cast<bool*>(_v) = _t->batteryCharging(); break;
        case 2: *reinterpret_cast<QString*>(_v) = _t->batteryIcon(); break;
        case 3: *reinterpret_cast<bool*>(_v) = _t->wifiEnabled(); break;
        case 4: *reinterpret_cast<bool*>(_v) = _t->wifiConnected(); break;
        case 5: *reinterpret_cast<QString*>(_v) = _t->wifiSSID(); break;
        case 6: *reinterpret_cast<int*>(_v) = _t->wifiStrength(); break;
        case 7: *reinterpret_cast<QString*>(_v) = _t->networkIcon(); break;
        case 8: *reinterpret_cast<bool*>(_v) = _t->bluetoothEnabled(); break;
        case 9: *reinterpret_cast<QString*>(_v) = _t->bluetoothIcon(); break;
        case 10: *reinterpret_cast<int*>(_v) = _t->volume(); break;
        case 11: *reinterpret_cast<bool*>(_v) = _t->muted(); break;
        case 12: *reinterpret_cast<QString*>(_v) = _t->audioIcon(); break;
        case 13: *reinterpret_cast<double*>(_v) = _t->brightness(); break;
        case 14: *reinterpret_cast<bool*>(_v) = _t->dndEnabled(); break;
        default: break;
        }
    }
    if (_c == QMetaObject::WriteProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 3: _t->setWifiEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 8: _t->setBluetoothEnabled(*reinterpret_cast<bool*>(_v)); break;
        case 10: _t->setVolume(*reinterpret_cast<int*>(_v)); break;
        case 11: _t->setMuted(*reinterpret_cast<bool*>(_v)); break;
        case 13: _t->setBrightness(*reinterpret_cast<double*>(_v)); break;
        case 14: _t->setDndEnabled(*reinterpret_cast<bool*>(_v)); break;
        default: break;
        }
    }
}

const QMetaObject *SystemStatus::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *SystemStatus::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN12SystemStatusE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int SystemStatus::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
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
void SystemStatus::batteryChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void SystemStatus::networkChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void SystemStatus::bluetoothChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void SystemStatus::audioChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void SystemStatus::brightnessChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}

// SIGNAL 5
void SystemStatus::dndChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 5, nullptr);
}
QT_WARNING_POP
