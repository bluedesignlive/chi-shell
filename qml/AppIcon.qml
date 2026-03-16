import QtQuick
import Chi 1.0

Item {
    id: root
    property string iconName: ""
    property int size: 48
    property string fallbackText: "?"

    width: size
    height: size

    Image {
        id: img
        anchors.fill: parent
        source: root.iconName !== "" ? "image://icon/" + root.iconName : ""
        sourceSize: Qt.size(root.size, root.size)
        fillMode: Image.PreserveAspectFit
        visible: status === Image.Ready
    }

    // fallback: first letter
    Text {
        anchors.centerIn: parent
        visible: img.status !== Image.Ready
        text: root.fallbackText
        color: ChiTheme.colors.onSurface
        font.pixelSize: root.size * 0.45
        font.weight: 600
        font.family: ChiTheme.fontFamily
    }
}
