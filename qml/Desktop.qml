// Desktop — background layer surface
// Spec section 5.8: wallpaper + optional widgets

import QtQuick
import Chi

Rectangle {
    id: desktop
    anchors.fill: parent
    color: ChiTheme.colors.background

    // wallpaper image
    Image {
        id: wallpaper
        anchors.fill: parent
        source: ""  // TODO: load from config
        fillMode: Image.PreserveAspectCrop
        visible: source !== ""
    }

    // centered clock widget (MVP)
    Column {
        anchors.centerIn: parent
        spacing: 8

        Text {
            id: clockText
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatTime(new Date(), "h:mm")
            color: ChiTheme.colors.onBackground
            font.pixelSize: ChiTheme.typography.displayLarge.size
            font.weight: ChiTheme.typography.displayLarge.weight
            font.family: ChiTheme.fontFamily
        }

        Text {
            id: dateText
            anchors.horizontalCenter: parent.horizontalCenter
            text: Qt.formatDate(new Date(), "dddd, MMMM d")
            color: ChiTheme.colors.onSurfaceVariant
            font.pixelSize: ChiTheme.typography.titleMedium.size
            font.weight: ChiTheme.typography.titleMedium.weight
            font.family: ChiTheme.fontFamily
        }
    }

    // update clock every second
    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            clockText.text = Qt.formatTime(new Date(), "h:mm")
            dateText.text = Qt.formatDate(new Date(), "dddd, MMMM d")
        }
    }
}
