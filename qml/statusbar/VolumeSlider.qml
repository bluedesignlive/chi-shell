import QtQuick
import Chi 1.0

Item {
    id: slider
    width: parent.width
    height: 52

    property real value: systemStatus.volume / 100.0

    Item {
        anchors.centerIn: parent
        width: parent.width
        height: 52

        // ── Active track (left) ──────────────────────
        Rectangle {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(12, parent.width * slider.value - 5)
            height: 40
            radius: 12
            color: ChiTheme.colors.primary

            Rectangle {
                anchors.right: parent.right
                anchors.top: parent.top
                width: Math.min(12, parent.width)
                height: parent.height
                color: parent.color
            }

            Behavior on width { SpringAnimation { spring: 14; damping: 1.0 } }
        }

        // ── Handle ───────────────────────────────────
        Rectangle {
            x: Math.max(0, Math.min(parent.width - 4, parent.width * slider.value - 2))
            anchors.verticalCenter: parent.verticalCenter
            width: 4; height: 52; radius: 2
            color: ChiTheme.colors.primary

            Behavior on x { SpringAnimation { spring: 14; damping: 1.0 } }
        }

        // ── Inactive track (right) ───────────────────
        Rectangle {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(12, parent.width * (1.0 - slider.value) - 5)
            height: 40
            radius: 12
            color: ChiTheme.colors.secondaryContainer

            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                width: Math.min(12, parent.width)
                height: parent.height
                color: parent.color
            }

            Behavior on width { SpringAnimation { spring: 14; damping: 1.0 } }

            Icon {
                source: systemStatus.audioIcon; size: 24
                color: ChiTheme.colors.onSurfaceVariant
                anchors.right: parent.right
                anchors.rightMargin: 10
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onPressed: (mouse) => {
            systemStatus.volume = Math.round(Math.max(0, Math.min(100, mouse.x / width * 100)))
        }
        onPositionChanged: (mouse) => {
            systemStatus.volume = Math.round(Math.max(0, Math.min(100, mouse.x / width * 100)))
        }
    }
}
