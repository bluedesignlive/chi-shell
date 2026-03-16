import QtQuick
import Chi

Rectangle {
    id: tile

    property string icon: ""
    property string title: ""
    property string description: ""
    property bool active: false
    property bool wide: true

    signal clicked()

    radius: wide ? 20 : 1000
    color: active ? ChiTheme.colors.primaryContainer
                  : ChiTheme.colors.surfaceContainerHigh

    Behavior on color { ColorAnimation { duration: 100 } }

    Row {
        anchors.fill: parent
        anchors.leftMargin: wide ? 8 : 0
        anchors.rightMargin: wide ? 12 : 0
        spacing: 4

        Item {
            width: 56; height: 56
            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                anchors.centerIn: parent
                width: 56; height: 56
                radius: active && wide ? 1000 : (wide ? 16 : 1000)
                color: active ? "transparent"
                              : (wide ? ChiTheme.colors.primaryContainer : "transparent")
                visible: wide
            }

            Icon {
                anchors.centerIn: parent
                source: tile.icon
                size: 32
                color: active ? (wide ? ChiTheme.colors.onPrimaryContainer : "#FFFFFF")
                              : ChiTheme.colors.onSurface
            }
        }

        Column {
            visible: wide
            anchors.verticalCenter: parent.verticalCenter
            spacing: 0

            Text {
                text: tile.title
                color: active ? ChiTheme.colors.onPrimaryContainer
                              : ChiTheme.colors.onSurface
                font.pixelSize: active ? 16 : 14
                font.weight: active ? 700 : 500
                font.family: ChiTheme.fontFamily
                elide: Text.ElideRight
                width: Math.max(10, tile.width - 80)
            }

            Text {
                text: tile.description
                visible: text !== ""
                color: active ? ChiTheme.colors.onPrimaryContainer
                              : ChiTheme.colors.onSurfaceVariant
                font.pixelSize: 14; font.weight: 400
                font.family: ChiTheme.fontFamily
                elide: Text.ElideRight
                width: Math.max(10, tile.width - 80)
                opacity: 0.85
            }
        }
    }

    scale: tileMouseArea.pressed ? 0.95 : 1.0
    Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }

    MouseArea {
        id: tileMouseArea
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.clicked()
    }
}
