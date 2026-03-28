import QtQuick
import Chi 1.0
import "../Notification" as Notif

Item {
    width: parent.width
    height: col.height

    Column {
        id: col
        width: parent.width
        spacing: 8

        // ── Time + Tray Icons row ────────────────────
        Row {
            spacing: 8

            // Time — tap to toggle notification center
            Text {
                id: bigTime
                text: Qt.formatTime(new Date(), "h:mm")
                color: ChiTheme.colors.onSurface
                font { pixelSize: 36; weight: 500; family: ChiTheme.fontFamily }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: shell.notificationCenterOpen = !shell.notificationCenterOpen
                }
            }

            // Notification tray icons — appear after time
            Notif.NotificationTrayIcons {
                anchors.verticalCenter: bigTime.verticalCenter
                anchors.verticalCenterOffset: 4 // slight offset for visual alignment
            }
        }

        // ── Date + system icons row ──────────────────
        Item {
            width: parent.width; height: 20

            Text {
                text: Qt.formatDate(new Date(), "ddd, MMM d")
                color: ChiTheme.colors.onSurfaceVariant
                font {
                    pixelSize: ChiTheme.typography.labelLarge.size
                    weight: ChiTheme.typography.labelLarge.weight
                    family: ChiTheme.fontFamily
                }
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            Row {
                spacing: 4
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                // DND indicator
                Icon {
                    source: "do_not_disturb_on"
                    size: 14
                    color: ChiTheme.colors.error
                    visible: notifications.doNotDisturb
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: 0.7
                }

                Icon {
                    source: wifiManager.icon; size: 18
                    color: ChiTheme.colors.onSurfaceVariant
                    visible: wifiManager.available
                    anchors.verticalCenter: parent.verticalCenter
                }
                Icon {
                    source: systemStatus.bluetoothIcon; size: 18
                    color: ChiTheme.colors.onSurfaceVariant
                    visible: systemStatus.hasBluetooth
                    anchors.verticalCenter: parent.verticalCenter
                }
                Row {
                    spacing: 2
                    visible: systemStatus.hasBattery
                    anchors.verticalCenter: parent.verticalCenter

                    Icon {
                        source: systemStatus.batteryIcon; size: 18
                        color: ChiTheme.colors.onSurfaceVariant
                        anchors.verticalCenter: parent.verticalCenter
                    }
                    Text {
                        text: systemStatus.batteryPercent + "%"
                        color: ChiTheme.colors.onSurfaceVariant
                        font { pixelSize: 14; weight: 500; family: ChiTheme.fontFamily }
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }
        }
    }

    Timer {
        interval: 60000; running: true; repeat: true
        triggeredOnStart: true
        onTriggered: bigTime.text = Qt.formatTime(new Date(), "h:mm")
    }
}
