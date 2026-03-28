import QtQuick
import Chi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"
    focus: true

    MouseArea {
        anchors.fill: parent
        z: 0
        onPressed: function(mouse) {
            var p = panel.mapFromItem(root, mouse.x, mouse.y)
            var inside = p.x >= 0 && p.y >= 0 && p.x <= panel.width && p.y <= panel.height
            if (!inside)
                shell.notificationCenterOpen = false
            else
                mouse.accepted = false
        }
    }

    Rectangle {
        id: panel
        z: 1
        anchors.top: parent.top
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 40
        width: Math.min(412, parent.width - 16)
        height: Math.min(contentCol.implicitHeight + header.height + 48, parent.height - 104)
        radius: 28
        color: ChiTheme.colors.secondaryContainer
        opacity: 0.97
        clip: true

        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onPressed: function(mouse) { mouse.accepted = true }
        }

        y: -panel.height
        NumberAnimation on y {
            to: 0
            duration: 260
            easing.type: Easing.OutCubic
            running: true
        }

        Item {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 16
            height: 48

            Text {
                text: "Notifications"
                color: ChiTheme.colors.onSurfaceVariant
                font {
                    pixelSize: ChiTheme.typography.titleSmall.size
                    weight: ChiTheme.typography.titleSmall.weight
                    family: ChiTheme.fontFamily
                }
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                spacing: 16

                Row {
                    spacing: 4

                    Icon {
                        source: notifications.doNotDisturb ? "do_not_disturb_on" : "do_not_disturb_off"
                        size: 18
                        color: notifications.doNotDisturb ? ChiTheme.colors.error : ChiTheme.colors.onSurfaceVariant
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    MouseArea {
                        width: 18
                        height: 18
                        anchors.verticalCenter: parent.verticalCenter
                        onClicked: notifications.doNotDisturb = !notifications.doNotDisturb
                    }
                }

                Text {
                    text: "Clear all"
                    visible: notifications.count > 0
                    color: ChiTheme.colors.primary
                    font {
                        pixelSize: ChiTheme.typography.labelLarge.size
                        weight: ChiTheme.typography.labelLarge.weight
                        family: ChiTheme.fontFamily
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: notifications.dismissAll()
                    }
                }
            }
        }

        Flickable {
            anchors.top: header.bottom
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16
            contentHeight: contentCol.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            Column {
                id: contentCol
                width: parent.width
                spacing: 4

                Text {
                    visible: notifications.count === 0
                    text: "No notifications"
                    color: ChiTheme.colors.onSurfaceVariant
                    font {
                        pixelSize: ChiTheme.typography.bodyMedium.size
                        family: ChiTheme.fontFamily
                    }
                    opacity: 0.6
                    topPadding: 60
                }

                Repeater {
                    model: notifications

                    delegate: NotificationDelegate {
                        width: contentCol.width
                        nId: model.notifId
                        nAppName: model.appName
                        nAppIcon: model.appIcon || ""
                        nResolvedIcon: model.resolvedIcon || ""
                        nDesktopEntry: model.desktopEntry || ""
                        nSummary: model.summary
                        nBody: model.body || ""
                        nActions: model.actions || []
                        nTimestamp: model.timestamp
                        nUrgency: model.urgency
                        nIsGrouped: model.isGrouped
                        nImagePath: model.imagePath || ""

                        onDismissed: (id) => notifications.dismiss(id)
                        onActionInvoked: (id, key) => notifications.invokeAction(id, key)
                        onSnoozeRequested: (id) => notifications.snooze(id)
                    }
                }
            }
        }
    }

    Keys.onEscapePressed: shell.notificationCenterOpen = false
}
