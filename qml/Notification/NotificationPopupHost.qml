import QtQuick
import Chi 1.0

Item {
    id: root
    anchors.fill: parent

    NotificationPopup {
        id: popup
        anchors.left: parent.left
        anchors.right: parent.right

        onPopupTapped: {
            console.log("[notif-popup] tapped -> opening center")
            shell.notificationCenterOpen = true
        }

        onPopupActionInvoked: (id, actionKey) => {
            console.log("[notif-popup] action", id, actionKey)
            notifications.invokeAction(id, actionKey)
        }

        onPopupDismissed: {
            console.log("[notif-popup] dismissed")
            if (typeof popupSurface !== "undefined") {
                popupSurface.setInputRegion(Qt.rect(0, 0, 0, 0))
                popupSurface.visible = false
            }
        }
    }

    Connections {
        target: notifications

        function onNotificationPosted(id, appName, summary, body, appIcon, imagePath, urgency, actions) {
            console.log("[notif-popup] posted", id, appName, appIcon, urgency, summary)

            if (typeof popupSurface !== "undefined") {
                popupSurface.visible = true
                popupSurface.setInputRegion(Qt.rect(0, 0, root.width, 180))
            }

            popup.show(id, appName, summary, body, appIcon, imagePath, urgency, actions)
        }
    }
}
