import QtQuick
import QtQuick.Layouts
import Chi 1.0
import Shinobi 1.0

ChiApplicationWindow {
    id: root

    width: 960
    height: 640
    title: {
        if (SessionManager.tabCount === 0) return "Shinobi"
        var t = SessionManager.tabTitles[SessionManager.activeTabIndex] || "Terminal"
        return t + " — Shinobi"
    }
    visible: true

    showMenu: false
    showTitle: false
    toolbarHeight: 38
    controlsStyle: "macOS"

    // ── Tab shortcuts ───────────────────────────────────────
    Shortcut { sequence: "Ctrl+Shift+T"; onActivated: SessionManager.createTab() }
    Shortcut {
        sequence: "Ctrl+Shift+W"
        onActivated: {
            if (SessionManager.tabCount > 0)
                SessionManager.closeTab(SessionManager.activeTabIndex)
        }
    }
    Shortcut { sequence: "Ctrl+Tab";       onActivated: SessionManager.nextTab() }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: SessionManager.previousTab() }

    // Alt+1-9 direct tab switch
    Repeater {
        model: 9
        Item {
            Shortcut {
                sequence: "Alt+" + (index + 1)
                onActivated: {
                    if (index < SessionManager.tabCount)
                        SessionManager.switchTab(index)
                }
            }
        }
    }

    // Fullscreen
    Shortcut {
        sequence: "F11"
        onActivated: {
            if (root.visibility === Window.FullScreen)
                root.showNormal()
            else
                root.showFullScreen()
        }
    }

    // ── Content ─────────────────────────────────────────────

    TerminalTabBar {
        id: tabBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 36
    }

    TerminalPane {
        id: terminalPane
        anchors.top: tabBar.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: statusBar.top
        session: SessionManager.activeSession
    }

    Rectangle {
        anchors.fill: terminalPane
        visible: SessionManager.tabCount === 0
        color: ChiTheme.colors.surfaceContainerLowest

        Column {
            anchors.centerIn: parent
            spacing: 16

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "忍"
                font.pixelSize: 72
                color: ChiTheme.colors.primary
                opacity: 0.4
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Shinobi Terminal"
                font.family: ChiTheme.typography.headlineSmall.family
                font.pixelSize: ChiTheme.typography.headlineSmall.size
                font.weight: Font.Medium
                color: ChiTheme.colors.onSurface
                opacity: 0.7
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Ctrl+Shift+T to open a new tab"
                font.family: ChiTheme.typography.bodyMedium.family
                font.pixelSize: ChiTheme.typography.bodyMedium.size
                color: ChiTheme.colors.onSurfaceVariant
                opacity: 0.5
            }
        }
    }

    TerminalStatusBar {
        id: statusBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 24
        session: SessionManager.activeSession
    }

    Component.onCompleted: SessionManager.createTab()
}
