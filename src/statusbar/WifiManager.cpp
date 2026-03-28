#include "WifiManager.h"

#include <QDBusInterface>
#include <QDBusArgument>
#include <QDBusReply>
#include <QDebug>
#include <algorithm>

WifiManager::WifiManager(QObject *parent)
    : QAbstractListModel(parent)
{
    m_refreshDebounce = new QTimer(this);
    m_refreshDebounce->setSingleShot(true);
    m_refreshDebounce->setInterval(500);
    connect(m_refreshDebounce, &QTimer::timeout,
            this, &WifiManager::refreshNetworks);

    findDevice();
    if (!m_available) return;

    connectSignals();
    readState();
    readActiveConnection();
    loadSavedConnections();   // async → calls refreshNetworks when done
}

// ═════════════════════════════════════════════════════
// Device discovery
// ═════════════════════════════════════════════════════

void WifiManager::findDevice()
{
    QDBusInterface nm("org.freedesktop.NetworkManager",
                      "/org/freedesktop/NetworkManager",
                      "org.freedesktop.NetworkManager",
                      QDBusConnection::systemBus());
    if (!nm.isValid()) return;

    QDBusMessage reply = nm.call("GetDevices");
    if (reply.type() != QDBusMessage::ReplyMessage
        || reply.arguments().isEmpty()) return;

    const QDBusArgument &arg =
        reply.arguments().at(0).value<QDBusArgument>();
    QList<QDBusObjectPath> devices;
    arg >> devices;

    for (const auto &devPath : devices) {
        QDBusInterface dev("org.freedesktop.NetworkManager",
                           devPath.path(),
                           "org.freedesktop.NetworkManager.Device",
                           QDBusConnection::systemBus());
        if (dev.property("DeviceType").toUInt() == 2) {
            m_available  = true;
            m_devicePath = devPath.path();
            m_ifaceName  = dev.property("Interface").toString();
            break;
        }
    }
    qDebug() << "WifiManager:" << (m_available
        ? "found " + m_ifaceName + " at " + m_devicePath
        : "no wifi device");
}

// ═════════════════════════════════════════════════════
// D-Bus signal connections (real-time)
// ═════════════════════════════════════════════════════

void WifiManager::connectSignals()
{
    auto bus = QDBusConnection::systemBus();

    // NM global property changes (WirelessEnabled, State)
    bus.connect("org.freedesktop.NetworkManager",
                "/org/freedesktop/NetworkManager",
                "org.freedesktop.DBus.Properties",
                "PropertiesChanged",
                this,
                SLOT(onNmPropertiesChanged(QString,QVariantMap,QStringList)));

    // Device state changes (connecting, connected, disconnected)
    bus.connect("org.freedesktop.NetworkManager",
                m_devicePath,
                "org.freedesktop.NetworkManager.Device",
                "StateChanged",
                this,
                SLOT(onDeviceStateChanged(uint,uint,uint)));

    // Access points appear / disappear
    bus.connect("org.freedesktop.NetworkManager",
                m_devicePath,
                "org.freedesktop.NetworkManager.Device.Wireless",
                "AccessPointAdded",
                this,
                SLOT(onAccessPointAdded(QDBusObjectPath)));

    bus.connect("org.freedesktop.NetworkManager",
                m_devicePath,
                "org.freedesktop.NetworkManager.Device.Wireless",
                "AccessPointRemoved",
                this,
                SLOT(onAccessPointRemoved(QDBusObjectPath)));
}

// ═════════════════════════════════════════════════════
// Signal handlers
// ═════════════════════════════════════════════════════

void WifiManager::onNmPropertiesChanged(
    const QString &iface, const QVariantMap &changed, const QStringList &)
{
    if (iface != "org.freedesktop.NetworkManager") return;

    if (changed.contains("WirelessEnabled")) {
        m_enabled = changed["WirelessEnabled"].toBool();
        if (!m_enabled) {
            m_connected = false;
            m_ssid.clear();
            m_strength = 0;
            beginResetModel();
            m_networks.clear();
            endResetModel();
        } else {
            QTimer::singleShot(1500, this, [this]{ scan(); });
        }
        emit stateChanged();
    }
}

void WifiManager::onDeviceStateChanged(uint newState, uint, uint)
{
    // 100 = activated
    bool nowConnected = (newState == 100);
    if (nowConnected != m_connected) {
        m_connected = nowConnected;
        readActiveConnection();
        m_refreshDebounce->start();
        emit stateChanged();
    }
}

void WifiManager::onAccessPointAdded(const QDBusObjectPath &)
{
    m_refreshDebounce->start();
}

void WifiManager::onAccessPointRemoved(const QDBusObjectPath &)
{
    m_refreshDebounce->start();
}

// ═════════════════════════════════════════════════════
// State reading
// ═════════════════════════════════════════════════════

void WifiManager::readState()
{
    QDBusInterface nm("org.freedesktop.NetworkManager",
                      "/org/freedesktop/NetworkManager",
                      "org.freedesktop.NetworkManager",
                      QDBusConnection::systemBus());
    m_enabled = nm.property("WirelessEnabled").toBool();

    QDBusInterface dev("org.freedesktop.NetworkManager",
                       m_devicePath,
                       "org.freedesktop.NetworkManager.Device",
                       QDBusConnection::systemBus());
    m_connected = (dev.property("State").toUInt() == 100);

    emit stateChanged();
}

void WifiManager::readActiveConnection()
{
    m_activeApPath.clear();
    if (!m_connected || !m_enabled) {
        if (!m_ssid.isEmpty()) {
            m_ssid.clear();
            m_strength = 0;
            emit stateChanged();
        }
        return;
    }

    QDBusInterface wireless("org.freedesktop.NetworkManager",
                            m_devicePath,
                            "org.freedesktop.NetworkManager.Device.Wireless",
                            QDBusConnection::systemBus());

    QDBusObjectPath apPath =
        wireless.property("ActiveAccessPoint").value<QDBusObjectPath>();
    m_activeApPath = apPath.path();

    if (m_activeApPath.isEmpty() || m_activeApPath == "/") return;

    QDBusInterface ap("org.freedesktop.NetworkManager",
                      m_activeApPath,
                      "org.freedesktop.NetworkManager.AccessPoint",
                      QDBusConnection::systemBus());
    if (!ap.isValid()) return;

    QString ssid = QString::fromUtf8(ap.property("Ssid").toByteArray());
    int str      = ap.property("Strength").toUInt();

    if (ssid != m_ssid || str != m_strength) {
        m_ssid     = ssid;
        m_strength = str;
        emit stateChanged();
    }
}

// ═════════════════════════════════════════════════════
// Network list (model)
// ═════════════════════════════════════════════════════

int WifiManager::rowCount(const QModelIndex &) const
{
    return m_networks.size();
}

QVariant WifiManager::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= m_networks.size())
        return {};

    const auto &n = m_networks[index.row()];
    switch (role) {
    case SsidRole:       return n.ssid;
    case StrengthRole:   return n.strength;
    case SecuredRole:    return n.secured;
    case ConnectedRole:  return n.connected;
    case SavedRole:      return n.saved;
    case SignalIconRole: return signalIcon(n.strength);
    }
    return {};
}

QHash<int, QByteArray> WifiManager::roleNames() const
{
    return {
        { SsidRole,       "ssid" },
        { StrengthRole,   "strength" },
        { SecuredRole,    "secured" },
        { ConnectedRole,  "connected" },
        { SavedRole,      "saved" },
        { SignalIconRole, "signalIcon" }
    };
}

void WifiManager::refreshNetworks()
{
    QDBusInterface wireless("org.freedesktop.NetworkManager",
                            m_devicePath,
                            "org.freedesktop.NetworkManager.Device.Wireless",
                            QDBusConnection::systemBus());

    QDBusMessage reply = wireless.call("GetAllAccessPoints");
    if (reply.type() != QDBusMessage::ReplyMessage
        || reply.arguments().isEmpty()) return;

    const QDBusArgument &arg =
        reply.arguments().at(0).value<QDBusArgument>();
    QList<QDBusObjectPath> apPaths;
    arg >> apPaths;

    // deduplicate by SSID, keep strongest
    QMap<QString, NetworkInfo> bySSID;

    for (const auto &apPath : apPaths) {
        QDBusInterface ap("org.freedesktop.NetworkManager",
                          apPath.path(),
                          "org.freedesktop.NetworkManager.AccessPoint",
                          QDBusConnection::systemBus());
        if (!ap.isValid()) continue;

        QString ssid = QString::fromUtf8(ap.property("Ssid").toByteArray());
        if (ssid.isEmpty()) continue;

        int  str      = ap.property("Strength").toUInt();
        uint flags    = ap.property("Flags").toUInt();
        uint wpa      = ap.property("WpaFlags").toUInt();
        uint rsn      = ap.property("RsnFlags").toUInt();
        bool secured  = (flags & 0x1) || wpa || rsn;
        bool isActive = (apPath.path() == m_activeApPath);

        auto it = bySSID.find(ssid);
        if (it == bySSID.end() || str > it->strength) {
            NetworkInfo info;
            info.ssid       = ssid;
            info.bestApPath = apPath.path();
            info.strength   = str;
            info.secured    = secured;
            info.connected  = isActive;
            info.saved      = m_savedSSIDs.contains(ssid);
            bySSID[ssid]    = info;
        } else if (isActive) {
            it->connected = true;
        }
    }

    QList<NetworkInfo> list = bySSID.values();
    std::sort(list.begin(), list.end(),
              [](const NetworkInfo &a, const NetworkInfo &b) {
        if (a.connected != b.connected) return a.connected;
        if (a.saved != b.saved)         return a.saved;
        return a.strength > b.strength;
    });

    beginResetModel();
    m_networks = list;
    endResetModel();
}

void WifiManager::loadSavedConnections()
{
    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        m_savedSSIDs.clear();
        if (exitCode == 0) {
            for (const auto &line : proc->readAllStandardOutput().split('\n')) {
                auto parts = line.split(':');
                if (parts.size() >= 2
                    && parts[1].trimmed() == "802-11-wireless")
                    m_savedSSIDs.insert(parts[0].trimmed());
            }
        }
        refreshNetworks();
        proc->deleteLater();
    });
    proc->start("nmcli", {"-t", "-f", "NAME,TYPE", "connection", "show"});
}

// ═════════════════════════════════════════════════════
// Actions
// ═════════════════════════════════════════════════════

void WifiManager::scan()
{
    if (!m_available || !m_enabled || m_scanning) return;

    m_scanning = true;
    emit scanningChanged();

    QDBusInterface wireless("org.freedesktop.NetworkManager",
                            m_devicePath,
                            "org.freedesktop.NetworkManager.Device.Wireless",
                            QDBusConnection::systemBus());

    QVariantMap opts;
    wireless.call("RequestScan", opts);

    QTimer::singleShot(3000, this, [this]() {
        refreshNetworks();
        m_scanning = false;
        emit scanningChanged();
    });
}

void WifiManager::connectToNetwork(int index, const QString &password)
{
    if (index < 0 || index >= m_networks.size()) return;

    const auto &net = m_networks[index];
    m_connecting = true;
    emit connectingChanged();

    QStringList args = {"device", "wifi", "connect", net.ssid};
    if (!password.isEmpty())
        args << "password" << password;
    if (!m_ifaceName.isEmpty())
        args << "ifname" << m_ifaceName;

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int exitCode, QProcess::ExitStatus) {
        m_connecting = false;
        emit connectingChanged();

        if (exitCode == 0) {
            emit connectResult(true, "Connected");
            loadSavedConnections();
        } else {
            QString err = proc->readAllStandardError().trimmed();
            if (err.isEmpty()) err = "Connection failed";
            emit connectResult(false, err);
        }
        proc->deleteLater();
    });
    proc->start("nmcli", args);
}

void WifiManager::disconnectFromNetwork()
{
    if (m_ifaceName.isEmpty()) return;

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [proc]() { proc->deleteLater(); });
    proc->start("nmcli", {"device", "disconnect", m_ifaceName});
}

void WifiManager::forgetNetwork(int index)
{
    if (index < 0 || index >= m_networks.size()) return;

    auto *proc = new QProcess(this);
    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc](int, QProcess::ExitStatus) {
        loadSavedConnections();
        proc->deleteLater();
    });
    proc->start("nmcli", {"connection", "delete", "id",
                           m_networks[index].ssid});
}

void WifiManager::setEnabled(bool enabled)
{
    QDBusInterface nm("org.freedesktop.NetworkManager",
                      "/org/freedesktop/NetworkManager",
                      "org.freedesktop.DBus.Properties",
                      QDBusConnection::systemBus());
    nm.call("Set", "org.freedesktop.NetworkManager",
            "WirelessEnabled",
            QVariant::fromValue(QDBusVariant(enabled)));
    m_enabled = enabled;
    emit stateChanged();
}

// ═════════════════════════════════════════════════════
// Helpers
// ═════════════════════════════════════════════════════

QString WifiManager::icon() const
{
    if (!m_enabled)   return "wifi_off";
    if (!m_connected) return "wifi_find";
    if (m_strength > 75) return "wifi";
    if (m_strength > 50) return "wifi_2_bar";
    return "wifi_1_bar";
}

QString WifiManager::signalIcon(int str) const
{
    if (str > 75) return "wifi";
    if (str > 50) return "wifi_2_bar";
    if (str > 25) return "wifi_1_bar";
    return "wifi_1_bar";
}
