import QtQuick
import Chi 1.0
import "../"

Row {
    id: leftBtns
    spacing: 2

    Repeater {
        model: [
            { icon: "apps",      action: "launcher" },
            { icon: "search",    action: "search"   },
            { icon: "view_cozy", action: "overview"  }
        ]

        Rectangle {
            required property var modelData
            required property int index

            width: 40; height: 40; radius: 20
            color: _ma.pressed ? Qt.rgba(1,1,1,0.1)
                 : _ma.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"

            Icon {
                anchors.centerIn: parent
                source: modelData.icon
                size: 24
                color: ChiTheme.colors.onSurfaceVariant
            }

            scale: _ma.pressed ? 0.9 : 1.0
            Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }

            MouseArea {
                id: _ma
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: function(mouse) {
                    if (mouse.modifiers & Qt.ShiftModifier) { mouse.accepted = false; return }
                    if (modelData.action === "launcher" || modelData.action === "search")
                        shell.appLauncherOpen = !shell.appLauncherOpen
                    else if (modelData.action === "overview")
                        wayfireIPC.toggleScale()
                }
                onPressAndHold: {
                    if (modelData.action === "overview")
                        wayfireIPC.toggleExpo()
                }
            }
        }
    }
}
