import QtQuick
import Chi 1.0
import "../"

Item {
    id: delegate
    width: 44
    height: 48

    // Model roles (injected by ListView delegate)
    required property int    index
    required property string appId
    required property string name
    required property string iconName
    required property bool   isPinned
    required property bool   isRunning
    required property bool   isActivated
    required property int    windowCount
    required property string section

    // Signals for parent
    signal showTooltip(int idx, string text, real gx)
    signal hideTooltip(int idx)
    signal openContextMenu(string appId, string name, bool isPinned,
                           bool isRunning, int idx, real gx)
    signal showPreview(string appId, real gx)
    signal hidePreview()

    // Drag state
    property bool held: false

    z: held ? 10 : 1
    opacity: held ? 0.7 : 1.0
    Behavior on opacity { NumberAnimation { duration: 120 } }

    // ── Visual content ────────────────────────────────────────────

    Column {
        anchors.centerIn: parent
        spacing: 2

        // Icon container
        Rectangle {
            id: iconBg
            width: 36; height: 36; radius: 10
            anchors.horizontalCenter: parent.horizontalCenter
            color: delegate.isActivated
                       ? ChiTheme.colors.primaryContainer
                       : delegate.isRunning
                           ? ChiTheme.colors.surfaceContainerHigh
                           : hoverArea.containsMouse
                               ? ChiTheme.colors.surfaceContainerHighest
                               : ChiTheme.colors.surfaceContainerHighest

            Behavior on color { ColorAnimation { duration: 150 } }

            AppIcon {
                anchors.centerIn: parent
                iconName: delegate.iconName
                size: 28
                fallbackText: delegate.name ? delegate.name[0].toUpperCase() : "?"
            }

            // Window count badge (shown for 2+ windows)
            Rectangle {
                visible: delegate.windowCount > 1
                anchors { top: parent.top; right: parent.right;
                          topMargin: -2; rightMargin: -2 }
                width: Math.max(16, badgeText.implicitWidth + 6)
                height: 16; radius: 8
                color: ChiTheme.colors.primary

                Text {
                    id: badgeText
                    anchors.centerIn: parent
                    text: delegate.windowCount.toString()
                    color: ChiTheme.colors.onPrimary
                    font { pixelSize: 9; weight: 600; family: ChiTheme.fontFamily }
                }
            }
        }

        // State indicator dot
        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: delegate.isActivated ? 16
                 : delegate.isRunning   ? 6
                 : 0
            height: 3; radius: 1.5
            color: delegate.isActivated
                       ? ChiTheme.colors.primary
                       : ChiTheme.colors.onSurfaceVariant
            visible: delegate.isRunning

            Behavior on width { SpringAnimation { spring: 14; damping: 0.75 } }
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        // Spacer to keep layout stable when no indicator
        Item {
            width: 1; height: 3
            visible: !delegate.isRunning
        }
    }

    // ── Interaction scaling ───────────────────────────────────────

    scale: hoverArea.pressed && !delegate.held ? 0.9
         : hoverArea.containsMouse             ? 1.05
         : 1.0
    Behavior on scale { SpringAnimation { spring: 16; damping: 0.65 } }

    // ── Mouse handling ────────────────────────────────────────────

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        property real dragStartX: 0
        property bool dragActive: false

        onContainsMouseChanged: {
            var root = delegate.ListView.view ? delegate.ListView.view.parent : null
            if (!root) return
            var p = delegate.mapToItem(root, delegate.width / 2, 0)
            if (containsMouse) {
                if (delegate.isRunning) {
                    delegate.hideTooltip(delegate.index)
                    delegate.showPreview(delegate.appId, p.x)
                } else {
                    delegate.hidePreview()
                    delegate.showTooltip(delegate.index, delegate.name, p.x)
                }
            } else {
                delegate.hideTooltip(delegate.index)
                delegate.hidePreview()
            }
        }

        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                dragStartX = mouse.x
            }
        }

        onPressAndHold: {
            if (delegate.isPinned) {
                delegate.held = true
            }
        }

        onReleased: function(mouse) {
            if (delegate.held) {
                delegate.held = false
            }
        }

        onPositionChanged: function(mouse) {
            if (!delegate.held || !delegate.isPinned) return
            // Calculate target index from global position
            var lv = delegate.ListView.view
            if (!lv) return
            var globalPos = delegate.mapToItem(lv.contentItem, delegate.width / 2, 0)
            var targetIdx = Math.floor((globalPos.x + lv.contentX) / 48)
            targetIdx = Math.max(0, Math.min(targetIdx, taskbarModel.count - 1))
            if (targetIdx !== delegate.index && taskbarModel.isPinnedAt(targetIdx)) {
                taskbarModel.move(delegate.index, targetIdx)
            }
        }

        onClicked: function(mouse) {
            if (delegate.held) return

            // Dismiss launcher if open
            if (shell.appLauncherOpen) {
                shell.appLauncherOpen = false
                return
            }

            if (mouse.button === Qt.RightButton) {
                var root = delegate.ListView.view ? delegate.ListView.view.parent : null
                if (root) {
                    var p = delegate.mapToItem(root, delegate.width / 2, 0)
                    delegate.openContextMenu(delegate.appId, delegate.name,
                                             delegate.isPinned, delegate.isRunning,
                                             delegate.index, p.x)
                }
                return
            }

            if (mouse.button === Qt.MiddleButton) {
                if (delegate.isRunning)
                    taskbarModel.closeWindow(delegate.index)
                else
                    taskbarModel.launchNew(delegate.index)
                return
            }

            // Left click
            if (!delegate.isRunning) {
                taskbarModel.launch(delegate.index)
            } else {
                taskbarModel.toggleMinimize(delegate.index)
            }
        }
    }
}
