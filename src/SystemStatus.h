#ifndef SYSTEMSTATUS_H
#define SYSTEMSTATUS_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QTimer>

// ═══════════════════════════════════════════════════════
// SystemStatus — D-Bus client for system services
//
// Reads battery, network, bluetooth, and audio state.
// Exposes properties for QML binding.
// ═══════════════════════════════════════════════════════

class SystemStatus : public QObject
{
    Q_OBJECT

    // battery
    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(bool batteryCharging READ batteryCharging NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryIcon READ batteryIcon NOTIFY batteryChanged)

    // network
    Q_PROPERTY(bool wifiEnabled READ wifiEnabled WRITE setWifiEnabled NOTIFY networkChanged)
    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY networkChanged)
    Q_PROPERTY(QString wifiSSID READ wifiSSID NOTIFY networkChanged)
    Q_PROPERTY(int wifiStrength READ wifiStrength NOTIFY networkChanged)
    Q_PROPERTY(QString networkIcon READ networkIcon NOTIFY networkChanged)

    // bluetooth
    Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled WRITE setBluetoothEnabled NOTIFY bluetoothChanged)
    Q_PROPERTY(QString bluetoothIcon READ bluetoothIcon NOTIFY bluetoothChanged)

    // audio
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY audioChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY audioChanged)
    Q_PROPERTY(QString audioIcon READ audioIcon NOTIFY audioChanged)

    // display
    Q_PROPERTY(double brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

    // general
    Q_PROPERTY(bool dndEnabled READ dndEnabled WRITE setDndEnabled NOTIFY dndChanged)

public:
    explicit SystemStatus(QObject *parent = nullptr);

    // battery
    int batteryPercent() const { return m_batteryPercent; }
    bool batteryCharging() const { return m_batteryCharging; }
    QString batteryIcon() const;

    // network
    bool wifiEnabled() const { return m_wifiEnabled; }
    bool wifiConnected() const { return m_wifiConnected; }
    QString wifiSSID() const { return m_wifiSSID; }
    int wifiStrength() const { return m_wifiStrength; }
    QString networkIcon() const;
    void setWifiEnabled(bool enabled);

    // bluetooth
    bool bluetoothEnabled() const { return m_bluetoothEnabled; }
    QString bluetoothIcon() const;
    void setBluetoothEnabled(bool enabled);

    // audio
    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    QString audioIcon() const;
    void setVolume(int volume);
    void setMuted(bool muted);

    // display
    double brightness() const { return m_brightness; }
    void setBrightness(double brightness);

    // dnd
    bool dndEnabled() const { return m_dndEnabled; }
    void setDndEnabled(bool enabled);

    Q_INVOKABLE void refresh();

signals:
    void batteryChanged();
    void networkChanged();
    void bluetoothChanged();
    void audioChanged();
    void brightnessChanged();
    void dndChanged();

private:
    void pollBattery();
    void pollNetwork();
    void pollBluetooth();
    void pollAudio();
    void pollBrightness();

    QTimer m_pollTimer;

    // battery
    int m_batteryPercent = 100;
    bool m_batteryCharging = false;

    // network
    bool m_wifiEnabled = false;
    bool m_wifiConnected = false;
    QString m_wifiSSID;
    int m_wifiStrength = 0;

    // bluetooth
    bool m_bluetoothEnabled = false;

    // audio
    int m_volume = 50;
    bool m_muted = false;

    // display
    double m_brightness = 0.8;

    // dnd
    bool m_dndEnabled = false;
};

#endif // SYSTEMSTATUS_H
