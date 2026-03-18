import QtQuick
import Chi 1.0
import "../"

Row {
    id: windowSection
    spacing: 4
    height: 48

    property Item rootItem: null

    signal showTooltip(int idx, string title, real gx)
    signal hideTooltip(int idx)
    signal openContextMenu(string appId, string title, bool isPinned,
                           int groupIdx, real gx)
    signal showPreview(string appId, real gx)
    signal hidePreview()

    Repeater {
        model: groupedWindows

        Item {
            id: wd
            width: 44; height: 48

            required property int    index
            required property string appId
            required property string title
            required property string iconName
            required property int    windowCount
            required property bool   isActivated
            required property bool   isMinimized

            Column {
                anchors.centerIn: parent
                spacing: 2

                Rectangle {
                    width: 36; height: 36; radius: 10
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: wd.isActivated
                               ? ChiTheme.colors.primaryContainer
                               : _wm.containsMouse
                                   ? ChiTheme.colors.surfaceContainerHigh
                                   : ChiTheme.colors.surfaceContainerHighest

                    Behavior on color { ColorAnimation { duration: 150 } }

                    AppIcon {
                        anchors.centerIn: parent
                        iconName: wd.iconName
                        size: 28
                        fallbackText: wd.title ? wd.title[0].toUpperCase() : "?"
                    }

                    // Window count badge
                    Rectangle {
                        visible: wd.windowCount > 1
                        anchors.top: parent.top; anchors.right: parent.right
                        anchors.topMargin: -2; anchors.rightMargin: -2
                        width: Math.max(16, _wc.implicitWidth + 6); height: 16; radius: 8
                        color: ChiTheme.colors.primary
                        Text {
                            id: _wc
                            anchors.centerIn: parent
                            text: wd.windowCount.toString()
                            color: ChiTheme.colors.onPrimary
                            font.pixelSize: 9; font.weight: 600
                            font.family: ChiTheme.fontFamily
                        }
                    }
                }

                // Indicator: wide+bright if activated, small dot otherwise (always running)
                Rectangle {
                    width: wd.isActivated ? 16 : 6
                    height: 3; radius: 1.5
                    anchors.horizontalCenter: parent.horizontalCenter
                    color: wd.isActivated ? ChiTheme.colors.primary
                                          : ChiTheme.colors.onSurfaceVariant
                    Behavior on width { SpringAnimation { spring: 14.0; damping: 0.75 } }
                    Behavior on color { ColorAnimation { duration: 150 } }
                }
            }

            scale: _wm.pressed ? 0.9 : _wm.containsMouse ? 1.05 : 1.0
            Behavior on scale { SpringAnimation { spring: 16.0; damping: 0.65 } }

            MouseArea {
                id: _wm
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true

                onContainsMouseChanged: {
                    if (!windowSection.rootItem) return
                    var p = wd.mapToItem(windowSection.rootItem, wd.width / 2, 0)
                    if (containsMouse) {
                        windowSection.hideTooltip(wd.index)
                        windowSection.showPreview(wd.appId, p.x)
                    } else {
                        windowSection.hideTooltip(wd.index)
                        windowSection.hidePreview()
                    }
                }

                onClicked: function(mouse) {
                    if (mouse.modifiers & Qt.ShiftModifier) { mouse.accepted = false; return }
                    if (shell.appLauncherOpen) {
                        shell.appLauncherOpen = false
                        return
                    }
                    if (mouse.button === Qt.RightButton) {
                        if (windowSection.rootItem) {
                            var p = wd.mapToItem(windowSection.rootItem, wd.width / 2, 0)
                            windowSection.openContextMenu(wd.appId, wd.title,
                                pinnedApps.isPinned(wd.appId), wd.index, p.x)
                        }
                        return
                    }
                    if (mouse.button === Qt.MiddleButton) {
                        groupedWindows.close(wd.index)
                        return
                    }
                    // Left click
                    if (wd.windowCount > 1)
                        wayfireIPC.activateScaleForApp(wd.appId)
                    else
                        groupedWindows.toggleMinimize(wd.index)
                }
            }
        }
    }
}
