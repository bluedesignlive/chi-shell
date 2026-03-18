import QtQuick
import Chi 1.0
import "../"

Item {
    id: actx
    anchors.fill: parent
    visible: _open

    property int  popupH: 250
    property int  totalWidth: 800
    property int  barH: 52

    property bool   _open: false
    property string _appId: ""
    property string _name: ""
    property bool   _isRunning: false
    property bool   _isPinned: false
    property int    _pinIdx: -1
    property int    _groupIdx: -1
    property int    _srcWinIdx: -1
    property real   _targetX: 0

    property bool _winMinimized:  _srcWinIdx >= 0 && _open ? windowTracker.isMinimizedAt(_srcWinIdx) : false
    property bool _winMaximized:  _srcWinIdx >= 0 && _open ? windowTracker.isMaximizedAt(_srcWinIdx) : false
    property bool _winFullscreen: _srcWinIdx >= 0 && _open ? windowTracker.isFullscreenAt(_srcWinIdx) : false
    property int  _winCount:      _isRunning && _open ? windowTracker.windowCountForApp(_appId) : 0

    property int _pinVer: 0
    Connections {
        target: pinnedApps
        function onCountChanged() { actx._pinVer++ }
    }
    readonly property bool _pinState: {
        void(actx._pinVer)
        return _appId !== "" && pinnedApps.isPinned(_appId)
    }

    Connections {
        target: windowTracker
        function onDataChanged() {
            if (!actx._open) return
            if (actx._srcWinIdx >= 0) {
                actx._winMinimized  = windowTracker.isMinimizedAt(actx._srcWinIdx)
                actx._winMaximized  = windowTracker.isMaximizedAt(actx._srcWinIdx)
                actx._winFullscreen = windowTracker.isFullscreenAt(actx._srcWinIdx)
            }
            actx._winCount = windowTracker.windowCountForApp(actx._appId)
        }
        function onCountChanged() {
            if (!actx._open) return
            actx._winCount = windowTracker.windowCountForApp(actx._appId)
            if (actx._winCount === 0 && actx._isRunning) actx.close()
        }
    }

    function openForPinned(appId, name, isRunning, isPinned, pinIdx, gx) {
        _appId = appId; _name = name; _isRunning = isRunning
        _isPinned = isPinned; _pinIdx = pinIdx; _groupIdx = -1
        _srcWinIdx = isRunning ? windowTracker.firstIndexForApp(appId) : -1
        _targetX = gx - card.width / 2
        _refreshState()
        _open = true
    }

    function openForWindow(appId, title, isPinned, groupIdx, gx) {
        _appId = appId; _name = title; _isRunning = true
        _isPinned = isPinned; _pinIdx = -1; _groupIdx = groupIdx
        _srcWinIdx = groupedWindows.firstWindowIndex(groupIdx)
        _targetX = gx - card.width / 2
        _refreshState()
        _open = true
    }

    function close() { _open = false }

    function _refreshState() {
        if (_srcWinIdx >= 0) {
            _winMinimized  = windowTracker.isMinimizedAt(_srcWinIdx)
            _winMaximized  = windowTracker.isMaximizedAt(_srcWinIdx)
            _winFullscreen = windowTracker.isFullscreenAt(_srcWinIdx)
        } else {
            _winMinimized = false; _winMaximized = false; _winFullscreen = false
        }
        _winCount = windowTracker.windowCountForApp(_appId)
    }

    MouseArea {
        anchors.fill: parent; z: 0
        onClicked: actx.close()
    }

    Rectangle {
        id: card; width: 240; z: 10
        radius: 16
        color: ChiTheme.colors.surfaceContainerHighest
        border.color: Qt.rgba(1,1,1,0.06); border.width: 1

        x: Math.max(4, Math.min(actx._targetX, actx.totalWidth - width - 4))
        y: actx.popupH - height - 8
        height: col.height + 16

        scale: actx._open ? 1.0 : 0.92
        opacity: actx._open ? 1.0 : 0.0
        transformOrigin: Item.Bottom
        Behavior on scale   { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 120 } }

        MouseArea { anchors.fill: parent; z: 0 }

        Column {
            id: col; x: 8; y: 8; width: parent.width - 16; spacing: 0; z: 1

            // Header
            Item {
                width: parent.width; height: 44
                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left; anchors.leftMargin: 8; spacing: 10
                    AppIcon {
                        iconName: actx._appId; size: 24
                        anchors.verticalCenter: parent.verticalCenter
                        fallbackText: actx._name ? actx._name[0].toUpperCase() : "?"
                    }
                    Text {
                        text: actx._name || actx._appId
                        color: ChiTheme.colors.onSurface; font.pixelSize: 13
                        font.weight: 600; font.family: ChiTheme.fontFamily
                        elide: Text.ElideRight; width: col.width - 60
                        anchors.verticalCenter: parent.verticalCenter
                    }
                }
            }

            Rectangle { width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06) }

            CtxItem {
                icon: "add"; label: "New Window"
                visible: pinnedApps.execForApp(actx._appId) !== ""
                onClicked: { pinnedApps.launchNew(actx._appId); actx.close() }
            }

            Rectangle {
                width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06)
                visible: actx._isRunning
            }

            CtxItem {
                icon: actx._winMinimized ? "open_in_full" : "minimize"
                label: actx._winMinimized ? "Restore" : "Minimize"
                visible: actx._isRunning && actx._srcWinIdx >= 0
                onClicked: {
                    windowTracker.setMinimized(actx._srcWinIdx, !actx._winMinimized)
                    actx.close()
                }
            }

            CtxItem {
                icon: actx._winMaximized ? "close_fullscreen" : "open_in_full"
                label: actx._winMaximized ? "Restore Down" : "Maximize"
                visible: actx._isRunning && actx._srcWinIdx >= 0
                onClicked: {
                    windowTracker.setMaximized(actx._srcWinIdx, !actx._winMaximized)
                    actx.close()
                }
            }

            CtxItem {
                icon: actx._winFullscreen ? "fullscreen_exit" : "fullscreen"
                label: actx._winFullscreen ? "Exit Fullscreen" : "Fullscreen"
                visible: actx._isRunning && actx._srcWinIdx >= 0
                onClicked: {
                    windowTracker.setFullscreen(actx._srcWinIdx, !actx._winFullscreen)
                    actx.close()
                }
            }

            Rectangle { width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06) }

            CtxItem {
                icon: actx._pinState ? "keep_off" : "push_pin"
                label: actx._pinState ? "Unpin from Taskbar" : "Pin to Taskbar"
                onClicked: {
                    if (actx._pinState) pinnedApps.unpin(actx._appId)
                    else pinnedApps.pin(actx._appId)
                    actx.close()
                }
            }

            Rectangle {
                width: parent.width; height: 1; color: Qt.rgba(1,1,1,0.06)
                visible: actx._isRunning
            }

            CtxItem {
                icon: "close"; label: "Close"
                isDestructive: true; visible: actx._isRunning
                onClicked: {
                    if (actx._srcWinIdx >= 0) windowTracker.close(actx._srcWinIdx)
                    actx.close()
                }
            }

            CtxItem {
                icon: "delete_sweep"; label: "Close All (" + actx._winCount + ")"
                isDestructive: true; visible: actx._isRunning && actx._winCount > 1
                onClicked: {
                    windowTracker.closeAllForApp(actx._appId)
                    actx.close()
                }
            }
        }
    }
}
