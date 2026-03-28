import QtQuick
import QtQuick.Layouts
import Chi 1.0

Rectangle {
    id: statusBar

    property var session: null

    // Slightly elevated from the terminal so it reads as a bar,
    // but still in the same color family
    color: ChiTheme.colors.surfaceContainerLow

    // Subtle top separator
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: ChiTheme.colors.outlineVariant
        opacity: 0.2
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        // ── Left: title / cwd ───────────────────────────────
        Text {
            Layout.fillWidth: true
            text: session ? session.title : ""
            font.family: ChiTheme.typography.labelSmall.family
            font.pixelSize: ChiTheme.typography.labelSmall.size
            color: ChiTheme.colors.onSurfaceVariant
            elide: Text.ElideMiddle
            opacity: 0.7
        }

        // ── Right: status indicator ─────────────────────────
        Row {
            spacing: 6
            visible: session !== null

            // Dot
            Rectangle {
                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: session && session.running
                       ? ChiTheme.colors.primary
                       : ChiTheme.colors.error

                // Subtle pulse for active sessions
                SequentialAnimation on opacity {
                    running: session !== null && session.running
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.4; duration: 1500; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 1500; easing.type: Easing.InOutSine }
                }
            }

            Text {
                text: session && session.running ? "running" : "exited"
                font.family: ChiTheme.typography.labelSmall.family
                font.pixelSize: 10
                color: ChiTheme.colors.onSurfaceVariant
                opacity: 0.5
            }
        }
    }
}
