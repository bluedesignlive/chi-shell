import QtQuick
import Chi 1.0

Item {
    id: footer
    width: parent.width; height: 56

    signal openPowerMenu()

    Rectangle {
        anchors.top: parent.top
        width: parent.width; height: 1
        color: ChiTheme.colors.outlineVariant; opacity: 0.15
    }

    Row {
        anchors.centerIn: parent
        spacing: 8

        // Settings
        Rectangle {
            width: 40; height: 40; radius: 1000
            color: ChiTheme.colors.surfaceContainerHigh

            Icon {
                anchors.centerIn: parent
                source: "settings"; size: 22
                color: ChiTheme.colors.onSurfaceVariant
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: shell.quickSettingsOpen = false
            }
        }

        // Night Light
        Rectangle {
            width: 40; height: 40; radius: 1000
            visible: nightLight.available
            color: nightLight.active
                   ? ChiTheme.colors.tertiaryContainer
                   : ChiTheme.colors.surfaceContainerHigh

            Behavior on color { ColorAnimation { duration: 150 } }

            Icon {
                anchors.centerIn: parent
                source: "nightlight"; size: 22
                color: nightLight.active
                       ? ChiTheme.colors.onTertiaryContainer
                       : ChiTheme.colors.onSurfaceVariant
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: nightLight.toggle()
            }
        }

        // Power
        Rectangle {
            width: 40; height: 40; radius: 1000
            color: ChiTheme.colors.primaryContainer

            Icon {
                anchors.centerIn: parent
                source: "power_settings_new"; size: 22
                color: ChiTheme.colors.onPrimaryContainer
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: footer.openPowerMenu()
            }
        }
    }
}
