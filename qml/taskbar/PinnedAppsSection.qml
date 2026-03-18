import QtQuick
import Chi 1.0
import "../"

Row {
    id: pinned
    spacing: 4
    height: 48

    property Item rootItem: null

    signal showTooltip(int idx, string name, real gx)
    signal hideTooltip(int idx)
    signal openContextMenu(string appId, string name, bool isRunning,
                           bool isPinned, int pinIdx, real gx)
    signal showPreview(string appId, real gx)
    signal hidePreview()

    Repeater {
        model: pinnedApps

        Item {
            id: pd
            width: 44; height: 48

            required property int    index
            required property string appId
            required property string name
            required property string iconName
            required property bool   isRunning
            required property bool   isActivated
            required property int    windowCount

            property bool held: false
            property real dragStartX: 0

            z: held ? 10 : 1
            opacity: held ? 0.8 : 1.0
            Behavior on opacity { NumberAnimation { duration: 150 } }

            Column {
                anchors.centerIn: parent
                spacing: 2

                Rectangle {
                    width: 36; height: 36; radius: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: pd.isActivated
                               ? ChiTheme.colors.primaryContainer
                               : _pm.containsMouse
                                   ? ChiTheme.colors.surfaceContainerHigh
                                   : ChiTheme.colors.surfaceContainerHighest

                    Behavior on color { ColorAnimation { duration: 150 } }

                    AppIcon {
                        anchors.centerIn: parent
                        iconName: pd.iconName
                        size: 28
                        fallbackText: pd.name ? pd.name[0].toUpperCase() : "?"
                    }

                    // Window count badge
                    Rectangle {
                        visible: pd.windowCount > 1
                        anchors.top: parent.top; anchors.right: parent.right
                        anchors.topMargin: -2; anchors.rightMargin: -2
                        width: Math.max(16, _cl.implicitWidth + 6); height: 16; radius: 8
                        color: ChiTheme.colors.primary
                        Text {
                            id: _cl
                            anchors.centerIn: parent
                            text: pd.windowCount.toString()
                            color: ChiTheme.colors.onPrimary
                            font.pixelSize: 9; font.weight: 600
                            font.family: ChiTheme.fontFamily
                        }
                    }
                }

                // Indicator: wide+bright if activated, small dot if just running, nothing if not running
                Rectangle {
                    width: pd.isActivated ? 16 : pd.isRunning ? 6 : 0
                    height: 3; radius: 1.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: pd.isActivated ? ChiTheme.colors.primary
                                          : ChiTheme.colors.onSurfaceVariant
                    visible: pd.isRunning
                    Behavior on width { SpringAnimation { spring: 14.0; damping: 0.75 } }
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
                Item { width: 1; height: 3; visible: !pd.isRunning }
            }

            scale: _pm.pressed && !pd.held ? 0.9 : _pm.containsMouse ? 1.05 : 1.0
            Behavior on scale { SpringAnimation { spring: 16.0; damping: 0.65 } }

            MouseArea {
                id: _pm
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                drag.target: pd.held ? pd : undefined
                drag.axis: Drag.XAxis

                onContainsMouseChanged: {
                    if (!pinned.rootItem) return
                    var p = pd.mapToItem(pinned.rootItem, pd.width / 2, 0)
                    if (containsMouse) {
                        if (pd.isRunning) {
                            pinned.hideTooltip(1000 + pd.index)
                            pinned.showPreview(pd.appId, p.x)
                        } else {
                            pinned.hidePreview()
                            pinned.showTooltip(1000 + pd.index, pd.name, p.x)
                        }
                    } else {
                        pinned.hideTooltip(1000 + pd.index)
                        pinned.hidePreview()
                    }
                }

                onPressAndHold: {
                    pd.held = true
                    pd.dragStartX = pd.x
                }

                onReleased: {
                    if (pd.held) {
                        pd.held = false
                        pd.x = pd.dragStartX
                    }
                }

                onPositionChanged: {
                    if (!pd.held) return
                    var globalPos = pd.mapToItem(pinned, pd.width / 2, 0)
                    var targetIdx = Math.floor(globalPos.x / 48)
                    targetIdx = Math.max(0, Math.min(targetIdx, pinnedApps.count - 1))
                    if (targetIdx !== pd.index) {
                        pinnedApps.reorder(pd.index, targetIdx)
                    }
                }

                onClicked: function(mouse) {
                    if (pd.held) return
                    if (mouse.modifiers & Qt.ShiftModifier) { mouse.accepted = false; return }
                    if (shell.appLauncherOpen) {
                        shell.appLauncherOpen = false
                        return
                    }
                    if (mouse.button === Qt.RightButton) {
                        if (pinned.rootItem) {
                            var p = pd.mapToItem(pinned.rootItem, pd.width / 2, 0)
                            pinned.openContextMenu(pd.appId, pd.name, pd.isRunning,
                                                   true, pd.index, p.x)
                        }
                        return
                    }
                    if (mouse.button === Qt.MiddleButton) {
                        if (pd.isRunning) {
                            var wi = windowTracker.firstIndexForApp(pd.appId)
                            if (wi >= 0) windowTracker.close(wi)
                        } else {
                            pinnedApps.launchNew(pd.appId)
                        }
                        return
                    }
                    // Left click
                    if (!pd.isRunning)
                        pinnedApps.launch(pd.index)
                    else if (pd.windowCount > 1)
                        wayfireIPC.activateScaleForApp(pd.appId)
                    else
                        pinnedApps.launch(pd.index)
                }
            }
        }
    }
}
