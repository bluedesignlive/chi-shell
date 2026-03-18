#ifndef SYSTEMSTATUS_H
#define SYSTEMSTATUS_H

#include <QObject>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QTimer>
#include <QProcess>

class SystemStatus : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int batteryPercent READ batteryPercent NOTIFY batteryChanged)
    Q_PROPERTY(bool batteryCharging READ batteryCharging NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryIcon READ batteryIcon NOTIFY batteryChanged)

    Q_PROPERTY(bool wifiEnabled READ wifiEnabled WRITE setWifiEnabled NOTIFY networkChanged)
    Q_PROPERTY(bool wifiConnected READ wifiConnected NOTIFY networkChanged)
    Q_PROPERTY(QString wifiSSID READ wifiSSID NOTIFY networkChanged)
    Q_PROPERTY(int wifiStrength READ wifiStrength NOTIFY networkChanged)
    Q_PROPERTY(QString networkIcon READ networkIcon NOTIFY networkChanged)

    Q_PROPERTY(bool bluetoothEnabled READ bluetoothEnabled WRITE setBluetoothEnabled NOTIFY bluetoothChanged)
    Q_PROPERTY(QString bluetoothIcon READ bluetoothIcon NOTIFY bluetoothChanged)

    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY audioChanged)
    Q_PROPERTY(bool muted READ muted WRITE setMuted NOTIFY audioChanged)
    Q_PROPERTY(QString audioIcon READ audioIcon NOTIFY audioChanged)

    Q_PROPERTY(double brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

    Q_PROPERTY(bool dndEnabled READ dndEnabled WRITE setDndEnabled NOTIFY dndChanged)

public:
    explicit SystemStatus(QObject *parent = nullptr);

    int batteryPercent() const { return m_batteryPercent; }
    bool batteryCharging() const { return m_batteryCharging; }
    QString batteryIcon() const;

    bool wifiEnabled() const { return m_wifiEnabled; }
    bool wifiConnected() const { return m_wifiConnected; }
    QString wifiSSID() const { return m_wifiSSID; }
    int wifiStrength() const { return m_wifiStrength; }
    QString networkIcon() const;
    void setWifiEnabled(bool enabled);

    bool bluetoothEnabled() const { return m_bluetoothEnabled; }
    QString bluetoothIcon() const;
    void setBluetoothEnabled(bool enabled);

    int volume() const { return m_volume; }
    bool muted() const { return m_muted; }
    QString audioIcon() const;
    void setVolume(int volume);
    void setMuted(bool muted);

    double brightness() const { return m_brightness; }
    void setBrightness(double brightness);

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
    void pollAudioVolume();
    void pollAudioMute();
    void pollBrightness();

    // Async process with automatic backoff on repeated failures
    void asyncCommand(const QString &id, const QString &program,
                      const QStringList &args,
                      std::function<void(const QString &output)> callback);
    bool shouldSkip(const QString &id) const;
    void recordSuccess(const QString &id);
    void recordFailure(const QString &id);

    QTimer m_pollTimer;

    int m_batteryPercent = 100;
    bool m_batteryCharging = false;

    bool m_wifiEnabled = false;
    bool m_wifiConnected = false;
    QString m_wifiSSID;
    int m_wifiStrength = 0;

    bool m_bluetoothEnabled = false;

    int m_volume = 50;
    bool m_muted = false;

    double m_brightness = 0.8;

    bool m_dndEnabled = false;

    // Backoff tracking: command-id → consecutive failures
    QHash<QString, int> m_failCounts;
    int m_pollCycle = 0;

    // Prevent overlapping processes for same command
    QSet<QString> m_runningCommands;
};

#endif // SYSTEMSTATUS_H
