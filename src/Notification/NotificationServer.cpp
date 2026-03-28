#include "NotificationServer.h"
#include <QDBusConnection>
#include <QDebug>
#include <QIcon>
#include <QRegularExpression>

static QString sanitizeCandidate(QString s)
{
    s = s.trimmed();
    if (s.endsWith(".desktop"))
        s.chop(8);
    return s;
}

static QString normalizedAppNameCandidate(QString s)
{
    s = s.trimmed().toLower();
    s.replace(QRegularExpression("[^a-z0-9]+"), "-");
    while (s.contains("--"))
        s.replace("--", "-");
    if (s.startsWith("-"))
        s.remove(0, 1);
    if (s.endsWith("-"))
        s.chop(1);
    return s;
}

NotificationServer::NotificationServer(QObject *parent)
    : QAbstractListModel(parent)
{
}

bool NotificationServer::registerOnBus()
{
    auto bus = QDBusConnection::sessionBus();

    if (!bus.registerService("org.freedesktop.Notifications")) {
        qWarning() << "[notifications] could not register service; another daemon may be running";
        return false;
    }

    if (!bus.registerObject("/org/freedesktop/Notifications", this,
            QDBusConnection::ExportScriptableSlots | QDBusConnection::ExportScriptableSignals)) {
        qWarning() << "[notifications] could not register object";
        return false;
    }

    qDebug() << "[notifications] registered on session bus";
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
    case IdRole:           return n.id;
    case AppNameRole:      return n.appName;
    case AppIconRole:      return n.appIcon;
    case SummaryRole:      return n.summary;
    case BodyRole:         return n.body;
    case ActionsRole:      return n.actions;
    case TimestampRole:    return n.timestamp;
    case UrgencyRole:      return n.urgency;
    case CategoryRole:     return n.category;
    case DesktopEntryRole: return n.desktopEntry;
    case ImagePathRole:    return n.imagePath;
    case ResolvedIconRole: return resolvedIcon(n);
    case TransientRole:    return n.transient;
    case ResidentRole:     return n.resident;
    case IsGroupedRole:    return isGrouped(index.row());
    case HintsRole:        return n.hints;
    }

    return {};
}

QHash<int, QByteArray> NotificationServer::roleNames() const
{
    return {
        { IdRole,           "notifId" },
        { AppNameRole,      "appName" },
        { AppIconRole,      "appIcon" },
        { SummaryRole,      "summary" },
        { BodyRole,         "body" },
        { ActionsRole,      "actions" },
        { TimestampRole,    "timestamp" },
        { UrgencyRole,      "urgency" },
        { CategoryRole,     "category" },
        { DesktopEntryRole, "desktopEntry" },
        { ImagePathRole,    "imagePath" },
        { ResolvedIconRole, "resolvedIcon" },
        { TransientRole,    "isTransient" },
        { ResidentRole,     "isResident" },
        { IsGroupedRole,    "isGrouped" },
        { HintsRole,        "hints" }
    };
}

QString NotificationServer::resolvedIcon(const NotificationData &notif) const
{
    QStringList candidates;

    auto addCandidate = [&](const QString &candidate) {
        const QString cleaned = sanitizeCandidate(candidate);
        if (!cleaned.isEmpty() && !candidates.contains(cleaned))
            candidates << cleaned;
    };

    addCandidate(notif.appIcon);
    addCandidate(notif.desktopEntry);
    addCandidate(notif.appName);
    addCandidate(normalizedAppNameCandidate(notif.appName));

    for (const QString &candidate : candidates) {
        if (candidate.startsWith("file://") || candidate.startsWith('/'))
            return candidate;

        if (QIcon::hasThemeIcon(candidate) || !QIcon::fromTheme(candidate).isNull())
            return candidate;
    }

    if (!candidates.isEmpty())
        return candidates.first();

    return "dialog-information";
}

QStringList NotificationServer::trayIcons() const
{
    QStringList icons;
    QSet<QString> seen;

    for (const auto &n : m_notifications) {
        if (n.urgency == 0)
            continue;

        const QString icon = resolvedIcon(n);
        if (!icon.isEmpty() && !seen.contains(icon)) {
            seen.insert(icon);
            icons << icon;
        }
    }

    return icons;
}

void NotificationServer::setDoNotDisturb(bool dnd)
{
    if (m_dnd == dnd)
        return;
    m_dnd = dnd;
    emit doNotDisturbChanged();
}

QStringList NotificationServer::GetCapabilities()
{
    return {
        "body",
        "body-markup",
        "body-hyperlinks",
        "body-images",
        "actions",
        "action-icons",
        "icon-static",
        "persistence",
        "sound"
    };
}

uint NotificationServer::Notify(const QString &app_name, uint replaces_id,
                                const QString &app_icon, const QString &summary,
                                const QString &body, const QStringList &actions,
                                const QVariantMap &hints, int expire_timeout)
{
    NotificationData notif;
    notif.appName       = app_name;
    notif.appIcon       = app_icon;
    notif.summary       = summary;
    notif.body          = body;
    notif.actions       = actions;
    notif.hints         = hints;
    notif.timestamp     = QDateTime::currentDateTime();
    notif.expireTimeout = expire_timeout;

    parseHints(notif, hints);

    if (replaces_id > 0) {
        for (int i = 0; i < m_notifications.size(); ++i) {
            if (m_notifications[i].id == replaces_id) {
                notif.id = replaces_id;
                m_notifications[i] = notif;
                emit dataChanged(index(i), index(i));
                emit trayIconsChanged();

                if (!m_dnd || notif.urgency == 2) {
                    emit notificationPosted(notif.id, notif.appName,
                                            notif.summary, notif.body,
                                            resolvedIcon(notif), notif.imagePath, notif.urgency,
                                            notif.actions);
                }

                scheduleExpiry(notif.id, expire_timeout);
                return notif.id;
            }
        }
    }

    notif.id = nextId();
    beginInsertRows({}, 0, 0);
    m_notifications.prepend(notif);
    endInsertRows();
    emit countChanged();
    emit trayIconsChanged();

    if (!m_dnd || notif.urgency == 2) {
        emit notificationPosted(notif.id, notif.appName,
                                notif.summary, notif.body,
                                resolvedIcon(notif), notif.imagePath, notif.urgency,
                                notif.actions);
    }

    scheduleExpiry(notif.id, expire_timeout);
    return notif.id;
}

void NotificationServer::CloseNotification(uint id)
{
    dismiss(id);
    emit NotificationClosed(id, 3);
}

QString NotificationServer::GetServerInformation(QString &vendor,
                                                 QString &version,
                                                 QString &spec_version)
{
    vendor = "chi";
    version = "0.2.0";
    spec_version = "1.2";
    return "Chi Shell";
}

void NotificationServer::dismiss(uint id)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == id) {
            beginRemoveRows({}, i, i);
            m_notifications.removeAt(i);
            endRemoveRows();
            emit countChanged();
            emit trayIconsChanged();
            emit NotificationClosed(id, 2);
            return;
        }
    }
}

void NotificationServer::dismissAll()
{
    if (m_notifications.isEmpty())
        return;

    beginResetModel();
    for (const auto &n : m_notifications)
        emit NotificationClosed(n.id, 2);
    m_notifications.clear();
    endResetModel();

    emit countChanged();
    emit trayIconsChanged();
}

void NotificationServer::dismissApp(const QString &appName)
{
    for (int i = m_notifications.size() - 1; i >= 0; --i) {
        if (m_notifications[i].appName == appName) {
            const uint id = m_notifications[i].id;
            beginRemoveRows({}, i, i);
            m_notifications.removeAt(i);
            endRemoveRows();
            emit NotificationClosed(id, 2);
        }
    }

    emit countChanged();
    emit trayIconsChanged();
}

void NotificationServer::invokeAction(uint id, const QString &actionKey)
{
    emit ActionInvoked(id, actionKey);

    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == id && !m_notifications[i].resident) {
            dismiss(id);
            return;
        }
    }
}

void NotificationServer::snooze(uint id, int durationMs)
{
    for (int i = 0; i < m_notifications.size(); ++i) {
        if (m_notifications[i].id == id) {
            NotificationData copy = m_notifications[i];

            beginRemoveRows({}, i, i);
            m_notifications.removeAt(i);
            endRemoveRows();
            emit countChanged();
            emit trayIconsChanged();

            QTimer::singleShot(durationMs, this, [this, copy]() mutable {
                copy.id = nextId();
                copy.timestamp = QDateTime::currentDateTime();

                beginInsertRows({}, 0, 0);
                m_notifications.prepend(copy);
                endInsertRows();
                emit countChanged();
                emit trayIconsChanged();

                emit notificationPosted(copy.id, copy.appName,
                                        copy.summary, copy.body,
                                        resolvedIcon(copy), copy.imagePath, copy.urgency,
                                        copy.actions);
            });
            return;
        }
    }
}

void NotificationServer::parseHints(NotificationData &notif, const QVariantMap &hints)
{
    if (hints.contains("urgency"))
        notif.urgency = hints.value("urgency").toUInt();

    if (hints.contains("category"))
        notif.category = hints.value("category").toString();

    if (hints.contains("desktop-entry"))
        notif.desktopEntry = sanitizeCandidate(hints.value("desktop-entry").toString());

    if (hints.contains("image-path"))
        notif.imagePath = hints.value("image-path").toString();
    else if (hints.contains("image_path"))
        notif.imagePath = hints.value("image_path").toString();

    if (hints.contains("sound-file"))
        notif.soundFile = hints.value("sound-file").toString();

    if (hints.contains("sound-name"))
        notif.soundName = hints.value("sound-name").toString();

    if (hints.contains("suppress-sound"))
        notif.suppressSound = hints.value("suppress-sound").toBool();

    if (hints.contains("transient"))
        notif.transient = hints.value("transient").toBool();

    if (hints.contains("resident"))
        notif.resident = hints.value("resident").toBool();

    if (hints.contains("action-icons"))
        notif.actionIcons = hints.value("action-icons").toBool();
}

uint NotificationServer::nextId()
{
    return m_nextId++;
}

void NotificationServer::scheduleExpiry(uint id, int timeout)
{
    if (timeout < 0)
        return;

    if (timeout == 0) {
        for (const auto &n : m_notifications) {
            if (n.id == id) {
                switch (n.urgency) {
                case 0: timeout = 3000; break;
                case 2: return;
                default: timeout = 5000; break;
                }
                break;
            }
        }
        if (timeout == 0)
            timeout = 5000;
    }

    QTimer::singleShot(timeout, this, [this, id]() {
        for (const auto &n : m_notifications) {
            if (n.id == id && n.transient) {
                dismiss(id);
                return;
            }
        }
    });
}

bool NotificationServer::isGrouped(int index) const
{
    if (index <= 0)
        return false;
    return m_notifications.at(index).appName == m_notifications.at(index - 1).appName;
}
