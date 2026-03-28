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

    // ── Terminal window config ───────────────────────────────
    showMenu: false
    showTitle: true
    centerTitle: true
    toolbarHeight: 0            // Kill the default toolbar — we use tabBar
    controlsStyle: "macOS"

    toolbarActions: [
        QtObject {
            property string icon: ChiTheme.isDarkMode ? "light_mode" : "dark_mode"
            property bool checked: false
            property bool enabled: true
            function triggered() { ChiTheme.toggleDarkMode() }
        }
    ]

    // ── Keyboard shortcuts ──────────────────────────────────
    Shortcut {
        sequence: "Ctrl+Shift+T"
        onActivated: SessionManager.createTab()
    }
    Shortcut {
        sequence: "Ctrl+Shift+W"
        onActivated: {
            if (SessionManager.tabCount > 0)
                SessionManager.closeTab(SessionManager.activeTabIndex)
        }
    }
    Shortcut { sequence: "Ctrl+Tab";       onActivated: SessionManager.nextTab() }
    Shortcut { sequence: "Ctrl+Shift+Tab"; onActivated: SessionManager.previousTab() }

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

    // ── Full-bleed content ──────────────────────────────────
    // The terminal fills the entire content area with no margins.
    // Tab bar at top, status bar at bottom, terminal in between —
    // all flush to the window edges.

    Item {
        anchors.fill: parent

        // ── Tab bar — flush to top edge ─────────────────────
        TerminalTabBar {
            id: tabBar
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 38
        }

        // ── Terminal — fills the entire middle ──────────────
        TerminalPane {
            id: terminalPane
            anchors.top: tabBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: statusBar.top
            session: SessionManager.activeSession
        }

        // ── Empty state (overlays terminal area) ────────────
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

        // ── Status bar — flush to bottom edge ───────────────
        TerminalStatusBar {
            id: statusBar
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 24
            session: SessionManager.activeSession
        }
    }

    Component.onCompleted: {
        SessionManager.createTab()
    }
}
