#ifndef SYSTEMSTATUS_H
#define SYSTEMSTATUS_H

#include <QObject>
#include <QTimer>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusObjectPath>
#include <QSet>
#include <QHash>
#include <functional>

class SystemStatus : public QObject
{
    Q_OBJECT

    // ── hardware capability flags (QML hides UI when false) ──
    Q_PROPERTY(bool hasBattery    READ hasBattery    CONSTANT)
    Q_PROPERTY(bool hasBacklight  READ hasBacklight  CONSTANT)
    Q_PROPERTY(bool hasWifi       READ hasWifi       CONSTANT)
    Q_PROPERTY(bool hasBluetooth  READ hasBluetooth  CONSTANT)

    // ── battery ──
    Q_PROPERTY(int     batteryPercent  READ batteryPercent  NOTIFY batteryChanged)
    Q_PROPERTY(bool    batteryCharging READ batteryCharging NOTIFY batteryChanged)
    Q_PROPERTY(QString batteryIcon     READ batteryIcon     NOTIFY batteryChanged)

    // ── network ──
    Q_PROPERTY(bool    wifiEnabled   READ wifiEnabled   WRITE setWifiEnabled   NOTIFY networkChanged)
    Q_PROPERTY(bool    wifiConnected READ wifiConnected NOTIFY networkChanged)
    Q_PROPERTY(QString wifiSSID      READ wifiSSID      NOTIFY networkChanged)
    Q_PROPERTY(int     wifiStrength  READ wifiStrength  NOTIFY networkChanged)
    Q_PROPERTY(QString networkIcon   READ networkIcon   NOTIFY networkChanged)

    // ── bluetooth ──
    Q_PROPERTY(bool    bluetoothEnabled READ bluetoothEnabled WRITE setBluetoothEnabled NOTIFY bluetoothChanged)
    Q_PROPERTY(QString bluetoothIcon    READ bluetoothIcon    NOTIFY bluetoothChanged)

    // ── audio ──
    Q_PROPERTY(int     volume    READ volume    WRITE setVolume NOTIFY audioChanged)
    Q_PROPERTY(bool    muted     READ muted     WRITE setMuted  NOTIFY audioChanged)
    Q_PROPERTY(QString audioIcon READ audioIcon NOTIFY audioChanged)

    // ── display ──
    Q_PROPERTY(double brightness READ brightness WRITE setBrightness NOTIFY brightnessChanged)

    // ── general ──
    Q_PROPERTY(bool dndEnabled READ dndEnabled WRITE setDndEnabled NOTIFY dndChanged)
    Q_PROPERTY(bool micMuted READ micMuted WRITE setMicMuted NOTIFY micChanged)

public:
    explicit SystemStatus(QObject *parent = nullptr);

    // hardware flags
    bool hasBattery()   const { return m_hasBattery; }
    bool hasBacklight() const { return m_hasBacklight; }
    bool hasWifi()      const { return m_hasWifi; }
    bool hasBluetooth() const { return m_hasBluetooth; }

    // battery
    int     batteryPercent()  const { return m_batteryPercent; }
    bool    batteryCharging() const { return m_batteryCharging; }
    QString batteryIcon() const;

    // network
    bool    wifiEnabled()   const { return m_wifiEnabled; }
    bool    wifiConnected() const { return m_wifiConnected; }
    QString wifiSSID()      const { return m_wifiSSID; }
    int     wifiStrength()  const { return m_wifiStrength; }
    QString networkIcon() const;
    void setWifiEnabled(bool enabled);

    // bluetooth
    bool    bluetoothEnabled() const { return m_bluetoothEnabled; }
    QString bluetoothIcon() const;
    void setBluetoothEnabled(bool enabled);

    // audio
    int     volume()    const { return m_volume; }
    bool    muted()     const { return m_muted; }
    QString audioIcon() const;
    void setVolume(int volume);
    void setMuted(bool muted);

    // display
    double brightness() const { return m_brightness; }
    void setBrightness(double brightness);

    // dnd
    bool dndEnabled() const { return m_dndEnabled; }
    void setDndEnabled(bool enabled);

    bool micMuted() const { return m_micMuted; }
    void setMicMuted(bool muted);

    Q_INVOKABLE void refresh();

signals:
    void batteryChanged();
    void networkChanged();
    void bluetoothChanged();
    void audioChanged();
    void brightnessChanged();
    void dndChanged();
    void micChanged();

private:
    void detectHardware();
    void pollBattery();
    void pollNetwork();
    void pollBluetooth();
    void pollAudioVolume();
    void pollAudioMute();
    void pollBrightness();

    // async process with backoff
    void asyncCommand(const QString &id, const QString &program,
                      const QStringList &args,
                      std::function<void(const QString &output)> callback);
    bool shouldSkip(const QString &id) const;
    void recordSuccess(const QString &id);
    void recordFailure(const QString &id);

    QTimer m_pollTimer;
    int    m_pollCycle = 0;

    // hardware flags
    bool    m_hasBattery   = false;
    bool    m_hasBacklight = false;
    bool    m_hasWifi      = false;
    bool    m_hasBluetooth = false;
    QString m_wifiDevicePath;           // cached NM device path

    // state
    int     m_batteryPercent  = 100;
    bool    m_batteryCharging = false;

    bool    m_wifiEnabled   = false;
    bool    m_wifiConnected = false;
    QString m_wifiSSID;
    int     m_wifiStrength  = 0;

    bool    m_bluetoothEnabled = false;

    int     m_volume = 50;
    bool    m_muted  = false;

    double  m_brightness = 0.8;
    bool    m_dndEnabled = false;
    bool    m_micMuted = false;

    // backoff
    QHash<QString, int> m_failCounts;
    QSet<QString>       m_runningCommands;
};

#endif
