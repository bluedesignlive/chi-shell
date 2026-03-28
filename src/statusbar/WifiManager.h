#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <QAbstractListModel>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QTimer>
#include <QProcess>
#include <QSet>

class WifiManager : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool available  READ available  CONSTANT)
    Q_PROPERTY(bool enabled    READ enabled    WRITE setEnabled NOTIFY stateChanged)
    Q_PROPERTY(bool connected  READ connected  NOTIFY stateChanged)
    Q_PROPERTY(QString ssid    READ ssid       NOTIFY stateChanged)
    Q_PROPERTY(int strength    READ strength   NOTIFY stateChanged)
    Q_PROPERTY(QString icon    READ icon       NOTIFY stateChanged)
    Q_PROPERTY(bool scanning   READ scanning   NOTIFY scanningChanged)
    Q_PROPERTY(bool connecting READ connecting  NOTIFY connectingChanged)

public:
    enum Role {
        SsidRole = Qt::UserRole + 1,
        StrengthRole,
        SecuredRole,
        ConnectedRole,
        SavedRole,
        SignalIconRole
    };

    explicit WifiManager(QObject *parent = nullptr);

    // properties
    bool available()  const { return m_available; }
    bool enabled()    const { return m_enabled; }
    bool connected()  const { return m_connected; }
    QString ssid()    const { return m_ssid; }
    int strength()    const { return m_strength; }
    QString icon()    const;
    bool scanning()   const { return m_scanning; }
    bool connecting() const { return m_connecting; }

    void setEnabled(bool enabled);

    // model
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // actions
    Q_INVOKABLE void scan();
    Q_INVOKABLE void connectToNetwork(int index, const QString &password = QString());
    Q_INVOKABLE void disconnectFromNetwork();
    Q_INVOKABLE void forgetNetwork(int index);

signals:
    void stateChanged();
    void scanningChanged();
    void connectingChanged();
    void connectResult(bool success, const QString &message);

private slots:
    void onNmPropertiesChanged(const QString &iface,
                                const QVariantMap &changed,
                                const QStringList &invalidated);
    void onDeviceStateChanged(uint newState, uint oldState, uint reason);
    void onAccessPointAdded(const QDBusObjectPath &path);
    void onAccessPointRemoved(const QDBusObjectPath &path);

private:
    struct NetworkInfo {
        QString ssid;
        QString bestApPath;
        int     strength = 0;
        bool    secured  = false;
        bool    connected = false;
        bool    saved    = false;
    };

    void findDevice();
    void connectSignals();
    void readState();
    void readActiveConnection();
    void refreshNetworks();
    void loadSavedConnections();
    QString signalIcon(int str) const;

    bool    m_available  = false;
    bool    m_enabled    = false;
    bool    m_connected  = false;
    bool    m_scanning   = false;
    bool    m_connecting = false;
    QString m_ssid;
    int     m_strength   = 0;

    QString m_devicePath;
    QString m_ifaceName;
    QString m_activeApPath;

    QList<NetworkInfo> m_networks;
    QSet<QString>      m_savedSSIDs;
    QTimer            *m_refreshDebounce = nullptr;
};

#endif
