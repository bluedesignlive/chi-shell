import QtQuick
import Chi 1.0

Rectangle {
    id: root
    anchors.fill: parent
    color: "transparent"

    property bool showWifiPanel: false
    property bool showPowerMenu: false

    // ── Scrim ────────────────────────────────────────
    Rectangle {
        id: scrim
        anchors.fill: parent
        color: ChiTheme.colors.scrim
        opacity: 0

        MouseArea {
            anchors.fill: parent
            onClicked: {
                if (root.showPowerMenu) root.showPowerMenu = false
                else if (root.showWifiPanel) root.showWifiPanel = false
                else shell.quickSettingsOpen = false
            }
        }
    }

    // ── Panel ────────────────────────────────────────
    Rectangle {
        id: panel
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 40; anchors.rightMargin: 8
        width: 412
        height: Math.min(implicitPanelH, parent.height - 104)
        radius: 28
        color: ChiTheme.colors.surface
        border.width: 1
        border.color: ChiTheme.colors.outlineVariant
        clip: true
        visible: !root.showPowerMenu

        opacity: 0
        transform: Translate { id: panelSlide; y: -20 }

        readonly property real implicitPanelH:
            showWifiPanel ? parent.height - 104
                          : mainContent.height + 32

        MouseArea { anchors.fill: parent }

        Flickable {
            id: mainFlickable
            anchors.fill: parent; anchors.margins: 16
            contentHeight: mainContent.height
            flickableDirection: Flickable.VerticalFlick
            boundsBehavior: Flickable.StopAtBounds
            visible: !root.showWifiPanel

            Column {
                id: mainContent
                width: mainFlickable.width
                spacing: 12

                StatusHeader {}
                BrightnessSlider {}
                VolumeSlider {}
                TileGrid {
                    onOpenWifiPanel: root.showWifiPanel = true
                }
                MediaPlayerCard {}
                QSFooter {
                    onOpenPowerMenu: root.showPowerMenu = true
                }
            }
        }

        WiFiPanel {
            anchors.fill: parent; anchors.margins: 16
            visible: root.showWifiPanel
            onClose: root.showWifiPanel = false
        }
    }

    // ── Power menu (full overlay) ────────────────────
    Loader {
        anchors.fill: parent
        active: root.showPowerMenu
        sourceComponent: PowerMenu {
            anchors.fill: parent
            onClose: root.showPowerMenu = false
        }
    }

    // ── Open animation ───────────────────────────────
    ParallelAnimation {
        id: openAnim

        NumberAnimation {
            target: panel; property: "opacity"
            from: 0; to: 1
            duration: ChiTheme.motion.durationMedium
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: panelSlide; property: "y"
            from: -20; to: 0
            duration: ChiTheme.motion.durationMedium
            easing.type: Easing.OutCubic
        }
        NumberAnimation {
            target: scrim; property: "opacity"
            from: 0; to: 0.32
            duration: ChiTheme.motion.durationMedium
            easing.type: Easing.OutCubic
        }
    }

    Component.onCompleted: openAnim.start()

    Keys.onEscapePressed: {
        if (root.showPowerMenu) root.showPowerMenu = false
        else if (root.showWifiPanel) root.showWifiPanel = false
        else shell.quickSettingsOpen = false
    }
}
