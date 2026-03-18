import QtQuick
import Chi 1.0
import "../"

Item {
    id: tipRoot
    anchors.fill: parent
    z: 100

    property int popupH: 250
    property int totalWidth: 800

    property int    _tipIdx: -1
    property string _tipText: ""
    property real   _tipX: 0
    property bool   _tipVis: false

    Timer { id: _tipDelay; interval: 400; onTriggered: if (tipRoot._tipIdx >= 0) tipRoot._tipVis = true }
    Timer { id: _tipHide;  interval: 100; onTriggered: if (tipRoot._tipIdx < 0) tipRoot._tipVis = false }

    function showTip(idx, text, gx) {
        _tipHide.stop(); _tipVis = false
        _tipIdx = idx; _tipText = text; _tipX = gx
        _tipDelay.restart()
    }

    function hideTip(idx) {
        if (_tipIdx !== idx) return
        _tipIdx = -1; _tipDelay.stop(); _tipHide.restart()
    }

    Item {
        visible: tipRoot._tipVis
        x: Math.max(4, Math.min(tipRoot._tipX - tipBody.width / 2, tipRoot.totalWidth - tipBody.width - 4))
        y: tipRoot.popupH - tipBody.height - 14

        Rectangle {
            id: tipBody; z: 1
            width: tipLbl.implicitWidth + 16; height: 24; radius: 8
            color: ChiTheme.colors.inverseSurface
            Text {
                id: tipLbl
                anchors.centerIn: parent
                text: tipRoot._tipText
                color: ChiTheme.colors.inverseOnSurface
                font.pixelSize: ChiTheme.typography.bodySmall.size
                font.weight: 500
                font.family: ChiTheme.fontFamily
            }
        }
        Rectangle {
            z: 0; width: 10; height: 10; rotation: 45
            color: ChiTheme.colors.inverseSurface
            x: Math.max(8, Math.min(tipRoot._tipX - parent.x - 5, tipBody.width - 18))
            y: tipBody.height - 5
        }
    }
}
