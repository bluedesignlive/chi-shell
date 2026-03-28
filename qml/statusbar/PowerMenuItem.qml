import QtQuick
import Chi 1.0

Rectangle {
    id: item

    property string icon: ""
    property string label: ""
    property color iconColor: ChiTheme.colors.onSurfaceVariant

    signal clicked()

    width: parent.width
    height: 48
    radius: 12
    color: itemMouse.containsMouse
           ? ChiTheme.colors.surfaceContainerHigh
           : "transparent"

    Behavior on color { ColorAnimation { duration: 100 } }

    Row {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 12

        Icon {
            source: item.icon; size: 22
            color: item.iconColor
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: item.label
            color: ChiTheme.colors.onSurface
            font { pixelSize: 15; weight: 400; family: ChiTheme.fontFamily }
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    MouseArea {
        id: itemMouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: item.clicked()
    }
}
