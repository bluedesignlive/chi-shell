// StatusBar — top layer surface
// Spec section 5.1: time + date (left), status icons (right)
// Height: 36px, font: labelLarge (14px, weight 500)

import QtQuick
import Chi

Rectangle {
    id: statusBar
    anchors.fill: parent
    color: ChiTheme.colors.surfaceContainer
    opacity: 0.95

    property var typo: ChiTheme.typography.labelLarge

    // ── left: time + date ────────────────────────────────
    Row {
        id: leftGroup
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Text {
            id: timeLabel
            text: Qt.formatTime(new Date(), "h:mm")
            color: ChiTheme.colors.onSurface
            font.pixelSize: statusBar.typo.size
            font.weight: statusBar.typo.weight
            font.family: ChiTheme.fontFamily
        }

        Text {
            id: dateLabel
            text: Qt.formatDate(new Date(), "ddd, MMM d")
            color: ChiTheme.colors.onSurfaceVariant
            font.pixelSize: statusBar.typo.size
            font.weight: statusBar.typo.weight
            font.family: ChiTheme.fontFamily
        }
    }

    // ── right: system status ─────────────────────────────
    Row {
        id: rightGroup
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        // settings icons (alarm, vpn, vibrate)
        // shown only when relevant — simplified for MVP

        // wifi icon
        Icon {
            source: systemStatus.networkIcon
            size: 18
            color: ChiTheme.colors.onSurfaceVariant
        }

        // battery icon
        Icon {
            source: systemStatus.batteryIcon
            size: 18
            color: ChiTheme.colors.onSurfaceVariant
        }

        // battery percentage
        Text {
            text: systemStatus.batteryPercent + "%"
            color: ChiTheme.colors.onSurfaceVariant
            font.pixelSize: statusBar.typo.size
            font.weight: statusBar.typo.weight
            font.family: ChiTheme.fontFamily
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // ── tap to open quick settings ───────────────────────
    MouseArea {
        anchors.fill: rightGroup
        anchors.margins: -8
        cursorShape: Qt.PointingHandCursor
        onClicked: shell.quickSettingsOpen = !shell.quickSettingsOpen
    }

    // ── clock timer ──────────────────────────────────────
    Timer {
        interval: 1000
        running: true
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            timeLabel.text = Qt.formatTime(new Date(), "h:mm")
            dateLabel.text = Qt.formatDate(new Date(), "ddd, MMM d")
        }
    }
}
