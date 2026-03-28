import QtQuick
import Chi 1.0

Item {
    id: powerMenu
    signal close()

    // Scrim
    Rectangle {
        anchors.fill: parent
        color: ChiTheme.colors.scrim
        opacity: 0.4
        MouseArea {
            anchors.fill: parent
            onClicked: powerMenu.close()
        }
    }

    // Dialog
    Rectangle {
        id: dialog
        anchors.centerIn: parent
        width: 280
        height: menuCol.height + 32
        radius: 28
        color: ChiTheme.colors.surface
        border.width: 1
        border.color: ChiTheme.colors.outlineVariant

        MouseArea { anchors.fill: parent }

        Column {
            id: menuCol
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 16
            spacing: 4

            Text {
                text: "Power"
                color: ChiTheme.colors.onSurface
                font { pixelSize: 18; weight: 500; family: ChiTheme.fontFamily }
                bottomPadding: 8
            }

            PowerMenuItem {
                icon: "lock"
                label: "Lock Screen"
                onClicked: { powerMenu.close(); shell.quickSettingsOpen = false; powerActions.lockScreen() }
            }

            PowerMenuItem {
                icon: "dark_mode"
                label: "Suspend"
                onClicked: { powerMenu.close(); shell.quickSettingsOpen = false; powerActions.suspend() }
            }

            PowerMenuItem {
                icon: "restart_alt"
                label: "Restart"
                onClicked: { powerMenu.close(); shell.quickSettingsOpen = false; powerActions.reboot() }
            }

            PowerMenuItem {
                icon: "power_settings_new"
                label: "Shut Down"
                iconColor: ChiTheme.colors.error
                onClicked: { powerMenu.close(); shell.quickSettingsOpen = false; powerActions.shutdown() }
            }

            PowerMenuItem {
                icon: "logout"
                label: "Log Out"
                onClicked: { powerMenu.close(); shell.quickSettingsOpen = false; powerActions.logout() }
            }
        }
    }

    // Animate in
    opacity: 0
    Component.onCompleted: opacity = 1
    Behavior on opacity {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    Keys.onEscapePressed: powerMenu.close()
}
