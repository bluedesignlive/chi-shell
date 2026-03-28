#ifndef NOTIFICATIONSERVER_H
#define NOTIFICATIONSERVER_H

#include <QObject>
#include <QAbstractListModel>
#include <QDateTime>
#include <QTimer>
#include <QVariantMap>

struct NotificationData
{
    uint id = 0;
    QString appName;
    QString appIcon;
    QString summary;
    QString body;
    QStringList actions;
    QVariantMap hints;
    QDateTime timestamp;
    int expireTimeout = -1;

    uint urgency = 1;           // 0=low, 1=normal, 2=critical
    QString category;
    QString desktopEntry;
    QString imagePath;
    QString soundFile;
    QString soundName;
    bool transient = false;
    bool resident = false;
    bool actionIcons = false;
    bool suppressSound = false;
};

class NotificationServer : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QStringList trayIcons READ trayIcons NOTIFY trayIconsChanged)
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb WRITE setDoNotDisturb NOTIFY doNotDisturbChanged)
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.Notifications")

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        AppNameRole,
        AppIconRole,
        SummaryRole,
        BodyRole,
        ActionsRole,
        TimestampRole,
        UrgencyRole,
        CategoryRole,
        DesktopEntryRole,
        ImagePathRole,
        ResolvedIconRole,
        TransientRole,
        ResidentRole,
        IsGroupedRole,
        HintsRole
    };

    explicit NotificationServer(QObject *parent = nullptr);
    bool registerOnBus();

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_notifications.size(); }

    QStringList trayIcons() const;

    bool doNotDisturb() const { return m_dnd; }
    void setDoNotDisturb(bool dnd);

    Q_INVOKABLE void dismiss(uint id);
    Q_INVOKABLE void dismissAll();
    Q_INVOKABLE void dismissApp(const QString &appName);
    Q_INVOKABLE void invokeAction(uint id, const QString &actionKey);
    Q_INVOKABLE void snooze(uint id, int durationMs = 300000);

public slots:
    Q_SCRIPTABLE Q_SCRIPTABLE QStringList GetCapabilities();
    Q_SCRIPTABLE Q_SCRIPTABLE uint Notify(const QString &app_name, uint replaces_id,
                const QString &app_icon, const QString &summary,
                const QString &body, const QStringList &actions,
                const QVariantMap &hints, int expire_timeout);
    Q_SCRIPTABLE Q_SCRIPTABLE void CloseNotification(uint id);
    Q_SCRIPTABLE Q_SCRIPTABLE QString GetServerInformation(QString &vendor, QString &version,
                                 QString &spec_version);

signals:
    Q_SCRIPTABLE Q_SCRIPTABLE void NotificationClosed(uint id, uint reason);
    Q_SCRIPTABLE Q_SCRIPTABLE void ActionInvoked(uint id, const QString &action_key);

    void countChanged();
    void trayIconsChanged();
    void doNotDisturbChanged();

    void notificationPosted(uint id, const QString &appName,
                            const QString &summary, const QString &body,
                            const QString &appIcon, const QString &imagePath, uint urgency,
                            const QStringList &actions);

private:
    void parseHints(NotificationData &notif, const QVariantMap &hints);
    QString resolvedIcon(const NotificationData &notif) const;
    uint nextId();
    void scheduleExpiry(uint id, int timeout);
    bool isGrouped(int index) const;

    QVector<NotificationData> m_notifications;
    uint m_nextId = 1;
    bool m_dnd = false;
};

#endif
