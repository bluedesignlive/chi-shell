import QtQuick
import QtQuick.Layouts
import Chi 1.0

Rectangle {
    id: statusBar

    property var session: null

    color: ChiTheme.colors.surfaceContainer
    height: 24

    // Top border
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: ChiTheme.colors.outlineVariant
        opacity: 0.3
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 16

        // Left: session info
        Text {
            Layout.fillWidth: true
            text: session ? session.title : "No session"
            font.family: ChiTheme.typography.labelSmall.family
            font.pixelSize: ChiTheme.typography.labelSmall.size
            color: ChiTheme.colors.onSurfaceVariant
            elide: Text.ElideMiddle
            opacity: 0.8
        }

        // Right: running indicator
        Row {
            spacing: 4
            visible: session !== null

            Rectangle {
                width: 6
                height: 6
                radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: session && session.running
                       ? ChiTheme.colors.primary
                       : ChiTheme.colors.error
            }

            Text {
                text: session && session.running ? "running" : "exited"
                font.family: ChiTheme.typography.labelSmall.family
                font.pixelSize: ChiTheme.typography.labelSmall.size
                color: ChiTheme.colors.onSurfaceVariant
                opacity: 0.7
            }
        }
    }
}
