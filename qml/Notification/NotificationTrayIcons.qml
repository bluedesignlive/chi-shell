import QtQuick
import Chi 1.0

Row {
    id: root
    spacing: 4
    property int maxIcons: 4
    visible: notifications.trayIcons && notifications.trayIcons.length > 0

    Repeater {
        model: notifications.trayIcons || []

        delegate: Item {
            width: visible ? 16 : 0
            height: 16
            visible: index < root.maxIcons

            readonly property string iconValue: modelData || ""
            readonly property bool isPath: iconValue.indexOf("file://") === 0
                                        || iconValue.indexOf("/") === 0

            // File path: original image
            Loader {
                anchors.centerIn: parent
                active: isPath
                sourceComponent: Image {
                    width: 14; height: 14
                    source: iconValue.indexOf("file://") === 0
                        ? iconValue : "file://" + iconValue
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                }
            }

            // Named icon: try theme first, safe material fallback
            Loader {
                anchors.centerIn: parent
                active: !isPath && iconValue !== ""
                sourceComponent: Item {
                    width: 14; height: 14

                    Image {
                        id: trayTheme
                        anchors.fill: parent
                        source: "image://icon/" + iconValue
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        sourceSize: Qt.size(14, 14)
                        visible: status === Image.Ready
                    }

                    Icon {
                        anchors.centerIn: parent
                        visible: trayTheme.status === Image.Error
                        source: "notifications"
                        size: 14
                        color: ChiTheme.colors.onSurface
                    }
                }
            }
        }
    }

    Text {
        visible: notifications.trayIcons && notifications.trayIcons.length > root.maxIcons
        text: "+" + (notifications.trayIcons.length - root.maxIcons)
        color: ChiTheme.colors.onSurface
        font { pixelSize: 10; weight: 600; family: ChiTheme.fontFamily }
        anchors.verticalCenter: parent.verticalCenter
        opacity: 0.65
    }
}
