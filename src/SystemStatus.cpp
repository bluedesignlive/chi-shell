#include "SystemStatus.h"
#include <QDBusReply>
#include <QDBusInterface>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDebug>

SystemStatus::SystemStatus(QObject *parent)
    : QObject(parent)
{
    connect(&m_pollTimer, &QTimer::timeout, this, &SystemStatus::refresh);
    m_pollTimer.start(15000);

    // Delay first poll — give system services time to start
    QTimer::singleShot(2000, this, &SystemStatus::refresh);
}

void SystemStatus::refresh()
{
    m_pollCycle++;
    pollBattery();
    pollNetwork();
    pollBluetooth();
    pollAudioVolume();
    pollAudioMute();
    pollBrightness();
}

// ─── Async process with backoff ─────────────────────────

bool SystemStatus::shouldSkip(const QString &id) const
{
    int fails = m_failCounts.value(id, 0);
    if (fails < 3)   return false;             // first 3 failures: always try
    if (fails >= 10)  return true;             // 10+ failures: give up entirely
    // 3-9 failures: try once every 4 cycles (once per minute at 15s interval)
    return (m_pollCycle % 4) != 0;
}

void SystemStatus::recordSuccess(const QString &id)
{
    m_failCounts[id] = 0;
}

void SystemStatus::recordFailure(const QString &id)
{
    int count = ++m_failCounts[id];
    if (count == 3)
        qWarning() << "SystemStatus:" << id << "failed 3 times, backing off";
    else if (count == 10)
        qWarning() << "SystemStatus:" << id << "failed 10 times, disabling";
}

void SystemStatus::asyncCommand(const QString &id, const QString &program,
                                 const QStringList &args,
                                 std::function<void(const QString &)> callback)
{
    if (shouldSkip(id)) return;

    // Don't start another if one is already running
    if (m_runningCommands.contains(id)) return;
    m_runningCommands.insert(id);

    auto *proc = new QProcess(this);

    auto *timeout = new QTimer(proc);
    timeout->setSingleShot(true);
    timeout->setInterval(2000);
    connect(timeout, &QTimer::timeout, proc, [this, proc, id]() {
        recordFailure(id);
        m_runningCommands.remove(id);
        proc->kill();
    });

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [this, proc, id, callback](int exitCode, QProcess::ExitStatus) {
        m_runningCommands.remove(id);
        if (exitCode == 0) {
            recordSuccess(id);
            callback(proc->readAllStandardOutput());
        } else {
            recordFailure(id);
        }
        proc->deleteLater();
    });

    connect(proc, &QProcess::errorOccurred, this, [this, proc, id](QProcess::ProcessError err) {
        m_runningCommands.remove(id);
        if (err == QProcess::FailedToStart)
            recordFailure(id);
        proc->deleteLater();
    });

    proc->start(program, args);
    timeout->start();
}

// ─── Battery via UPower (DBus — always fast) ────────────

void SystemStatus::pollBattery()
{
    QDBusInterface upower(
        "org.freedesktop.UPower",
        "/org/freedesktop/UPower/devices/DisplayDevice",
        "org.freedesktop.UPower.Device",
        QDBusConnection::systemBus()
    );

    if (!upower.isValid()) return;

    int percent = static_cast<int>(upower.property("Percentage").toDouble());
    uint state = upower.property("State").toUInt();
    bool charging = (state == 1 || state == 4);

    if (percent != m_batteryPercent || charging != m_batteryCharging) {
        m_batteryPercent = percent;
        m_batteryCharging = charging;
        emit batteryChanged();
    }
}

QString SystemStatus::batteryIcon() const
{
    if (m_batteryCharging) return "battery_charging_full";
    if (m_batteryPercent > 90) return "battery_full";
    if (m_batteryPercent > 60) return "battery_5_bar";
    if (m_batteryPercent > 30) return "battery_3_bar";
    if (m_batteryPercent > 10) return "battery_1_bar";
    return "battery_alert";
}

// ─── Network via NetworkManager ─────────────────────────

void SystemStatus::pollNetwork()
{
    QDBusInterface nm(
        "org.freedesktop.NetworkManager",
        "/org/freedesktop/NetworkManager",
        "org.freedesktop.NetworkManager",
        QDBusConnection::systemBus()
    );

    if (!nm.isValid()) return;

    bool wifiEnabled = nm.property("WirelessEnabled").toBool();
    uint nmState = nm.property("State").toUInt();
    bool connected = (nmState >= 70);

    if (!connected) {
        if (wifiEnabled != m_wifiEnabled || connected != m_wifiConnected
            || !m_wifiSSID.isEmpty()) {
            m_wifiEnabled = wifiEnabled;
            m_wifiConnected = false;
            m_wifiSSID.clear();
            m_wifiStrength = 0;
            emit networkChanged();
        }
        return;
    }

    // SSID + strength via nmcli (async, with --wait 0 to prevent scan blocking)
    asyncCommand("nmcli-wifi", "nmcli",
        {"-t", "-f", "ACTIVE,SSID,SIGNAL", "--wait", "0", "dev", "wifi", "list"},
        [this, wifiEnabled, connected](const QString &output) {
            QString ssid;
            int strength = 0;
            for (const auto &line : output.split('\n')) {
                if (line.startsWith("yes:")) {
                    auto parts = line.split(':');
                    if (parts.size() >= 3) {
                        ssid = parts[1];
                        strength = parts[2].toInt();
                    }
                    break;
                }
            }
            if (wifiEnabled != m_wifiEnabled || connected != m_wifiConnected
                || ssid != m_wifiSSID || strength != m_wifiStrength) {
                m_wifiEnabled = wifiEnabled;
                m_wifiConnected = connected;
                m_wifiSSID = ssid;
                m_wifiStrength = strength;
                emit networkChanged();
            }
        }
    );
}

QString SystemStatus::networkIcon() const
{
    if (!m_wifiEnabled) return "wifi_off";
    if (!m_wifiConnected) return "wifi_find";
    if (m_wifiStrength > 75) return "wifi";
    if (m_wifiStrength > 50) return "wifi_2_bar";
    if (m_wifiStrength > 25) return "wifi_1_bar";
    return "wifi_1_bar";
}

void SystemStatus::setWifiEnabled(bool enabled)
{
    QDBusInterface nm(
        "org.freedesktop.NetworkManager",
        "/org/freedesktop/NetworkManager",
        "org.freedesktop.DBus.Properties",
        QDBusConnection::systemBus()
    );
    nm.call("Set", "org.freedesktop.NetworkManager",
            "WirelessEnabled", QVariant::fromValue(QDBusVariant(enabled)));
    m_wifiEnabled = enabled;
    emit networkChanged();
}

// ─── Bluetooth via BlueZ (DBus — always fast) ──────────

void SystemStatus::pollBluetooth()
{
    QDBusInterface adapter(
        "org.bluez",
        "/org/bluez/hci0",
        "org.bluez.Adapter1",
        QDBusConnection::systemBus()
    );

    if (!adapter.isValid()) return;

    bool powered = adapter.property("Powered").toBool();
    if (powered != m_bluetoothEnabled) {
        m_bluetoothEnabled = powered;
        emit bluetoothChanged();
    }
}

QString SystemStatus::bluetoothIcon() const
{
    return m_bluetoothEnabled ? "bluetooth" : "bluetooth_disabled";
}

void SystemStatus::setBluetoothEnabled(bool enabled)
{
    QDBusInterface adapter(
        "org.bluez",
        "/org/bluez/hci0",
        "org.freedesktop.DBus.Properties",
        QDBusConnection::systemBus()
    );
    adapter.call("Set", "org.bluez.Adapter1",
                 "Powered", QVariant::fromValue(QDBusVariant(enabled)));
    m_bluetoothEnabled = enabled;
    emit bluetoothChanged();
}

// ─── Audio via pactl (async with backoff) ───────────────

void SystemStatus::pollAudioVolume()
{
    asyncCommand("pactl-volume", "pactl", {"get-sink-volume", "@DEFAULT_SINK@"},
        [this](const QString &output) {
            int idx = output.indexOf('%');
            if (idx > 0) {
                int start = idx - 1;
                while (start > 0 && output[start - 1].isDigit())
                    --start;
                int vol = output.mid(start, idx - start).toInt();
                if (vol != m_volume) {
                    m_volume = vol;
                    emit audioChanged();
                }
            }
        }
    );
}

void SystemStatus::pollAudioMute()
{
    asyncCommand("pactl-mute", "pactl", {"get-sink-mute", "@DEFAULT_SINK@"},
        [this](const QString &output) {
            bool muted = output.contains("yes");
            if (muted != m_muted) {
                m_muted = muted;
                emit audioChanged();
            }
        }
    );
}

QString SystemStatus::audioIcon() const
{
    if (m_muted) return "volume_off";
    if (m_volume > 66) return "volume_up";
    if (m_volume > 33) return "volume_down";
    if (m_volume > 0) return "volume_mute";
    return "volume_off";
}

void SystemStatus::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    QProcess::startDetached("pactl", {
        "set-sink-volume", "@DEFAULT_SINK@",
        QString::number(volume) + "%"
    });
    m_volume = volume;
    emit audioChanged();
}

void SystemStatus::setMuted(bool muted)
{
    QProcess::startDetached("pactl", {
        "set-sink-mute", "@DEFAULT_SINK@",
        muted ? "1" : "0"
    });
    m_muted = muted;
    emit audioChanged();
}

// ─── Brightness via sysfs (file I/O — always fast) ──────

void SystemStatus::pollBrightness()
{
    QDir backlightDir("/sys/class/backlight");
    QStringList devices = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (devices.isEmpty()) return;

    QString device = backlightDir.filePath(devices.first());
    QFile maxFile(device + "/max_brightness");
    QFile curFile(device + "/brightness");

    if (!maxFile.open(QIODevice::ReadOnly) || !curFile.open(QIODevice::ReadOnly))
        return;

    double max = maxFile.readAll().trimmed().toDouble();
    double cur = curFile.readAll().trimmed().toDouble();

    if (max > 0) {
        double brightness = cur / max;
        if (qAbs(brightness - m_brightness) > 0.01) {
            m_brightness = brightness;
            emit brightnessChanged();
        }
    }
}

void SystemStatus::setBrightness(double brightness)
{
    brightness = qBound(0.0, brightness, 1.0);
    QDir backlightDir("/sys/class/backlight");
    QStringList devices = backlightDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    if (devices.isEmpty()) {
        int percent = static_cast<int>(brightness * 100);
        QProcess::startDetached("brightnessctl", {
            "set", QString::number(percent) + "%"
        });
    } else {
        QString device = backlightDir.filePath(devices.first());
        QFile maxFile(device + "/max_brightness");
        if (maxFile.open(QIODevice::ReadOnly)) {
            int max = maxFile.readAll().trimmed().toInt();
            int val = static_cast<int>(brightness * max);
            QProcess::startDetached("brightnessctl", {
                "set", QString::number(val)
            });
        }
    }
    m_brightness = brightness;
    emit brightnessChanged();
}

void SystemStatus::setDndEnabled(bool enabled)
{
    m_dndEnabled = enabled;
    emit dndChanged();
}
