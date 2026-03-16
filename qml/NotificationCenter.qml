import QtQuick
import Chi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    // ── scrim — click to close ───────────────────────────
    MouseArea {
        anchors.fill: parent
        onClicked: shell.notificationCenterOpen = false
    }

    // ── panel ────────────────────────────────────────────
    Rectangle {
        id: panel
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 40
        anchors.rightMargin: 8
        width: 412
        height: Math.min(500, parent.height - 104)
        radius: 28
        color: ChiTheme.colors.secondaryContainer
        opacity: 0.97
        clip: true

        MouseArea { anchors.fill: parent }

        Item {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 16
            height: 40

            Text {
                text: "Notifications"
                color: ChiTheme.colors.onSurfaceVariant
                font.pixelSize: ChiTheme.typography.titleSmall.size
                font.weight: ChiTheme.typography.titleSmall.weight
                font.family: ChiTheme.fontFamily
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: "Clear all"
                color: ChiTheme.colors.primary
                font.pixelSize: ChiTheme.typography.labelLarge.size
                font.weight: ChiTheme.typography.labelLarge.weight
                font.family: ChiTheme.fontFamily
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                visible: notifications.count > 0

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: notifications.dismissAll()
                }
            }
        }

        ListView {
            id: notifList
            anchors.top: header.bottom; anchors.topMargin: 8
            anchors.left: parent.left; anchors.right: parent.right
            anchors.bottom: parent.bottom; anchors.margins: 16
            spacing: 8; clip: true
            model: notifications

            Text {
                anchors.centerIn: parent
                visible: notifications.count === 0
                text: "No notifications"
                color: ChiTheme.colors.onSurfaceVariant
                font.pixelSize: ChiTheme.typography.bodyMedium.size
                font.family: ChiTheme.fontFamily
                opacity: 0.6
            }

            delegate: Item {
                id: notifDelegate
                width: notifList.width
                height: notifCard.height; clip: true

                Rectangle {
                    id: notifCard
                    width: parent.width
                    height: notifContent.height + 24
                    radius: 16
                    color: ChiTheme.colors.surfaceContainerHigh

                    Column {
                        id: notifContent
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.margins: 12; spacing: 4

                        Item {
                            width: parent.width; height: 20

                            Row {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter; spacing: 8
                                Icon { source: model.appIcon || "notifications"; size: 18; color: ChiTheme.colors.onSurfaceVariant; anchors.verticalCenter: parent.verticalCenter }
                                Text { text: model.appName; color: ChiTheme.colors.onSurfaceVariant; font.pixelSize: ChiTheme.typography.bodySmall.size; font.family: ChiTheme.fontFamily; anchors.verticalCenter: parent.verticalCenter }
                            }

                            Text {
                                text: Qt.formatTime(model.timestamp, "h:mm")
                                color: ChiTheme.colors.onSurfaceVariant; font.pixelSize: ChiTheme.typography.bodySmall.size
                                font.family: ChiTheme.fontFamily; opacity: 0.6
                                anchors.right: parent.right; anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        Text { text: model.summary; color: ChiTheme.colors.onSurface; font.pixelSize: ChiTheme.typography.titleSmall.size; font.weight: ChiTheme.typography.titleSmall.weight; font.family: ChiTheme.fontFamily; wrapMode: Text.WordWrap; width: parent.width }
                        Text { text: model.body; visible: text !== ""; color: ChiTheme.colors.onSurfaceVariant; font.pixelSize: ChiTheme.typography.bodyMedium.size; font.family: ChiTheme.fontFamily; wrapMode: Text.WordWrap; width: parent.width; maximumLineCount: 3; elide: Text.ElideRight }
                    }

                    Behavior on x { SpringAnimation { spring: 14.0; damping: 0.75 } }
                }

                MouseArea {
                    anchors.fill: parent
                    drag.target: notifCard; drag.axis: Drag.XAxis
                    drag.minimumX: -parent.width; drag.maximumX: parent.width
                    onReleased: {
                        if (Math.abs(notifCard.x) > parent.width * 0.4)
                            notifications.dismiss(model.notifId)
                        else
                            notifCard.x = 0
                    }
                }
            }

            add: Transition {
                NumberAnimation { properties: "y"; from: -60; duration: 200; easing.type: Easing.OutCubic }
                NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 200 }
            }
            remove: Transition { NumberAnimation { properties: "opacity"; to: 0; duration: 150 } }
        }
    }

    Keys.onEscapePressed: shell.notificationCenterOpen = false
}
