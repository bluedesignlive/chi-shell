#ifndef NOTIFICATIONSERVER_H
#define NOTIFICATIONSERVER_H

#include <QObject>
#include <QAbstractListModel>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDateTime>
#include <QTimer>

// ═══════════════════════════════════════════════════════
// NotificationData — single notification
// ═══════════════════════════════════════════════════════

struct NotificationData
{
    uint id = 0;
    QString appName;
    QString appIcon;
    QString summary;
    QString body;
    QStringList actions;
    QDateTime timestamp;
    int expireTimeout = -1;
};

// ═══════════════════════════════════════════════════════
// NotificationServer — org.freedesktop.Notifications
//
// The shell IS the notification server. This class owns
// the D-Bus name and implements the full spec. Provides
// a list model for NotificationCenter.qml to bind to.
// ═══════════════════════════════════════════════════════

class NotificationServer : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        AppNameRole,
        AppIconRole,
        SummaryRole,
        BodyRole,
        ActionsRole,
        TimestampRole
    };

    explicit NotificationServer(QObject *parent = nullptr);
    bool registerOnBus();

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_notifications.size(); }

    // QML-callable
    Q_INVOKABLE void dismiss(uint id);
    Q_INVOKABLE void dismissAll();
    Q_INVOKABLE void invokeAction(uint id, const QString &actionKey);

public slots:
    // D-Bus methods (called by other apps via D-Bus)
    QStringList GetCapabilities();
    uint Notify(const QString &app_name, uint replaces_id,
                const QString &app_icon, const QString &summary,
                const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expire_timeout);
    void CloseNotification(uint id);
    QString GetServerInformation(QString &vendor, QString &version,
                                 QString &spec_version);

signals:
    // D-Bus signals
    void NotificationClosed(uint id, uint reason);
    void ActionInvoked(uint id, const QString &action_key);

    // QML signals
    void countChanged();
    void notificationPosted(uint id, const QString &summary,
                            const QString &body, const QString &appIcon);

private:
    uint nextId();
    void scheduleExpiry(uint id, int timeout);

    QVector<NotificationData> m_notifications;
    uint m_nextId = 1;
};

#endif // NOTIFICATIONSERVER_H
