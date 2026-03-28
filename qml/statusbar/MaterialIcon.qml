import QtQuick
import Chi 1.0

Text {
    property string icon: ""
    property real size: 24

    text: icon
    font.family: "Material Icons"
    font.pixelSize: size
    color: ChiTheme.colors.onSurface
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    visible: icon !== ""
}
