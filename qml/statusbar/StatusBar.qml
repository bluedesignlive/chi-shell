import QtQuick
import Chi 1.0
import "../Notification" as Notif

Rectangle {
    id: root
    anchors.fill: parent
    color: ChiTheme.colors.surface

    Item {
        id: leftCluster
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        width: row.implicitWidth
        height: parent.height

        Row {
            id: row
            spacing: 8
            anchors.verticalCenter: parent.verticalCenter

            Text {
                id: clock
                text: Qt.formatTime(new Date(), "h:mm")
                color: ChiTheme.colors.onSurface
                font { pixelSize: 14; weight: 600; family: ChiTheme.fontFamily }
            }

            Notif.NotificationTrayIcons {
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: shell.notificationCenterOpen = !shell.notificationCenterOpen
        }
    }

    Row {
        anchors.centerIn: parent
        spacing: 8

        Rectangle {
            width: 8
            height: 8
            radius: 4
            color: ChiTheme.colors.error
            visible: screenCapture.recording
            anchors.verticalCenter: parent.verticalCenter

            SequentialAnimation on opacity {
                loops: Animation.Infinite
                running: screenCapture.recording
                NumberAnimation { to: 0.3; duration: 800 }
                NumberAnimation { to: 1.0; duration: 800 }
            }
        }

        Text {
            visible: screenCapture.recording
            text: {
                var s = screenCapture.recordSeconds
                var m = Math.floor(s / 60)
                var sec = s % 60
                return m + ":" + (sec < 10 ? "0" : "") + sec
            }
            color: ChiTheme.colors.error
            font { pixelSize: 12; weight: 600; family: ChiTheme.fontFamily }
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            visible: !screenCapture.recording
            text: Qt.formatDate(new Date(), "ddd, MMM d")
            color: ChiTheme.colors.onSurfaceVariant
            font { pixelSize: 13; weight: 500; family: ChiTheme.fontFamily }
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: indicatorRow.width + 24
        cursorShape: Qt.PointingHandCursor
        onClicked: shell.quickSettingsOpen = !shell.quickSettingsOpen

        Row {
            id: indicatorRow
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Icon {
                source: "do_not_disturb_on"
                size: 18
                color: ChiTheme.colors.error
                visible: notifications.doNotDisturb
                anchors.verticalCenter: parent.verticalCenter
            }

            Icon {
                source: systemStatus.audioIcon
                size: 18
                color: ChiTheme.colors.onSurfaceVariant
                anchors.verticalCenter: parent.verticalCenter
            }

            Icon {
                source: wifiManager.icon
                size: 18
                color: ChiTheme.colors.onSurfaceVariant
                visible: wifiManager.available
                anchors.verticalCenter: parent.verticalCenter
            }

            Icon {
                source: "bluetooth"
                size: 18
                color: ChiTheme.colors.onSurfaceVariant
                visible: systemStatus.hasBluetooth && systemStatus.bluetoothEnabled
                anchors.verticalCenter: parent.verticalCenter
            }

            Row {
                spacing: 2
                visible: systemStatus.hasBattery
                anchors.verticalCenter: parent.verticalCenter

                Icon {
                    source: systemStatus.batteryIcon
                    size: 18
                    color: (systemStatus.batteryPercent <= 15 && !systemStatus.batteryCharging)
                           ? ChiTheme.colors.error
                           : ChiTheme.colors.onSurfaceVariant
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: systemStatus.batteryPercent + "%"
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 12; weight: 500; family: ChiTheme.fontFamily }
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: ChiTheme.colors.outlineVariant
        opacity: 0.2
    }

    Timer {
        interval: 60000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: clock.text = Qt.formatTime(new Date(), "h:mm")
    }
}
