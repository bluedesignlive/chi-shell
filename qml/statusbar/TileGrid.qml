import QtQuick
import Chi 1.0

Column {
    id: grid
    width: parent.width
    spacing: 8

    signal openWifiPanel()

    readonly property real slotW: (width - 8) / 2
    readonly property real miniW: (slotW - 8) / 2

    // ── Mic mute state (polls once, then on toggle) ──
    property bool micMuted: false

    Component.onCompleted: {
        checkMicMute()
    }

    function checkMicMute() {
        // Will be overwritten by actual state
    }

    function toggleMicMute() {
        micMuted = !micMuted
        var val = micMuted ? "1" : "0"
        // fire-and-forget
        Qt.createQmlObject(
            'import QtQuick; Item { Component.onCompleted: Qt.quit() }', grid)
    }

    // ── Row 1: WiFi + Bluetooth ──────────────────────
    Flow {
        width: parent.width; spacing: 8
        visible: wifiManager.available || systemStatus.hasBluetooth

        SettingsTile {
            width: {
                if (wifiManager.available && systemStatus.hasBluetooth)
                    return grid.slotW
                return grid.width
            }
            height: 72
            icon: wifiManager.icon
            title: {
                if (!wifiManager.enabled) return "WiFi"
                if (wifiManager.connected && wifiManager.ssid)
                    return wifiManager.ssid
                return "WiFi"
            }
            description: {
                if (!wifiManager.enabled) return "Off"
                if (wifiManager.connected) return "Connected"
                return "Not connected"
            }
            active: wifiManager.enabled && wifiManager.connected
            wide: true
            visible: wifiManager.available
            onClicked: wifiManager.enabled = !wifiManager.enabled
            onLongPressed: grid.openWifiPanel()
        }

        SettingsTile {
            width: {
                if (wifiManager.available && systemStatus.hasBluetooth)
                    return grid.slotW
                return grid.width
            }
            height: 72
            icon: systemStatus.bluetoothIcon
            title: "Bluetooth"
            description: systemStatus.bluetoothEnabled ? "On" : "Off"
            active: systemStatus.bluetoothEnabled
            wide: true
            visible: systemStatus.hasBluetooth
            onClicked: systemStatus.bluetoothEnabled = !systemStatus.bluetoothEnabled
        }
    }

    // ── Row 2: Theme + DND/Mute ──────────────────────
    Row {
        width: parent.width; spacing: 8

        SettingsTile {
            width: grid.slotW; height: 72
            icon: ChiTheme.isDarkMode ? "dark_mode" : "light_mode"
            title: ChiTheme.isDarkMode ? "Dark" : "Light"
            description: "Theme"
            active: ChiTheme.isDarkMode; wide: true
            onClicked: ChiTheme.toggleDarkMode()
        }

        Item {
            width: grid.slotW; height: 72
            Row {
                anchors.fill: parent; spacing: 8

                SettingsTile {
                    width: grid.miniW; height: 72
                    icon: systemStatus.dndEnabled ? "do_not_disturb_on"
                                                  : "do_not_disturb_off"
                    active: systemStatus.dndEnabled; wide: false
                    onClicked: systemStatus.dndEnabled = !systemStatus.dndEnabled
                }
                SettingsTile {
                    width: grid.miniW; height: 72
                    icon: systemStatus.muted ? "volume_off" : systemStatus.audioIcon
                    active: !systemStatus.muted; wide: false
                    onClicked: systemStatus.muted = !systemStatus.muted
                }
            }
        }
    }

    // ── Row 3: Power Profile + mic/caffeine ──────────
    Flow {
        width: parent.width; spacing: 8

        SettingsTile {
            width: grid.slotW; height: 72
            icon: powerProfiles.icon
            title: powerProfiles.label
            description: "Power"
            active: powerProfiles.activeProfile !== "balanced"
            wide: true
            visible: powerProfiles.available
            onClicked: powerProfiles.cycle()
        }

        Item {
            width: grid.slotW; height: 72

            Row {
                anchors.fill: parent; spacing: 8

                SettingsTile {
                    id: caffeineTile
                    property bool on: false
                    width: grid.miniW; height: 72
                    icon: "coffee"; active: on; wide: false
                    onClicked: on = !on
                }
                SettingsTile {
                    width: grid.miniW; height: 72
                    icon: grid.micMuted ? "mic_off" : "mic"
                    active: !grid.micMuted; wide: false
                    onClicked: {
                        grid.micMuted = !grid.micMuted
                        systemStatus.setMicMuted(grid.micMuted)
                    }
                }
            }
        }
    }

    // ── Row 4: Screenshot + Screen Record ────────────
    Flow {
        width: parent.width; spacing: 8
        visible: screenCapture.hasGrim || screenCapture.hasRecorder

        SettingsTile {
            width: grid.slotW; height: 72
            icon: "screenshot"
            title: "Screenshot"
            description: "Capture"
            active: false; wide: true
            visible: screenCapture.hasGrim
            onClicked: {
                shell.quickSettingsOpen = false
                delayedScreenshot.start()
            }
            onLongPressed: {
                shell.quickSettingsOpen = false
                delayedRegionShot.start()
            }
        }

        SettingsTile {
            width: grid.slotW; height: 72
            icon: screenCapture.recording ? "stop_circle" : "screen_record"
            title: screenCapture.recording
                   ? formatTime(screenCapture.recordSeconds)
                   : "Record"
            description: screenCapture.recording ? "Recording..." : "Screen"
            active: screenCapture.recording; wide: true
            visible: screenCapture.hasRecorder
            onClicked: {
                if (screenCapture.recording) {
                    screenCapture.stopRecording()
                } else {
                    shell.quickSettingsOpen = false
                    delayedRecord.start()
                }
            }
            onLongPressed: {
                if (!screenCapture.recording) {
                    shell.quickSettingsOpen = false
                    delayedRegionRecord.start()
                }
            }
        }
    }

    Timer { id: delayedScreenshot; interval: 350; onTriggered: screenCapture.screenshotFull() }
    Timer { id: delayedRegionShot; interval: 350; onTriggered: screenCapture.screenshotRegion() }
    Timer { id: delayedRecord; interval: 350; onTriggered: screenCapture.startRecording(true) }
    Timer { id: delayedRegionRecord; interval: 350; onTriggered: screenCapture.startRecordingRegion(true) }

    function formatTime(secs) {
        var m = Math.floor(secs / 60)
        var s = secs % 60
        return m + ":" + (s < 10 ? "0" : "") + s
    }
}
