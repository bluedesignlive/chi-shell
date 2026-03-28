import QtQuick
import Chi 1.0

Item {
    id: slider
    width: parent.width
    height: visible ? 52 : 0
    visible: systemStatus.hasBacklight

    property real value: systemStatus.brightness

    Item {
        anchors.centerIn: parent
        width: parent.width
        height: 52

        // ── Active track (left side) ─────────────────
        Rectangle {
            id: activeTrack
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(12, parent.width * slider.value - 5)
            height: 40
            radius: 12
            color: ChiTheme.colors.primary

            // square right edge
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
            id: handle
            x: Math.max(0, Math.min(parent.width - 4, parent.width * slider.value - 2))
            anchors.verticalCenter: parent.verticalCenter
            width: 4; height: 52; radius: 2
            color: ChiTheme.colors.primary

            Behavior on x { SpringAnimation { spring: 14; damping: 1.0 } }
        }

        // ── Inactive track (right side) ──────────────
        Rectangle {
            id: inactiveTrack
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            width: Math.max(12, parent.width * (1.0 - slider.value) - 5)
            height: 40
            radius: 12
            color: ChiTheme.colors.secondaryContainer

            // square left edge
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                width: Math.min(12, parent.width)
                height: parent.height
                color: parent.color
            }

            Behavior on width { SpringAnimation { spring: 14; damping: 1.0 } }

            // ── Trailing icon ────────────────────────
            Icon {
                source: "brightness_medium"; size: 24
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
            systemStatus.brightness = Math.max(0.01, Math.min(1, mouse.x / width))
        }
        onPositionChanged: (mouse) => {
            systemStatus.brightness = Math.max(0.01, Math.min(1, mouse.x / width))
        }
    }
}
