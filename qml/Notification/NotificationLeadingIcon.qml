import QtQuick
import Chi 1.0

Item {
    id: root
    width: 40
    height: 40

    property string iconSource: ""
    property color accentColor: ChiTheme.colors.primary

    readonly property bool isFilePath: iconSource.indexOf("file://") === 0
                                    || iconSource.indexOf("/") === 0

    // Known generic freedesktop names that will never exist in image://icon/
    readonly property bool isGenericName: {
        var s = iconSource
        return s === "" || s === "dialog-information" || s === "dialog-info"
            || s === "dialog-warning" || s === "dialog-warn"
            || s === "dialog-error" || s === "dialog-critical"
            || s === "dialog-question" || s === "dialog-password"
            || s === "notifications"
    }

    // Map generic names to safe material icons
    readonly property string materialName: {
        var s = iconSource
        if (s === "dialog-information" || s === "dialog-info") return "info"
        if (s === "dialog-warning" || s === "dialog-warn") return "warning"
        if (s === "dialog-error" || s === "dialog-critical") return "error"
        if (s === "dialog-question") return "help"
        if (s === "dialog-password") return "lock"
        return "notifications"
    }

    Rectangle {
        anchors.fill: parent
        radius: 20
        color: root.accentColor
        opacity: 0.14
    }

    // ── File path: original image ──
    Loader {
        anchors.centerIn: parent
        active: root.isFilePath
        sourceComponent: Image {
            width: 24; height: 24
            source: root.iconSource.indexOf("file://") === 0
                ? root.iconSource : "file://" + root.iconSource
            fillMode: Image.PreserveAspectFit
            smooth: true
            sourceSize: Qt.size(24, 24)
        }
    }

    // ── Generic name: go straight to material icon, no image://icon/ attempt ──
    Loader {
        anchors.centerIn: parent
        active: root.isGenericName
        sourceComponent: Icon {
            source: root.materialName
            size: 24
            color: ChiTheme.colors.onSurface
        }
    }

    // ── App name: try freedesktop theme first, fallback to material bell ──
    Loader {
        anchors.centerIn: parent
        active: !root.isFilePath && !root.isGenericName && root.iconSource !== ""
        sourceComponent: Item {
            width: 24; height: 24

            Image {
                id: themeIcon
                anchors.fill: parent
                source: "image://icon/" + root.iconSource
                fillMode: Image.PreserveAspectFit
                smooth: true
                sourceSize: Qt.size(24, 24)
                visible: status === Image.Ready
            }

            Icon {
                anchors.centerIn: parent
                visible: themeIcon.status === Image.Error
                source: "notifications"
                size: 24
                color: ChiTheme.colors.onSurface
            }
        }
    }
}
