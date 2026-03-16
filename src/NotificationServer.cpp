#include "NotificationServer.h"
#include <QDBusConnection>
#include <QDBusMetaType>
#include <QDebug>

NotificationServer::NotificationServer(QObject *parent)
    : QAbstractListModel(parent)
{
}

bool NotificationServer::registerOnBus()
{
    auto bus = QDBusConnection::sessionBus();

    if (!bus.registerService("org.freedesktop.Notifications")) {
        qWarning() << "NotificationServer: could not register service."
                    << "Another notification daemon may be running.";
        return false;
    }

    if (!bus.registerObject("/org/freedesktop/Notifications", this,
            QDBusConnection::ExportAllSlots |
            QDBusConnection::ExportAllSignals)) {
        qWarning() << "NotificationServer: could not register object";
        return false;
    }

    qDebug() << "NotificationServer: registered on session bus";
    return true;
}

int NotificationServer::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_notifications.size();
}

QVariant NotificationServer::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_notifications.size())
        return {};

    const auto &n = m_notifications.at(index.row());

    switch (role) {
    case IdRole:        return n.id;
    case AppNameRole:   return n.appName;
    case AppIconRole:   return n.appIcon;
    case SummaryRole:   return n.summary;
    case BodyRole:      return n.body;
    case ActionsRole:   return n.actions;
    case TimestampRole: return n.timestamp;
    }

    return {};
}

QHash<int, QByteArray> NotificationServer::roleNames() const
{
    return {
        { IdRole,        "notifId"   },
        { AppNameRole,   "appName"   },
        { AppIconRole,   "appIcon"   },
        { SummaryRole,   "summary"   },
        { BodyRole,      "body"      },
        { ActionsRole,   "actions"   },
        { TimestampRole, "timestamp" }
    };
}

// ─── D-Bus Methods ──────────────────────────────────────

QStringList NotificationServer::GetCapabilities()
{
    return { "body", "actions", "icon-static", "persistence" };
}

uint NotificationServer::Notify(const QString &app_name, uint replaces_id,
                                 const QString &app_icon, const QString &summary,
                                 const QString &body, const QStringList &actions,
                                 const QVariantMap &hints, int expire_timeout)
{
    Q_UNUSED(hints)

    NotificationData notif;
    notif.appName      = app_name;
    notif.appIcon      = app_icon;
    notif.summary      = summary;
    notif.body         = body;
    notif.actions      = actions;
    notif.timestamp    = QDateTime::currentDateTime();
    notif.expireTimeout = expire_timeout;

    // replace existing notification if replaces_id is set
    if (replaces_id > 0) {
        for (int i = 0; i < m_notifications.size(); ++i) {
            if (m_notifications[i].id == replaces_id) {
                notif.id = replaces_id;
                m_notifications[i] = notif;
                emit dataChanged(index(i), index(i));
                emit notificationPosted(notif.id, summary, body, app_icon);
                scheduleExpiry(notif.id, expire_timeout);
                return notif.id;
            }
        }
    }

    // new notification — insert at top
    notif.id = nextId();
    beginInsertRows({}, 0, 0);
    m_notifications.prepend(notif);
    endInsertRows();
    emit countChanged();
    emit notificationPosted(notif.id, summary, body, app_icon);

    scheduleExpiry(notif.id, expire_timeout);

    return notif.id;
}

void NotificationServer::CloseNotification(uint id)
{
    dismiss(id);
    emit NotificationClosed(id, 3); // 3 = closed by CloseNotification call
}

QString NotificationServer::GetServerInformation(QString &vendor,
                                                   QString &version,
                                                   QString &spec_version)
{
    vendor       = "chi";
    version      = "0.1.0";
    spec_version = "1.2";
    return "Chi Shell";
}

// ─── QML Callable ───────────────────────────────────────

void NotificationServer::dismiss(uint id)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == id) {
            beginRemoveRows({}, i, i);
            m_notifications.removeAt(i);
            endRemoveRows();
            emit countChanged();
            emit NotificationClosed(id, 2); // 2 = dismissed by user
            return;
        }
    }
}

void NotificationServer::dismissAll()
{
    if (m_notifications.isEmpty())
        return;

    beginResetModel();
    for (const auto &n : m_notifications) {
        emit NotificationClosed(n.id, 2);
    }
    m_notifications.clear();
    endResetModel();
    emit countChanged();
}

void NotificationServer::invokeAction(uint id, const QString &actionKey)
{
    emit ActionInvoked(id, actionKey);
}

// ─── Internal ───────────────────────────────────────────

uint NotificationServer::nextId()
{
    return m_nextId++;
}

void NotificationServer::scheduleExpiry(uint id, int timeout)
{
    if (timeout <= 0)
        return; // -1 = never expire, 0 = server decides (we keep it)

    QTimer::singleShot(timeout, this, [this, id]() {
        dismiss(id);
    });
}
