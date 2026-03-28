import QtQuick
import QtQuick.Layouts
import Chi 1.0

Rectangle {
    id: statusBar
    property var session: null

    color: ChiTheme.colors.surfaceContainerLow

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: ChiTheme.colors.outlineVariant
        opacity: 0.15
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: session ? session.title : ""
            font.family: ChiTheme.typography.labelSmall.family
            font.pixelSize: ChiTheme.typography.labelSmall.size
            color: ChiTheme.colors.onSurfaceVariant
            elide: Text.ElideMiddle
            opacity: 0.6
        }

        Row {
            spacing: 6
            visible: session !== null

            Rectangle {
                width: 6; height: 6; radius: 3
                anchors.verticalCenter: parent.verticalCenter
                color: session && session.running
                       ? ChiTheme.colors.primary : ChiTheme.colors.error
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
