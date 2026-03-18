import QtQuick
import Chi 1.0
import "../"

Rectangle {
    id: ci
    width: parent ? parent.width : 200
    height: 40; radius: 8
    color: _h.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"

    property string icon: ""
    property string label: ""
    property bool isDestructive: false
    property bool checked: false

    signal clicked()

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left; anchors.leftMargin: 12
        spacing: 12

        Icon {
            source: ci.icon; size: 20
            anchors.verticalCenter: parent.verticalCenter
            color: ci.isDestructive ? ChiTheme.colors.error : ChiTheme.colors.onSurface
        }

        Text {
            text: ci.label
            anchors.verticalCenter: parent.verticalCenter
            color: ci.isDestructive ? ChiTheme.colors.error : ChiTheme.colors.onSurface
            font.pixelSize: ChiTheme.typography.bodyMedium.size
            font.family: ChiTheme.fontFamily
            elide: Text.ElideRight
        }
    }

    Icon {
        source: "check"; size: 16; visible: ci.checked
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right; anchors.rightMargin: 12
        color: ChiTheme.colors.primary
    }

    MouseArea {
        id: _h; anchors.fill: parent; hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: ci.clicked()
    }
}
