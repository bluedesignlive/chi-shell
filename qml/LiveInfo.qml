// LiveInfo — top layer surface (centered pill)
// Spec section 5.6: contextual status for live activities
// MVP: stub — will show media, timers, recording in v0.2.0

import QtQuick
import Chi

Rectangle {
    id: liveInfo
    anchors.fill: parent
    color: "transparent"

    // pill shape
    Rectangle {
        id: pill
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 4
        width: pillContent.width + 24
        height: 32
        radius: 1000  // full pill
        color: ChiTheme.colors.surfaceContainerHighest
        visible: false  // hidden until activity

        Behavior on width {
            SpringAnimation {
                spring: 7.0   // defaultSpatial
                damping: 0.80
            }
        }

        Row {
            id: pillContent
            anchors.centerIn: parent
            spacing: 6

            Icon {
                id: pillIcon
                source: ""
                size: 18
                color: ChiTheme.colors.primary
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                id: pillLabel
                text: ""
                color: ChiTheme.colors.onSurface
                font.pixelSize: ChiTheme.typography.labelLarge.size
                font.weight: ChiTheme.typography.labelLarge.weight
                font.family: ChiTheme.fontFamily
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
