import QtQuick
import QtQuick.Layouts
import Chi 1.0
import Shinobi 1.0

ChiApplicationWindow {
    id: root

    width: 960
    height: 640
    title: "Shinobi"
    visible: true

    // ── Terminal-specific window config ──────────────────────
    showMenu: false
    showTitle: true
    centerTitle: true
    toolbarHeight: 38
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
    Shortcut {
        sequence: "Ctrl+Tab"
        onActivated: SessionManager.nextTab()
    }
    Shortcut {
        sequence: "Ctrl+Shift+Tab"
        onActivated: SessionManager.previousTab()
    }

    // Alt+1 through Alt+9 for direct tab switch
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

    // ── Content ─────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
        TerminalTabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: 36
        }

        // Terminal pane area
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TerminalPane {
                id: terminalPane
                anchors.fill: parent
                session: SessionManager.activeSession
            }

            // Empty state — no tabs open
            Rectangle {
                anchors.fill: parent
                visible: SessionManager.tabCount === 0
                color: ChiTheme.colors.surface

                Column {
                    anchors.centerIn: parent
                    spacing: 16

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "忍"
                        font.pixelSize: 64
                        color: ChiTheme.colors.primary
                        opacity: 0.6
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Shinobi Terminal"
                        font.family: ChiTheme.typography.headlineSmall.family
                        font.pixelSize: ChiTheme.typography.headlineSmall.size
                        color: ChiTheme.colors.onSurface
                        opacity: 0.8
                    }

                    Text {
                        anchors.horizontalCenter: parent.horizontalCenter
                        text: "Ctrl+Shift+T to open a new tab"
                        font.family: ChiTheme.typography.bodyMedium.family
                        font.pixelSize: ChiTheme.typography.bodyMedium.size
                        color: ChiTheme.colors.onSurfaceVariant
                    }
                }
            }
        }

        // Status bar
        TerminalStatusBar {
            id: statusBar
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            session: SessionManager.activeSession
        }
    }

    // ── Auto-create first tab on startup ────────────────────
    Component.onCompleted: {
        SessionManager.createTab()
    }
}
