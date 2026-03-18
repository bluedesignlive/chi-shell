import QtQuick
import Chi 1.0
import "../"

Item {
    id: preview
    anchors.fill: parent
    z: 120
    visible: _open

    property int popupH: 250
    property int totalWidth: 800

    property bool   _open: false
    property string _appId: ""
    property real   _targetX: 0
    property var    _windows: []
    property int    _tick: 0

    // WindowTracker role constants (Qt::UserRole + 1 = 257)
    readonly property int _roleAppId:     257
    readonly property int _roleTitle:     258
    readonly property int _roleActivated: 259
    readonly property int _roleMinimized: 260
    readonly property int _roleIconName:  263

    function _buildWindowList() {
        var list = []
        for (var i = 0; i < windowTracker.count; i++) {
            var idx = windowTracker.index(i, 0)
            var aid = windowTracker.data(idx, _roleAppId)
            if (aid === _appId) {
                list.push({
                    srcIndex:  i,
                    title:     windowTracker.data(idx, _roleTitle) || aid,
                    activated: windowTracker.data(idx, _roleActivated) || false,
                    minimized: windowTracker.data(idx, _roleMinimized) || false,
                    iconName:  windowTracker.data(idx, _roleIconName) || aid
                })
            }
        }
        _windows = list
    }

    function show(appId, gx) {
        _appId = appId
        _buildWindowList()
        if (_windows.length === 0) return
        var cardW = Math.min(Math.max(_windows.length, 1) * 220 + 16, totalWidth - 32)
        _targetX = gx - cardW / 2
        _open = true
        _requestCaptures()
        _captureTimer.start()
    }

    function hide() {
        _captureTimer.stop()
        _open = false
        _appId = ""
        _windows = []
    }

    function _requestCaptures() {
        if (typeof wayfireIPC !== "undefined" && typeof wayfireIPC.captureViewThumbnails === "function") {
            wayfireIPC.captureViewThumbnails(_appId)
        }
        _tickBump.start()
    }

    Timer {
        id: _tickBump; interval: 400
        onTriggered: preview._tick++
    }

    Timer {
        id: _captureTimer; interval: 2000; repeat: true
        onTriggered: if (preview._open) preview._requestCaptures()
    }

    Timer {
        id: previewDelay; interval: 400
        property string pendingAppId: ""
        property real pendingX: 0
        onTriggered: preview.show(pendingAppId, pendingX)
    }

    function scheduleShow(appId, gx) {
        if (_open && _appId === appId) {
            cancelHide()
            return
        }
        previewDelay.pendingAppId = appId
        previewDelay.pendingX = gx
        previewDelay.restart()
    }

    function scheduleHide() {
        previewDelay.stop()
        if (_open) hideDelay.restart()
    }

    Timer {
        id: hideDelay; interval: 300
        onTriggered: preview.hide()
    }

    function cancelHide() { hideDelay.stop() }

    // Dismiss layer — covers only popup zone above the bar
    MouseArea {
        x: 0; y: 0
        width: parent.width
        height: preview.popupH
        z: 0
        onClicked: preview.hide()
    }

    Rectangle {
        id: previewCard; z: 10
        width: Math.min(Math.max(_windows.length, 1) * 220 + 16, totalWidth - 32)
        height: 190
        radius: 16
        color: ChiTheme.colors.surfaceContainerHighest
        border.color: Qt.rgba(1,1,1,0.06); border.width: 1

        x: Math.max(4, Math.min(preview._targetX, preview.totalWidth - width - 4))
        y: preview.popupH - height - 12

        scale: preview._open ? 1.0 : 0.92
        opacity: preview._open ? 1.0 : 0.0
        transformOrigin: Item.Bottom
        Behavior on scale   { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 150 } }

        // Block clicks from reaching dismiss layer
        MouseArea { anchors.fill: parent; z: 0 }

        // Robust hover — HoverHandler is NOT stolen by child MouseAreas
        HoverHandler {
            id: cardHover
            onHoveredChanged: {
                if (hovered) preview.cancelHide()
                else preview.scheduleHide()
            }
        }

        Row {
            anchors.centerIn: parent; spacing: 8; z: 1

            Repeater {
                model: preview._windows

                Rectangle {
                    id: thumbCard
                    width: 208; height: 166; radius: 12
                    color: modelData.activated ? ChiTheme.colors.primaryContainer
                                               : ChiTheme.colors.surfaceContainerHigh

                    property bool hovered: thumbMA.containsMouse

                    Column {
                        anchors.fill: parent; anchors.margins: 8; spacing: 4

                        Rectangle {
                            width: parent.width; height: 108; radius: 8
                            color: ChiTheme.colors.surfaceContainer
                            clip: true

                            Image {
                                id: thumbImg
                                anchors.fill: parent
                                fillMode: Image.PreserveAspectFit
                                source: preview._tick > 0
                                    ? "file:///tmp/chi-thumb-" + preview._appId
                                      + "-" + index + ".png#" + preview._tick
                                    : ""
                                cache: false
                                asynchronous: true
                                visible: status === Image.Ready
                            }

                            AppIcon {
                                anchors.centerIn: parent
                                iconName: modelData.iconName; size: 48
                                fallbackText: modelData.title ? modelData.title[0].toUpperCase() : "?"
                                visible: thumbImg.status !== Image.Ready
                            }

                            Rectangle {
                                anchors.fill: parent; radius: 8
                                color: Qt.rgba(0,0,0,0.5)
                                visible: modelData.minimized
                                Text {
                                    anchors.centerIn: parent; text: "Minimized"
                                    color: "#FFF"; font.pixelSize: 11
                                    font.family: ChiTheme.fontFamily
                                }
                            }
                        }

                        Row {
                            width: parent.width; spacing: 4; height: 22

                            AppIcon {
                                iconName: modelData.iconName; size: 16
                                fallbackText: ""
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Text {
                                text: modelData.title
                                color: ChiTheme.colors.onSurface
                                font.pixelSize: 11; font.weight: 500
                                font.family: ChiTheme.fontFamily
                                elide: Text.ElideRight
                                width: parent.width - 44
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            Rectangle {
                                width: 20; height: 20; radius: 10
                                anchors.verticalCenter: parent.verticalCenter
                                color: closeMA.containsMouse ? ChiTheme.colors.errorContainer : "transparent"
                                visible: thumbCard.hovered

                                Icon {
                                    anchors.centerIn: parent; source: "close"; size: 12
                                    color: closeMA.containsMouse ? ChiTheme.colors.error
                                                                 : ChiTheme.colors.onSurfaceVariant
                                }

                                MouseArea {
                                    id: closeMA; anchors.fill: parent
                                    hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                    z: 10
                                    onClicked: {
                                        windowTracker.close(modelData.srcIndex)
                                        Qt.callLater(function() {
                                            if (preview._open) {
                                                preview._buildWindowList()
                                                if (preview._windows.length === 0) preview.hide()
                                            }
                                        })
                                    }
                                }
                            }
                        }
                    }

                    MouseArea {
                        id: thumbMA; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        z: -1
                        onClicked: {
                            windowTracker.activate(modelData.srcIndex)
                            preview.hide()
                        }
                        onContainsMouseChanged: {
                            if (containsMouse) preview.cancelHide()
                        }
                    }

                    scale: thumbMA.pressed ? 0.96 : thumbMA.containsMouse ? 1.02 : 1.0
                    Behavior on scale { SpringAnimation { spring: 16; damping: 0.7 } }
                }
            }
        }
    }
}
