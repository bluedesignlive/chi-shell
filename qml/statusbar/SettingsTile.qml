import QtQuick
import Chi 1.0

Rectangle {
    id: tile

    property string icon: ""
    property string title: ""
    property string description: ""
    property bool active: false
    property bool wide: true

    signal clicked()
    signal longPressed()

    radius: wide ? 20 : 1000
    color: active ? ChiTheme.colors.primary
                  : ChiTheme.colors.surfaceContainer

    Behavior on color { ColorAnimation { duration: ChiTheme.motion.durationFast } }

    // ── Wide tile ────────────────────────────────────
    Row {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 12
        spacing: 4
        visible: tile.wide

        Item {
            width: 56; height: parent.height

            Rectangle {
                anchors.centerIn: parent
                width: 56; height: 56; radius: 16
                color: ChiTheme.colors.primary
                visible: !tile.active

                Icon {
                    anchors.centerIn: parent
                    source: tile.icon; size: 24
                    color: ChiTheme.colors.onPrimary
                }
            }

            Icon {
                anchors.centerIn: parent
                source: tile.icon; size: 32
                color: ChiTheme.colors.onPrimary
                visible: tile.active
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width - 68
            spacing: 0

            Text {
                text: tile.title
                width: parent.width; elide: Text.ElideRight
                color: active ? ChiTheme.colors.onPrimary
                              : ChiTheme.colors.onSurface
                font { pixelSize: 14; weight: 500; family: ChiTheme.fontFamily }
            }
            Text {
                text: tile.description
                visible: text !== ""
                width: parent.width; elide: Text.ElideRight
                color: active ? Qt.rgba(ChiTheme.colors.onPrimary.r,
                                        ChiTheme.colors.onPrimary.g,
                                        ChiTheme.colors.onPrimary.b, 0.7)
                              : ChiTheme.colors.onSurfaceVariant
                font { pixelSize: 14; weight: 400; family: ChiTheme.fontFamily }
            }
        }
    }

    // ── Icon-only ────────────────────────────────────
    Icon {
        anchors.centerIn: parent
        source: tile.icon; size: 32
        color: active ? ChiTheme.colors.onPrimary
                      : ChiTheme.colors.onSurfaceVariant
        visible: !tile.wide
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: tile.clicked()
        onPressAndHold: tile.longPressed()
        onPressed: tile.scale = 0.96
        onReleased: tile.scale = 1.0
        onCanceled: tile.scale = 1.0
    }

    Behavior on scale {
        NumberAnimation { duration: 100; easing.type: Easing.OutCubic }
    }
}
