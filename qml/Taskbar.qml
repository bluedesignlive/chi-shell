import QtQuick
import Chi 1.0

Item {
    id: root
    anchors.fill: parent

    readonly property int popupH: typeof TASKBAR_POPUP_H !== "undefined" ? TASKBAR_POPUP_H : 250
    readonly property int barH:   typeof TASKBAR_BAR_H   !== "undefined" ? TASKBAR_BAR_H   : 52

    // ═══════════════════════════════════════════════════
    // BAR
    // ═══════════════════════════════════════════════════
    Rectangle {
        id: bar
        x: 0; y: root.popupH
        width: root.width; height: root.barH
        color: ChiTheme.colors.surfaceContainer
        opacity: 0.95

        Row {
            id: leftSection
            anchors.left: parent.left; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            Rectangle {
                width: 40; height: 40; radius: 20
                color: _hm.pressed ? Qt.rgba(1,1,1,0.1) : _hm.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
                Icon { anchors.centerIn: parent; source: "apps"; size: 24; color: ChiTheme.colors.onSurfaceVariant }
                scale: _hm.pressed ? 0.9 : 1.0
                Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
                MouseArea { id: _hm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: shell.appLauncherOpen = !shell.appLauncherOpen }
            }

            Rectangle {
                width: 40; height: 40; radius: 20
                color: _sm.pressed ? Qt.rgba(1,1,1,0.1) : _sm.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
                Icon { anchors.centerIn: parent; source: "search"; size: 24; color: ChiTheme.colors.onSurfaceVariant }
                scale: _sm.pressed ? 0.9 : 1.0
                Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
                MouseArea { id: _sm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: shell.appLauncherOpen = !shell.appLauncherOpen }
            }

            Rectangle {
                width: 40; height: 40; radius: 20
                color: _tv.pressed ? Qt.rgba(1,1,1,0.1) : _tv.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
                Icon { anchors.centerIn: parent; source: "view_cozy"; size: 24; color: ChiTheme.colors.onSurfaceVariant }
                scale: _tv.pressed ? 0.9 : 1.0
                Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
                MouseArea { id: _tv; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: wayfireIPC.toggleScale()
                    onPressAndHold: wayfireIPC.toggleExpo() }
            }
        }

        // ── Pinned Apps (with drag reorder) ──────────────
        Item {
            id: pinnedContainer
            anchors.left: leftSection.right; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: Math.min(pinnedRow.width, bar.width * 0.35)
            height: 48
            clip: false

            property int dragFromIndex: -1
            property int dragToIndex: -1

            Row {
                id: pinnedRow
                spacing: 4

                Repeater {
                    id: pinnedRepeater
                    model: pinnedApps

                    Item {
                        id: pd
                        width: 44; height: 48

                        required property int    index
                        required property string appId
                        required property string name
                        required property string iconName
                        required property bool   isRunning
                        required property int    windowCount

                        // Drag state
                        property bool held: false
                        property real dragStartX: 0

                        z: held ? 10 : 1
                        opacity: held ? 0.8 : 1.0

                        Behavior on opacity { NumberAnimation { duration: 150 } }

                        Column {
                            anchors.centerIn: parent; spacing: 2

                            Rectangle {
                                width: 36; height: 36; radius: 10
                                color: pd.isRunning ? ChiTheme.colors.primaryContainer
                                                    : ChiTheme.colors.surfaceContainerHighest
                                anchors.horizontalCenter: parent.horizontalCenter

                                AppIcon {
                                    anchors.centerIn: parent; iconName: pd.iconName; size: 28
                                    fallbackText: pd.name ? pd.name[0].toUpperCase() : "?"
                                }

                                Rectangle {
                                    visible: pd.windowCount > 1
                                    anchors.top: parent.top; anchors.right: parent.right
                                    anchors.topMargin: -2; anchors.rightMargin: -2
                                    width: Math.max(16, _cl.implicitWidth + 6); height: 16; radius: 8
                                    color: ChiTheme.colors.primary
                                    Text {
                                        id: _cl; anchors.centerIn: parent; text: pd.windowCount.toString()
                                        color: ChiTheme.colors.onPrimary; font.pixelSize: 9; font.weight: 600
                                        font.family: ChiTheme.fontFamily
                                    }
                                }
                            }

                            Rectangle {
                                width: pd.isRunning ? 12 : 0; height: 3; radius: 1.5
                                anchors.horizontalCenter: parent.horizontalCenter
                                color: ChiTheme.colors.primary; visible: pd.isRunning
                                Behavior on width { SpringAnimation { spring: 14.0; damping: 0.75 } }
                            }
                            Item { width: 12; height: 3; visible: !pd.isRunning }
                        }

                        scale: _pm.pressed && !pd.held ? 0.9 : _pm.containsMouse ? 1.1 : 1.0
                        Behavior on scale { SpringAnimation { spring: 16.0; damping: 0.65 } }

                        MouseArea {
                            id: _pm; anchors.fill: parent
                            acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                            hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                            drag.target: pd.held ? pd : undefined
                            drag.axis: Drag.XAxis

                            onContainsMouseChanged: {
                                if (containsMouse) {
                                    var p = pd.mapToItem(root, pd.width / 2, 0)
                                    root.showTip(1000 + pd.index, pd.name, p.x)
                                } else { root.hideTip(1000 + pd.index) }
                            }

                            onPressAndHold: {
                                pd.held = true
                                pd.dragStartX = pd.x
                                pinnedContainer.dragFromIndex = pd.index
                            }

                            onReleased: {
                                if (pd.held) {
                                    pd.held = false
                                    pd.x = pd.dragStartX // snap back visually
                                    // reorder already happened in position change handler
                                    pinnedContainer.dragFromIndex = -1
                                    pinnedContainer.dragToIndex = -1
                                }
                            }

                            onPositionChanged: {
                                if (!pd.held) return
                                // Figure out which slot we're over
                                var globalPos = pd.mapToItem(pinnedContainer, pd.width / 2, 0)
                                var targetIdx = Math.floor(globalPos.x / 48)
                                targetIdx = Math.max(0, Math.min(targetIdx, pinnedApps.count - 1))
                                if (targetIdx !== pinnedContainer.dragToIndex && targetIdx !== pd.index) {
                                    pinnedContainer.dragToIndex = targetIdx
                                    pinnedApps.reorder(pd.index, targetIdx)
                                }
                            }

                            onClicked: function(mouse) {
                                if (pd.held) return
                                if (mouse.button === Qt.RightButton) {
                                    var p = pd.mapToItem(root, pd.width / 2, 0)
                                    root.openCtx(pd.appId, pd.name, pd.isRunning, true, pd.index, -1, p.x)
                                    return
                                }
                                if (mouse.button === Qt.MiddleButton) { root.closeFirst(pd.appId); return }
                                if (!pd.isRunning) pinnedApps.launch(pd.index)
                                else if (pd.windowCount > 1) wayfireIPC.activateScaleForApp(pd.appId)
                                else pinnedApps.launch(pd.index)
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: divider
            anchors.left: pinnedContainer.right; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            width: 1; height: 32; color: ChiTheme.colors.outlineVariant
            visible: pinnedApps.count > 0 && unpinnedWindows.count > 0
        }

        // ── Running Windows ──────────────────────────────
        ListView {
            id: windowList
            anchors.left: divider.visible ? divider.right : pinnedContainer.right
            anchors.leftMargin: 8
            anchors.right: rightSection.left; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: 48; orientation: ListView.Horizontal; spacing: 4; clip: false
            model: unpinnedWindows

            add: Transition {
                ParallelAnimation {
                    NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 200 }
                    NumberAnimation { property: "scale"; from: 0.6; to: 1.0; duration: 250; easing.type: Easing.OutBack }
                }
            }
            remove: Transition { NumberAnimation { property: "opacity"; to: 0; duration: 150 } }
            displaced: Transition { NumberAnimation { properties: "x"; duration: 200; easing.type: Easing.OutCubic } }

            delegate: Item {
                id: wd; width: 44; height: 48
                required property int    index
                required property string appId
                required property string title
                required property bool   activated
                required property bool   minimized
                required property string iconName

                Column {
                    anchors.centerIn: parent; spacing: 2

                    Rectangle {
                        width: 36; height: 36; radius: 10
                        color: wd.activated ? ChiTheme.colors.primaryContainer
                                            : ChiTheme.colors.surfaceContainerHighest
                        anchors.horizontalCenter: parent.horizontalCenter
                        AppIcon {
                            anchors.centerIn: parent; iconName: wd.iconName; size: 28
                            fallbackText: wd.title ? wd.title[0].toUpperCase() : "?"
                        }
                    }

                    Rectangle {
                        width: wd.activated ? 16 : 6; height: 3; radius: 1.5
                        anchors.horizontalCenter: parent.horizontalCenter
                        color: wd.activated ? ChiTheme.colors.primary : ChiTheme.colors.outlineVariant
                        Behavior on width  { SpringAnimation { spring: 14.0; damping: 0.75 } }
                        Behavior on color  { ColorAnimation { duration: 150 } }
                    }
                }

                scale: _wm.pressed ? 0.9 : _wm.containsMouse ? 1.1 : 1.0
                Behavior on scale { SpringAnimation { spring: 16.0; damping: 0.65 } }

                MouseArea {
                    id: _wm; anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                    onContainsMouseChanged: {
                        if (containsMouse) {
                            var p = wd.mapToItem(root, wd.width / 2, 0)
                            root.showTip(wd.index, wd.title || wd.appId, p.x)
                        } else { root.hideTip(wd.index) }
                    }
                    onClicked: function(mouse) {
                        if (mouse.button === Qt.RightButton) {
                            var p = wd.mapToItem(root, wd.width / 2, 0)
                            root.openCtx(wd.appId, wd.title, true,
                                         pinnedApps.isPinned(wd.appId), -1, wd.index, p.x)
                            return
                        }
                        if (mouse.button === Qt.MiddleButton) unpinnedWindows.close(wd.index)
                        else unpinnedWindows.toggleMinimize(wd.index)
                    }
                }
            }
        }

        Row {
            id: rightSection
            anchors.right: parent.right; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter; spacing: 2

            Rectangle {
                width: 40; height: 40; radius: 20
                color: _dm.pressed ? Qt.rgba(1,1,1,0.1) : _dm.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
                Icon { anchors.centerIn: parent; source: "desktop_windows"; size: 24; color: ChiTheme.colors.onSurfaceVariant }
                scale: _dm.pressed ? 0.9 : 1.0
                Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
                MouseArea { id: _dm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: wayfireIPC.showDesktop() }
            }

            Rectangle {
                width: 40; height: 40; radius: 20
                color: _nm.pressed ? Qt.rgba(1,1,1,0.1) : _nm.containsMouse ? Qt.rgba(1,1,1,0.05) : "transparent"
                Icon { anchors.centerIn: parent; source: "notifications"; size: 24; color: ChiTheme.colors.onSurfaceVariant }
                scale: _nm.pressed ? 0.9 : 1.0
                Behavior on scale { SpringAnimation { spring: 14.0; damping: 0.75 } }
                MouseArea { id: _nm; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: shell.notificationCenterOpen = !shell.notificationCenterOpen }
                Rectangle {
                    visible: notifications.count > 0
                    anchors.top: parent.top; anchors.right: parent.right
                    anchors.topMargin: 2; anchors.rightMargin: 2
                    width: Math.max(18, _bt.implicitWidth + 8); height: 18; radius: 9
                    color: ChiTheme.colors.error
                    Text { id: _bt; anchors.centerIn: parent; text: notifications.count.toString()
                        color: "#FFF"; font.pixelSize: 10; font.weight: 600; font.family: ChiTheme.fontFamily }
                }
            }
        }
    } // end bar

    // ═══════════════════════════════════════════════════
    // TOOLTIP
    // ═══════════════════════════════════════════════════
    property int _tipIdx: -1; property string _tipText: ""; property real _tipX: 0; property bool _tipVis: false

    Timer { id: _tipDelay; interval: 400; onTriggered: if (root._tipIdx >= 0) root._tipVis = true }
    Timer { id: _tipHide; interval: 100; onTriggered: if (root._tipIdx < 0) root._tipVis = false }

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
        id: tip; visible: root._tipVis; z: 100
        x: Math.max(4, Math.min(root._tipX - tipBody.width / 2, root.width - tipBody.width - 4))
        y: root.popupH - tipBody.height - 14

        Rectangle {
            id: tipBody; z: 1; width: tipLbl.implicitWidth + 16; height: 24; radius: 8
            color: ChiTheme.colors.inverseSurface
            Text { id: tipLbl; anchors.centerIn: parent; text: root._tipText
                color: ChiTheme.colors.inverseOnSurface
                font.pixelSize: ChiTheme.typography.bodySmall.size; font.weight: 500; font.family: ChiTheme.fontFamily }
        }
        Rectangle { z: 0; width: 10; height: 10; rotation: 45; color: ChiTheme.colors.inverseSurface
            x: Math.max(8, Math.min(root._tipX - tip.x - 5, tipBody.width - 18)); y: tipBody.height - 5 }
    }

    // ═══════════════════════════════════════════════════
    // CONTEXT MENU
    // ═══════════════════════════════════════════════════
    property string _cAppId: ""; property string _cName: ""
    property bool _cRunning: false; property bool _cPinned: false
    property int _cPinIdx: -1; property int _cWinIdx: -1
    property bool ctxOpen: false

    onCtxOpenChanged: shell.taskbarPopupActive = ctxOpen

    Connections {
        target: pinnedApps
        function onCountChanged() {
            if (root.ctxOpen && root._cAppId !== "")
                root._cPinned = pinnedApps.isPinned(root._cAppId)
        }
    }

    function openCtx(appId, name, running, pinned, pinIdx, winIdx, gx) {
        _cAppId = appId; _cName = name; _cRunning = running
        _cPinned = pinned; _cPinIdx = pinIdx; _cWinIdx = winIdx
        ctx.targetX = gx - 110; ctxOpen = true
    }
    function closeCtx() { ctxOpen = false }
    function closeFirst(appId) {
        for (var i = 0; i < windowTracker.count; i++) {
            if (windowTracker.data(windowTracker.index(i, 0), 258) === appId) {
                windowTracker.close(i); return
            }
        }
    }

    MouseArea {
        x: 0; y: 0; width: root.width; height: root.popupH + root.barH
        z: 150; visible: root.ctxOpen; onClicked: root.closeCtx()
    }

    Rectangle {
        id: ctx; visible: root.ctxOpen; z: 200; width: 220; radius: 16
        color: ChiTheme.colors.surfaceContainerHighest
        border.color: Qt.rgba(1,1,1,0.06); border.width: 1
        property real targetX: 0
        x: Math.max(4, Math.min(targetX, root.width - width - 4))
        y: root.popupH - height - 8
        height: ctxCol.height + 16
        scale: visible ? 1.0 : 0.92; opacity: visible ? 1.0 : 0.0
        Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
        Behavior on opacity { NumberAnimation { duration: 120 } }
        transformOrigin: Item.Bottom

        Column {
            id: ctxCol; x: 8; y: 8; width: parent.width - 16; spacing: 2

            Rectangle {
                width: parent.width; height: 40; radius: 8; visible: root._cRunning
                color: _nwH.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                Row {
                    anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; spacing: 12
                    Icon { source: "add"; size: 20; anchors.verticalCenter: parent.verticalCenter; color: ChiTheme.colors.onSurface }
                    Text { text: "New Window"; anchors.verticalCenter: parent.verticalCenter; color: ChiTheme.colors.onSurface
                        font.pixelSize: ChiTheme.typography.bodyMedium.size; font.family: ChiTheme.fontFamily }
                }
                MouseArea { id: _nwH; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: { if (root._cPinIdx >= 0) pinnedApps.launch(root._cPinIdx); root.closeCtx() } }
            }

            Rectangle {
                width: parent.width; height: 40; radius: 8
                color: _puH.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                Row {
                    anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; spacing: 12
                    Icon { source: root._cPinned ? "keep_off" : "push_pin"; size: 20
                        anchors.verticalCenter: parent.verticalCenter; color: ChiTheme.colors.onSurface }
                    Text { text: root._cPinned ? "Unpin from Taskbar" : "Pin to Taskbar"
                        anchors.verticalCenter: parent.verticalCenter; color: ChiTheme.colors.onSurface
                        font.pixelSize: ChiTheme.typography.bodyMedium.size; font.family: ChiTheme.fontFamily }
                }
                MouseArea { id: _puH; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root._cPinned) pinnedApps.unpin(root._cAppId)
                        else pinnedApps.pin(root._cAppId)
                        root.closeCtx()
                    } }
            }

            Rectangle {
                width: parent.width; height: 40; radius: 8; visible: root._cRunning
                color: _clH.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                Row {
                    anchors.verticalCenter: parent.verticalCenter; anchors.left: parent.left; anchors.leftMargin: 12; spacing: 12
                    Icon { source: "close"; size: 20; anchors.verticalCenter: parent.verticalCenter; color: ChiTheme.colors.error }
                    Text { text: "Close"; anchors.verticalCenter: parent.verticalCenter; color: ChiTheme.colors.error
                        font.pixelSize: ChiTheme.typography.bodyMedium.size; font.family: ChiTheme.fontFamily }
                }
                MouseArea { id: _clH; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (root._cWinIdx >= 0) unpinnedWindows.close(root._cWinIdx)
                        else root.closeFirst(root._cAppId)
                        root.closeCtx()
                    } }
            }
        }
    }
}
