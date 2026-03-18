import QtQuick
import Chi 1.0
import "../"

Item {
    id: tctx
    anchors.fill: parent
    z: 200
    visible: _open

    property int  popupH: 250
    property int  totalWidth: 800
    property bool autoHideEnabled: false

    property bool _open: false
    property real _targetX: 0

    signal toggleAutoHide()
    signal showDesktop()
    signal showOverview()

    function open(gx) {
        _targetX = gx - card.width / 2
        _open = true
    }
    function close() { _open = false }

    // Dismiss layer — identical to AppContextMenu
    MouseArea {
        anchors.fill: parent; z: 0
        onClicked: tctx.close()
    }

    Rectangle {
        id: card; width: 240; z: 10
        radius: 16
        color: ChiTheme.colors.surfaceContainerHighest
        border.color: Qt.rgba(1,1,1,0.06); border.width: 1

        x: Math.max(4, Math.min(tctx._targetX, tctx.totalWidth - width - 4))
        y: tctx.popupH - height - 8
        height: col.height + 16

        scale: tctx._open ? 1.0 : 0.92
        opacity: tctx._open ? 1.0 : 0.0
        transformOrigin: Item.Bottom
        Behavior on scale   { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 120 } }

        // Block clicks from reaching dismiss — identical to AppContextMenu
        MouseArea { anchors.fill: parent; z: 0 }

        Column {
            id: col; x: 8; y: 8; width: parent.width - 16; spacing: 0; z: 1

            CtxItem {
                icon: "desktop_windows"; label: "Show Desktop"
                onClicked: { tctx.showDesktop(); tctx.close() }
            }

            CtxItem {
                icon: "view_cozy"; label: "Task Overview"
                onClicked: { tctx.showOverview(); tctx.close() }
            }

            Rectangle { width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06) }

            CtxItem {
                icon: tctx.autoHideEnabled ? "visibility_off" : "visibility"
                label: "Auto-hide Taskbar"
                checked: tctx.autoHideEnabled
                onClicked: { tctx.toggleAutoHide(); tctx.close() }
            }

            Rectangle { width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06) }

            CtxItem {
                icon: "tune"; label: "Taskbar Settings"
                onClicked: { console.log("TODO: open taskbar settings"); tctx.close() }
            }
        }
    }
}
