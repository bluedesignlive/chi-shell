import QtQuick
import Chi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    // ── scrim — click to close ───────────────────────────
    MouseArea {
        anchors.fill: parent
        onClicked: shell.quickSettingsOpen = false
    }

    // ── panel ────────────────────────────────────────────
    Rectangle {
        id: panel
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 40       // statusbar 36 + 4 gap
        anchors.rightMargin: 8
        width: 412
        height: Math.min(panelColumn.height + 32, parent.height - 104)
        radius: 28
        color: ChiTheme.colors.surface
        clip: true

        // block scrim click
        MouseArea { anchors.fill: parent }

        Column {
            id: panelColumn
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 12

            // ── A) Status Header ─────────────────────────
            Item {
                width: parent.width
                height: 100

                Column {
                    anchors.fill: parent
                    spacing: 8

                    Text {
                        id: bigTime
                        text: Qt.formatTime(new Date(), "h:mm")
                        color: ChiTheme.colors.onSurface
                        font.pixelSize: 36; font.weight: 500
                        font.family: ChiTheme.fontFamily
                    }

                    Item {
                        width: parent.width; height: 20

                        Text {
                            text: Qt.formatDate(new Date(), "ddd, MMM d")
                            color: ChiTheme.colors.onSurfaceVariant
                            font.pixelSize: ChiTheme.typography.labelLarge.size
                            font.weight: ChiTheme.typography.labelLarge.weight
                            font.family: ChiTheme.fontFamily
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                        }

                        Row {
                            spacing: 4
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter

                            Icon { source: systemStatus.networkIcon; size: 18; color: ChiTheme.colors.onSurfaceVariant }
                            Icon { source: systemStatus.bluetoothIcon; size: 18; color: ChiTheme.colors.onSurfaceVariant }
                            Icon { source: systemStatus.batteryIcon; size: 18; color: ChiTheme.colors.onSurfaceVariant }

                            Text {
                                text: systemStatus.batteryPercent + "%"
                                color: ChiTheme.colors.onSurfaceVariant
                                font.pixelSize: 14; font.weight: 500
                                font.family: ChiTheme.fontFamily
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }
                }
            }

            // ── B) Brightness Slider ─────────────────────
            Item {
                width: parent.width; height: 44

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width; height: 40; radius: 12
                    color: ChiTheme.colors.surfaceContainerHigh

                    Rectangle {
                        width: parent.width * systemStatus.brightness
                        height: parent.height; radius: 12
                        color: ChiTheme.colors.primaryContainer
                        Behavior on width { SpringAnimation { spring: 14.0; damping: 1.0 } }
                    }

                    Icon {
                        source: "brightness_medium"; size: 24
                        color: ChiTheme.colors.onSurfaceVariant
                        anchors.right: parent.right; anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: 0.7
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onPositionChanged: function(mouse) {
                        systemStatus.brightness = Math.max(0, Math.min(1, mouse.x / width))
                    }
                    onPressed: function(mouse) {
                        systemStatus.brightness = Math.max(0, Math.min(1, mouse.x / width))
                    }
                }
            }

            // ── C) Volume Slider ─────────────────────────
            Item {
                width: parent.width; height: 44

                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width; height: 40; radius: 12
                    color: ChiTheme.colors.surfaceContainerHigh

                    Rectangle {
                        width: parent.width * (systemStatus.volume / 100)
                        height: parent.height; radius: 12
                        color: ChiTheme.colors.primaryContainer
                        Behavior on width { SpringAnimation { spring: 14.0; damping: 1.0 } }
                    }

                    Icon {
                        source: systemStatus.audioIcon; size: 24
                        color: ChiTheme.colors.onSurfaceVariant
                        anchors.right: parent.right; anchors.rightMargin: 8
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: 0.7
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    onPositionChanged: function(mouse) {
                        systemStatus.volume = Math.round(Math.max(0, Math.min(100, mouse.x / width * 100)))
                    }
                    onPressed: function(mouse) {
                        systemStatus.volume = Math.round(Math.max(0, Math.min(100, mouse.x / width * 100)))
                    }
                }
            }

            // ── D) Tile Grid ─────────────────────────────
            Column {
                width: parent.width; spacing: 8

                Row {
                    width: parent.width; spacing: 8

                    SettingsTile {
                        width: (parent.width - 8) / 2; height: 72
                        icon: systemStatus.networkIcon
                        title: systemStatus.wifiConnected ? systemStatus.wifiSSID || "WiFi" : "WiFi"
                        description: systemStatus.wifiConnected ? "Connected" : "Off"
                        active: systemStatus.wifiEnabled; wide: true
                        onClicked: systemStatus.wifiEnabled = !systemStatus.wifiEnabled
                    }

                    SettingsTile {
                        width: (parent.width - 8) / 2; height: 72
                        icon: systemStatus.bluetoothIcon
                        title: "Bluetooth"
                        description: systemStatus.bluetoothEnabled ? "On" : "Off"
                        active: systemStatus.bluetoothEnabled; wide: true
                        onClicked: systemStatus.bluetoothEnabled = !systemStatus.bluetoothEnabled
                    }
                }

                Row {
                    width: parent.width; spacing: 8

                    SettingsTile {
                        width: (parent.width - 8) / 2; height: 72
                        icon: ChiTheme.isDarkMode ? "dark_mode" : "light_mode"
                        title: ChiTheme.isDarkMode ? "Dark" : "Light"
                        description: "Theme"
                        active: ChiTheme.isDarkMode; wide: true
                        onClicked: ChiTheme.toggleDarkMode()
                    }

                    Row {
                        width: (parent.width - 8) / 2; spacing: 8

                        SettingsTile {
                            width: (parent.width - 8) / 4 - 4; height: 72
                            icon: systemStatus.dndEnabled ? "do_not_disturb_on" : "do_not_disturb_off"
                            active: systemStatus.dndEnabled; wide: false
                            onClicked: systemStatus.dndEnabled = !systemStatus.dndEnabled
                        }

                        SettingsTile {
                            width: (parent.width - 8) / 4 - 4; height: 72
                            icon: systemStatus.muted ? "volume_off" : systemStatus.audioIcon
                            active: !systemStatus.muted; wide: false
                            onClicked: systemStatus.muted = !systemStatus.muted
                        }
                    }
                }
            }

            // ── E) Footer ────────────────────────────────
            Item {
                width: parent.width; height: 56

                Row {
                    anchors.centerIn: parent; spacing: 8

                    Rectangle {
                        width: 40; height: 40; radius: 1000
                        color: ChiTheme.colors.surfaceContainerHigh
                        Icon { anchors.centerIn: parent; source: "settings"; size: 24; color: ChiTheme.colors.onSurfaceVariant }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                    }

                    Rectangle {
                        width: 40; height: 40; radius: 1000
                        color: ChiTheme.colors.primaryContainer
                        Icon { anchors.centerIn: parent; source: "power_settings_new"; size: 24; color: ChiTheme.colors.onPrimaryContainer }
                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor }
                    }
                }
            }
        }
    }

    Keys.onEscapePressed: shell.quickSettingsOpen = false

    Timer {
        interval: 1000; running: true; repeat: true; triggeredOnStart: true
        onTriggered: bigTime.text = Qt.formatTime(new Date(), "h:mm")
    }
}
