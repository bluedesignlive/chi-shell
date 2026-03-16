/****************************************************************************
** Meta object code from reading C++ file 'NotificationServer.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.10.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../src/NotificationServer.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'NotificationServer.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN18NotificationServerE_t {};
} // unnamed namespace

template <> constexpr inline auto NotificationServer::qt_create_metaobjectdata<qt_meta_tag_ZN18NotificationServerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "NotificationServer",
        "D-Bus Interface",
        "org.freedesktop.Notifications",
        "NotificationClosed",
        "",
        "id",
        "reason",
        "ActionInvoked",
        "action_key",
        "countChanged",
        "notificationPosted",
        "summary",
        "body",
        "appIcon",
        "GetCapabilities",
        "Notify",
        "app_name",
        "replaces_id",
        "app_icon",
        "actions",
        "QVariantMap",
        "hints",
        "expire_timeout",
        "CloseNotification",
        "GetServerInformation",
        "QString&",
        "vendor",
        "version",
        "spec_version",
        "dismiss",
        "dismissAll",
        "invokeAction",
        "actionKey",
        "count"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'NotificationClosed'
        QtMocHelpers::SignalData<void(uint, uint)>(3, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 5 }, { QMetaType::UInt, 6 },
        }}),
        // Signal 'ActionInvoked'
        QtMocHelpers::SignalData<void(uint, const QString &)>(7, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 5 }, { QMetaType::QString, 8 },
        }}),
        // Signal 'countChanged'
        QtMocHelpers::SignalData<void()>(9, 4, QMC::AccessPublic, QMetaType::Void),
        // Signal 'notificationPosted'
        QtMocHelpers::SignalData<void(uint, const QString &, const QString &, const QString &)>(10, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 5 }, { QMetaType::QString, 11 }, { QMetaType::QString, 12 }, { QMetaType::QString, 13 },
        }}),
        // Slot 'GetCapabilities'
        QtMocHelpers::SlotData<QStringList()>(14, 4, QMC::AccessPublic, QMetaType::QStringList),
        // Slot 'Notify'
        QtMocHelpers::SlotData<uint(const QString &, uint, const QString &, const QString &, const QString &, const QStringList &, const QVariantMap &, int)>(15, 4, QMC::AccessPublic, QMetaType::UInt, {{
            { QMetaType::QString, 16 }, { QMetaType::UInt, 17 }, { QMetaType::QString, 18 }, { QMetaType::QString, 11 },
            { QMetaType::QString, 12 }, { QMetaType::QStringList, 19 }, { 0x80000000 | 20, 21 }, { QMetaType::Int, 22 },
        }}),
        // Slot 'CloseNotification'
        QtMocHelpers::SlotData<void(uint)>(23, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 5 },
        }}),
        // Slot 'GetServerInformation'
        QtMocHelpers::SlotData<QString(QString &, QString &, QString &)>(24, 4, QMC::AccessPublic, QMetaType::QString, {{
            { 0x80000000 | 25, 26 }, { 0x80000000 | 25, 27 }, { 0x80000000 | 25, 28 },
        }}),
        // Method 'dismiss'
        QtMocHelpers::MethodData<void(uint)>(29, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 5 },
        }}),
        // Method 'dismissAll'
        QtMocHelpers::MethodData<void()>(30, 4, QMC::AccessPublic, QMetaType::Void),
        // Method 'invokeAction'
        QtMocHelpers::MethodData<void(uint, const QString &)>(31, 4, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::UInt, 5 }, { QMetaType::QString, 32 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
        // property 'count'
        QtMocHelpers::PropertyData<int>(33, QMetaType::Int, QMC::DefaultPropertyFlags, 2),
    };
    QtMocHelpers::UintData qt_enums {
    };
    QtMocHelpers::UintData qt_constructors {};
    QtMocHelpers::ClassInfos qt_classinfo({
            {    1,    2 },
    });
    return QtMocHelpers::metaObjectData<NotificationServer, qt_meta_tag_ZN18NotificationServerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums, qt_constructors, qt_classinfo);
}
Q_CONSTINIT const QMetaObject NotificationServer::staticMetaObject = { {
    QMetaObject::SuperData::link<QAbstractListModel::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18NotificationServerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18NotificationServerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN18NotificationServerE_t>.metaTypes,
    nullptr
} };

void NotificationServer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<NotificationServer *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->NotificationClosed((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint>>(_a[2]))); break;
        case 1: _t->ActionInvoked((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 2: _t->countChanged(); break;
        case 3: _t->notificationPosted((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4]))); break;
        case 4: { QStringList _r = _t->GetCapabilities();
            if (_a[0]) *reinterpret_cast<QStringList*>(_a[0]) = std::move(_r); }  break;
        case 5: { uint _r = _t->Notify((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[3])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[4])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[5])),(*reinterpret_cast<std::add_pointer_t<QStringList>>(_a[6])),(*reinterpret_cast<std::add_pointer_t<QVariantMap>>(_a[7])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[8])));
            if (_a[0]) *reinterpret_cast<uint*>(_a[0]) = std::move(_r); }  break;
        case 6: _t->CloseNotification((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1]))); break;
        case 7: { QString _r = _t->GetServerInformation((*reinterpret_cast<std::add_pointer_t<QString&>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString&>>(_a[2])),(*reinterpret_cast<std::add_pointer_t<QString&>>(_a[3])));
            if (_a[0]) *reinterpret_cast<QString*>(_a[0]) = std::move(_r); }  break;
        case 8: _t->dismiss((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1]))); break;
        case 9: _t->dismissAll(); break;
        case 10: _t->invokeAction((*reinterpret_cast<std::add_pointer_t<uint>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (NotificationServer::*)(uint , uint )>(_a, &NotificationServer::NotificationClosed, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (NotificationServer::*)(uint , const QString & )>(_a, &NotificationServer::ActionInvoked, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (NotificationServer::*)()>(_a, &NotificationServer::countChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (NotificationServer::*)(uint , const QString & , const QString & , const QString & )>(_a, &NotificationServer::notificationPosted, 3))
            return;
    }
    if (_c == QMetaObject::ReadProperty) {
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast<int*>(_v) = _t->count(); break;
        default: break;
        }
    }
}

const QMetaObject *NotificationServer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *NotificationServer::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN18NotificationServerE_t>.strings))
        return static_cast<void*>(this);
    return QAbstractListModel::qt_metacast(_clname);
}

int NotificationServer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QAbstractListModel::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 11)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 11;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 11)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 11;
    }
    if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 1;
    }
    return _id;
}

// SIGNAL 0
void NotificationServer::NotificationClosed(uint _t1, uint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void NotificationServer::ActionInvoked(uint _t1, const QString & _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void NotificationServer::countChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void NotificationServer::notificationPosted(uint _t1, const QString & _t2, const QString & _t3, const QString & _t4)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2, _t3, _t4);
}
QT_WARNING_POP
