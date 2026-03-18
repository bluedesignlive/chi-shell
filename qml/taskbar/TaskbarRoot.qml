import QtQuick
import Chi 1.0
import "../"

Item {
    id: root
    anchors.fill: parent

    readonly property int popupH: typeof TASKBAR_POPUP_H !== "undefined" ? TASKBAR_POPUP_H : 250
    readonly property int barH:   typeof TASKBAR_BAR_H   !== "undefined" ? TASKBAR_BAR_H   : 52

    property bool autoHideEnabled: false
    property bool popupsOpen: appCtx.visible || taskbarCtx.visible
                              || windowPreview.visible || shell.appLauncherOpen
    property bool barRevealed: !autoHideEnabled || _hoverSensor.containsMouse
                               || popupsOpen || revealLock
    property bool revealLock: false

    onPopupsOpenChanged: updateInputRegion()
    Component.onCompleted: updateInputRegion()
    onWidthChanged: updateInputRegion()

    function updateInputRegion() {
        if (typeof taskbarSurface === "undefined") return
        if (popupsOpen)
            taskbarSurface.setFullInputRegion()
        else
            taskbarSurface.setInputRegion(Qt.rect(0, popupH, root.width, barH))
    }

    Timer {
        id: _hideDelay; interval: 800
        onTriggered: root.revealLock = false
    }

    MouseArea {
        id: _hoverSensor
        x: 0; y: root.popupH + root.barH - 4
        width: root.width; height: 4
        hoverEnabled: true; visible: root.autoHideEnabled
        propagateComposedEvents: true
        onContainsMouseChanged: {
            if (containsMouse) { root.revealLock = true; _hideDelay.stop() }
        }
    }

    Rectangle {
        id: bar
        x: 0
        y: root.barRevealed ? root.popupH : root.popupH + root.barH
        width: root.width; height: root.barH
        color: ChiTheme.colors.surfaceContainer
        opacity: 0.95

        Behavior on y { NumberAnimation { duration: 250; easing.type: Easing.OutCubic } }

        MouseArea {
            anchors.fill: parent; z: -1
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            hoverEnabled: true

            onContainsMouseChanged: {
                if (root.autoHideEnabled && containsMouse) {
                    root.revealLock = true; _hideDelay.stop()
                } else if (root.autoHideEnabled && !containsMouse) {
                    _hideDelay.restart()
                }
            }

            onClicked: function(mouse) {
                if (shell.appLauncherOpen) { shell.appLauncherOpen = false; return }
                if (appCtx.visible) { appCtx.close(); return }
                if (taskbarCtx.visible) { taskbarCtx.close(); return }
                taskbarCtx.open(mouse.x)
            }
        }

        LeftButtons {
            id: leftButtons
            anchors.left: parent.left; anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }

        // ── Unified scrollable app area ──────────────────────────
        Flickable {
            id: appFlickable
            anchors.left: leftButtons.right; anchors.leftMargin: 8
            anchors.right: rightButtons.left; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            height: 48
            contentWidth: appContentRow.width
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.HorizontalFlick

            Row {
                id: appContentRow
                spacing: 4

                // ── Pinned apps ──────────────────────────────────
                PinnedAppsSection {
                    id: pinnedSection
                    rootItem: root
                    onShowTooltip: function(idx, name, gx) { tooltip.showTip(idx, name, gx) }
                    onHideTooltip: function(idx) { tooltip.hideTip(idx) }
                    onOpenContextMenu: function(appId, name, isRunning, isPinned, pinIdx, gx) {
                        appCtx.openForPinned(appId, name, isRunning, isPinned, pinIdx, gx)
                    }
                    onShowPreview: function(appId, gx) { windowPreview.scheduleShow(appId, gx) }
                    onHidePreview: windowPreview.scheduleHide()
                }

                // ── Divider ──────────────────────────────────────
                Item {
                    width: dividerRect.visible ? 17 : 0
                    height: 48
                    Rectangle {
                        id: dividerRect
                        anchors.centerIn: parent
                        width: 1; height: 32
                        color: ChiTheme.colors.outlineVariant
                        visible: pinnedApps.count > 0 && groupedWindows.count > 0
                    }
                }

                // ── Unpinned running apps (grouped) ──────────────
                RunningAppsSection {
                    id: windowSection
                    rootItem: root
                    onShowTooltip: function(idx, title, gx) { tooltip.showTip(idx, title, gx) }
                    onHideTooltip: function(idx) { tooltip.hideTip(idx) }
                    onOpenContextMenu: function(appId, title, isPinned, groupIdx, gx) {
                        appCtx.openForWindow(appId, title, isPinned, groupIdx, gx)
                    }
                    onShowPreview: function(appId, gx) { windowPreview.scheduleShow(appId, gx) }
                    onHidePreview: windowPreview.scheduleHide()
                }
            }
        }

        RightButtons {
            id: rightButtons
            anchors.right: parent.right; anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    // ═══════════════════════════════════════════════════
    // OVERLAYS
    // ═══════════════════════════════════════════════════

    TaskbarTooltip {
        id: tooltip
        popupH: root.popupH
        totalWidth: root.width
    }

    WindowPreview {
        id: windowPreview
        popupH: root.popupH
        totalWidth: root.width
    }

    AppContextMenu {
        id: appCtx
        z: 200
        popupH: root.popupH
        totalWidth: root.width
        barH: root.barH
    }

    TaskbarContextMenu {
        id: taskbarCtx
        z: 200
        popupH: root.popupH
        totalWidth: root.width
        autoHideEnabled: root.autoHideEnabled
        onToggleAutoHide: root.autoHideEnabled = !root.autoHideEnabled
        onShowDesktop: wayfireIPC.showDesktop()
        onShowOverview: wayfireIPC.toggleScale()
    }

    Connections {
        target: appCtx
        function onVisibleChanged() {
            shell.taskbarPopupActive = appCtx.visible
            root.updateInputRegion()
        }
    }
    Connections {
        target: taskbarCtx
        function onVisibleChanged() { root.updateInputRegion() }
    }
    Connections {
        target: windowPreview
        function onVisibleChanged() { root.updateInputRegion() }
    }
}
