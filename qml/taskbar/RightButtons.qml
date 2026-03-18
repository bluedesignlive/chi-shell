import QtQuick
import Chi 1.0
import "../"

Row {
    id: rightBtns
    spacing: 2

    Rectangle {
        width: 40; height: 40; radius: 20
        color: _dm.pressed ? Qt.rgba(1,1,1,0.1)
             : _dm.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
        Icon {
            anchors.centerIn: parent
            source: "desktop_windows"
            size: 24
            color: ChiTheme.colors.onSurfaceVariant
        }
        scale: _dm.pressed ? 0.9 : 1.0
        Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
        MouseArea {
            id: _dm
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: function(mouse) {
                if (mouse.modifiers & Qt.ShiftModifier) { mouse.accepted = false; return }
                if (shell.appLauncherOpen) shell.appLauncherOpen = false
                else wayfireIPC.showDesktop()
            }
        }
    }

    Rectangle {
        width: 40; height: 40; radius: 20
        color: _nm.pressed ? Qt.rgba(1,1,1,0.1)
             : _nm.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
        Icon {
            anchors.centerIn: parent
            source: "notifications"
            size: 24
            color: ChiTheme.colors.onSurfaceVariant
        }
        scale: _nm.pressed ? 0.9 : 1.0
        Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
        MouseArea {
            id: _nm
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: function(mouse) {
                if (mouse.modifiers & Qt.ShiftModifier) { mouse.accepted = false; return }
                if (shell.appLauncherOpen) shell.appLauncherOpen = false
                shell.notificationCenterOpen = !shell.notificationCenterOpen
            }
        }
        Rectangle {
            visible: typeof notifications !== "undefined" && notifications.count > 0
            anchors.top: parent.top; anchors.right: parent.right
            anchors.topMargin: 2; anchors.rightMargin: 2
            width: Math.max(18, _bt.implicitWidth + 8); height: 18; radius: 9
            color: ChiTheme.colors.error
            Text {
                id: _bt
                anchors.centerIn: parent
                text: typeof notifications !== "undefined" ? notifications.count.toString() : "0"
                color: "#FFF"
                font.pixelSize: 10
                font.weight: 600
                font.family: ChiTheme.fontFamily
            }
        }
    }
}
