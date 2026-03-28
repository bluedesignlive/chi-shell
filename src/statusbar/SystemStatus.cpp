#include "SystemStatus.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusArgument>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDebug>

SystemStatus::SystemStatus(QObject *parent)
    : QObject(parent)
{
    detectHardware();

    connect(&m_pollTimer, &QTimer::timeout, this, &SystemStatus::refresh);
    m_pollTimer.start(15000);

    QTimer::singleShot(1500, this, &SystemStatus::refresh);
}

// ═══════════════════════════════════════════════════════
// Hardware detection — runs once at startup
// ═══════════════════════════════════════════════════════

void SystemStatus::detectHardware()
{
    // ── Battery: check UPower DisplayDevice type ─────
    {
        QDBusInterface upower(
            "org.freedesktop.UPower",
            "/org/freedesktop/UPower/devices/DisplayDevice",
            "org.freedesktop.UPower.Device",
            QDBusConnection::systemBus());

        if (upower.isValid()) {
            uint devType = upower.property("Type").toUInt();
            m_hasBattery = (devType == 2);  // 2 = Battery
        }
        qDebug() << "SystemStatus: battery" << (m_hasBattery ? "detected" : "not found");
    }

    // ── Backlight ────────────────────────────────────
    {
        QDir dir("/sys/class/backlight");
        m_hasBacklight = !dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot).isEmpty();
        qDebug() << "SystemStatus: backlight" << (m_hasBacklight ? "detected" : "not found");
    }

    // ── WiFi: find NM wireless device, cache its path ─
    {
        QDBusInterface nm(
            "org.freedesktop.NetworkManager",
            "/org/freedesktop/NetworkManager",
            "org.freedesktop.NetworkManager",
            QDBusConnection::systemBus());

        if (nm.isValid()) {
            QDBusMessage reply = nm.call("GetDevices");
            if (reply.type() == QDBusMessage::ReplyMessage && !reply.arguments().isEmpty()) {
                const QDBusArgument &arg = reply.arguments().at(0).value<QDBusArgument>();
                QList<QDBusObjectPath> devices;
                arg >> devices;

                for (const auto &devPath : devices) {
                    QDBusInterface dev(
                        "org.freedesktop.NetworkManager",
                        devPath.path(),
                        "org.freedesktop.NetworkManager.Device",
                        QDBusConnection::systemBus());

                    if (dev.property("DeviceType").toUInt() == 2) {
                        m_hasWifi = true;
                        m_wifiDevicePath = devPath.path();
                        break;
                    }
                }
            }
        }
        qDebug() << "SystemStatus: wifi" << (m_hasWifi ? "detected at" + m_wifiDevicePath : "not found");
    }

    // ── Bluetooth ────────────────────────────────────
    {
        QDBusInterface adapter(
            "org.bluez", "/org/bluez/hci0",
            "org.bluez.Adapter1",
            QDBusConnection::systemBus());
        m_hasBluetooth = adapter.isValid();
        qDebug() << "SystemStatus: bluetooth" << (m_hasBluetooth ? "detected" : "not found");
    }
}

// ═══════════════════════════════════════════════════════
// Refresh — called by timer and on-demand
// ═══════════════════════════════════════════════════════

void SystemStatus::refresh()
{
    m_pollCycle++;
    if (m_hasBattery)    pollBattery();
    if (m_hasWifi)       pollNetwork();
    if (m_hasBluetooth)  pollBluetooth();
    pollAudioVolume();
    pollAudioMute();
    if (m_hasBacklight)  pollBrightness();
}

// ═══════════════════════════════════════════════════════
// Async process with backoff
// ═══════════════════════════════════════════════════════

bool SystemStatus::shouldSkip(const QString &id) const
{
    int fails = m_failCounts.value(id, 0);
    if (fails < 3)  return false;
    if (fails >= 10) return true;
    return (m_pollCycle % 4) != 0;
}

void SystemStatus::recordSuccess(const QString &id)  { m_failCounts[id] = 0; }

void SystemStatus::recordFailure(const QString &id)
{
    int c = ++m_failCounts[id];
    if (c == 3)  qWarning() << "SystemStatus:" << id << "failed 3×, backing off";
    if (c == 10) qWarning() << "SystemStatus:" << id << "failed 10×, disabled";
}

void SystemStatus::asyncCommand(const QString &id, const QString &program,
                                 const QStringList &args,
                                 std::function<void(const QString &)> callback)
{
    if (shouldSkip(id) || m_runningCommands.contains(id))
        return;
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
        if (err == QProcess::FailedToStart) recordFailure(id);
        proc->deleteLater();
    });

    proc->start(program, args);
    timeout->start();
}

// ═══════════════════════════════════════════════════════
// Battery — UPower D-Bus (skipped if no battery)
// ═══════════════════════════════════════════════════════

void SystemStatus::pollBattery()
{
    QDBusInterface upower(
        "org.freedesktop.UPower",
        "/org/freedesktop/UPower/devices/DisplayDevice",
        "org.freedesktop.UPower.Device",
        QDBusConnection::systemBus());

    if (!upower.isValid()) return;

    int percent = static_cast<int>(upower.property("Percentage").toDouble());
    uint state  = upower.property("State").toUInt();
    // 1=charging, 4=fully-charged (plugged in)
    bool charging = (state == 1 || state == 4);

    if (percent != m_batteryPercent || charging != m_batteryCharging) {
        m_batteryPercent  = percent;
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

// ═══════════════════════════════════════════════════════
// Network — PURE D-Bus, no nmcli, no subprocess
// Uses cached WiFi device path from detectHardware()
// ═══════════════════════════════════════════════════════

void SystemStatus::pollNetwork()
{
    QDBusInterface nm(
        "org.freedesktop.NetworkManager",
        "/org/freedesktop/NetworkManager",
        "org.freedesktop.NetworkManager",
        QDBusConnection::systemBus());

    if (!nm.isValid()) return;

    bool wifiEnabled = nm.property("WirelessEnabled").toBool();
    uint nmState     = nm.property("State").toUInt();
    bool connected   = (nmState >= 70);

    QString ssid;
    int strength = 0;

    if (connected && wifiEnabled && !m_wifiDevicePath.isEmpty()) {
        // Read ActiveAccessPoint from cached WiFi device
        QDBusInterface wifiDev(
            "org.freedesktop.NetworkManager",
            m_wifiDevicePath,
            "org.freedesktop.NetworkManager.Device.Wireless",
            QDBusConnection::systemBus());

        if (wifiDev.isValid()) {
            QDBusObjectPath apPath = wifiDev.property("ActiveAccessPoint")
                                        .value<QDBusObjectPath>();

            if (!apPath.path().isEmpty() && apPath.path() != "/") {
                QDBusInterface ap(
                    "org.freedesktop.NetworkManager",
                    apPath.path(),
                    "org.freedesktop.NetworkManager.AccessPoint",
                    QDBusConnection::systemBus());

                if (ap.isValid()) {
                    ssid     = QString::fromUtf8(ap.property("Ssid").toByteArray());
                    strength = ap.property("Strength").toUInt();
                }
            }
        }
    }

    if (wifiEnabled != m_wifiEnabled || connected != m_wifiConnected
        || ssid != m_wifiSSID || strength != m_wifiStrength) {
        m_wifiEnabled   = wifiEnabled;
        m_wifiConnected = connected;
        m_wifiSSID      = ssid;
        m_wifiStrength  = strength;
        emit networkChanged();
    }
}

QString SystemStatus::networkIcon() const
{
    if (!m_wifiEnabled)   return "wifi_off";
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
        QDBusConnection::systemBus());
    nm.call("Set", "org.freedesktop.NetworkManager",
            "WirelessEnabled", QVariant::fromValue(QDBusVariant(enabled)));
    m_wifiEnabled = enabled;
    emit networkChanged();
}

// ═══════════════════════════════════════════════════════
// Bluetooth — BlueZ D-Bus
// ═══════════════════════════════════════════════════════

void SystemStatus::pollBluetooth()
{
    QDBusInterface adapter(
        "org.bluez", "/org/bluez/hci0",
        "org.bluez.Adapter1",
        QDBusConnection::systemBus());
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
        "org.bluez", "/org/bluez/hci0",
        "org.freedesktop.DBus.Properties",
        QDBusConnection::systemBus());
    adapter.call("Set", "org.bluez.Adapter1",
                 "Powered", QVariant::fromValue(QDBusVariant(enabled)));
    m_bluetoothEnabled = enabled;
    emit bluetoothChanged();
}

// ═══════════════════════════════════════════════════════
// Audio — async pactl with backoff
// ═══════════════════════════════════════════════════════

void SystemStatus::pollAudioVolume()
{
    asyncCommand("pactl-volume", "pactl", {"get-sink-volume", "@DEFAULT_SINK@"},
        [this](const QString &output) {
            int idx = output.indexOf('%');
            if (idx > 0) {
                int start = idx - 1;
                while (start > 0 && output[start - 1].isDigit()) --start;
                int vol = output.mid(start, idx - start).toInt();
                if (vol != m_volume) { m_volume = vol; emit audioChanged(); }
            }
        });
}

void SystemStatus::pollAudioMute()
{
    asyncCommand("pactl-mute", "pactl", {"get-sink-mute", "@DEFAULT_SINK@"},
        [this](const QString &output) {
            bool m = output.contains("yes");
            if (m != m_muted) { m_muted = m; emit audioChanged(); }
        });
}

QString SystemStatus::audioIcon() const
{
    if (m_muted)       return "volume_off";
    if (m_volume > 66) return "volume_up";
    if (m_volume > 33) return "volume_down";
    if (m_volume > 0)  return "volume_mute";
    return "volume_off";
}

void SystemStatus::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    QProcess::startDetached("pactl", {"set-sink-volume", "@DEFAULT_SINK@",
                                       QString::number(volume) + "%"});
    m_volume = volume;
    emit audioChanged();
}

void SystemStatus::setMuted(bool muted)
{
    QProcess::startDetached("pactl", {"set-sink-mute", "@DEFAULT_SINK@",
                                       muted ? "1" : "0"});
    m_muted = muted;
    emit audioChanged();
}

// ═══════════════════════════════════════════════════════
// Brightness — sysfs (skipped if no backlight)
// ═══════════════════════════════════════════════════════

void SystemStatus::pollBrightness()
{
    QDir dir("/sys/class/backlight");
    QStringList devs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (devs.isEmpty()) return;

    QString base = dir.filePath(devs.first());
    QFile maxF(base + "/max_brightness");
    QFile curF(base + "/brightness");
    if (!maxF.open(QIODevice::ReadOnly) || !curF.open(QIODevice::ReadOnly)) return;

    double mx = maxF.readAll().trimmed().toDouble();
    double cu = curF.readAll().trimmed().toDouble();
    if (mx <= 0) return;

    double b = cu / mx;
    if (qAbs(b - m_brightness) > 0.01) {
        m_brightness = b;
        emit brightnessChanged();
    }
}

void SystemStatus::setBrightness(double brightness)
{
    brightness = qBound(0.0, brightness, 1.0);
    int percent = static_cast<int>(brightness * 100);
    QProcess::startDetached("brightnessctl", {"set", QString::number(percent) + "%"});
    m_brightness = brightness;
    emit brightnessChanged();
}

void SystemStatus::setDndEnabled(bool enabled)
{
    m_dndEnabled = enabled;
    emit dndChanged();
}

void SystemStatus::setMicMuted(bool muted)
{
    QProcess::startDetached("pactl", {
        "set-source-mute", "@DEFAULT_SOURCE@",
        muted ? "1" : "0"
    });
    m_micMuted = muted;
    emit micChanged();
}
