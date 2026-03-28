import QtQuick
import QtQuick.Layouts
import Chi 1.0

// The tab bar sits directly below the Chi toolbar (which has the
// window controls). It uses surfaceContainerLow to be visually
// distinct from the terminal area below it.

Rectangle {
    id: tabBar
    color: ChiTheme.colors.surfaceContainerLow

    // Thin separator at bottom
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: ChiTheme.colors.outlineVariant
        opacity: 0.15
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 0

        // ── Tab strip ───────────────────────────────────────
        ListView {
            id: tabList
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            clip: true
            spacing: 1

            model: SessionManager.tabTitles

            delegate: Item {
                id: tabDelegate
                width: Math.min(200, Math.max(120, tabLabel.implicitWidth + 48))
                height: tabList.height

                readonly property bool isActive: index === SessionManager.activeTabIndex

                Rectangle {
                    anchors.fill: parent
                    anchors.topMargin: 5
                    anchors.bottomMargin: 0

                    radius: 8
                    // Only round top — fake it with a bottom rect
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: parent.radius
                        color: parent.color
                    }

                    color: {
                        if (tabDelegate.isActive)
                            return ChiTheme.colors.surfaceContainerLowest
                        if (tabMouse.containsMouse)
                            return Qt.rgba(ChiTheme.colors.onSurface.r,
                                          ChiTheme.colors.onSurface.g,
                                          ChiTheme.colors.onSurface.b, 0.05)
                        return "transparent"
                    }

                    Behavior on color {
                        ColorAnimation { duration: ChiTheme.motion.durationFast }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 6
                        spacing: 4

                        Text {
                            id: tabLabel
                            Layout.fillWidth: true
                            text: modelData || ("Tab " + (index + 1))
                            font.family: ChiTheme.typography.labelSmall.family
                            font.pixelSize: ChiTheme.typography.labelSmall.size
                            font.weight: tabDelegate.isActive ? Font.Medium : Font.Normal
                            color: tabDelegate.isActive
                                   ? ChiTheme.colors.onSurface
                                   : ChiTheme.colors.onSurfaceVariant
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        Rectangle {
                            width: 20
                            height: 20
                            radius: 10
                            visible: tabMouse.containsMouse || tabDelegate.isActive
                            color: closeMouseArea.containsMouse
                                   ? Qt.rgba(ChiTheme.colors.onSurface.r,
                                             ChiTheme.colors.onSurface.g,
                                             ChiTheme.colors.onSurface.b, 0.1)
                                   : "transparent"

                            Text {
                                anchors.centerIn: parent
                                text: "close"
                                font.family: "Material Symbols Rounded"
                                font.pixelSize: 14
                                color: ChiTheme.colors.onSurfaceVariant
                                opacity: closeMouseArea.containsMouse ? 1.0 : 0.5
                            }

                            MouseArea {
                                id: closeMouseArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: SessionManager.closeTab(index)
                            }
                        }
                    }

                    MouseArea {
                        id: tabMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
                        z: -1
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.MiddleButton)
                                SessionManager.closeTab(index)
                            else
                                SessionManager.switchTab(index)
                        }
                    }
                }
            }
        }

        // ── New tab ─────────────────────────────────────────
        Item {
            width: 28; height: 28
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                anchors.fill: parent; radius: 8
                color: newTabMouse.containsMouse
                       ? Qt.rgba(ChiTheme.colors.onSurface.r,
                                 ChiTheme.colors.onSurface.g,
                                 ChiTheme.colors.onSurface.b, 0.08)
                       : "transparent"
            }
            Text {
                anchors.centerIn: parent
                text: "add"
                font.family: "Material Symbols Rounded"
                font.pixelSize: 18
                color: ChiTheme.colors.onSurfaceVariant
            }
            MouseArea {
                id: newTabMouse
                anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: SessionManager.createTab()
            }
        }

        // ── Theme toggle ────────────────────────────────────
        Item {
            width: 28; height: 28
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                anchors.fill: parent; radius: 8
                color: themeMouse.containsMouse
                       ? Qt.rgba(ChiTheme.colors.onSurface.r,
                                 ChiTheme.colors.onSurface.g,
                                 ChiTheme.colors.onSurface.b, 0.08)
                       : "transparent"
            }
            Text {
                anchors.centerIn: parent
                text: ChiTheme.isDarkMode ? "light_mode" : "dark_mode"
                font.family: "Material Symbols Rounded"
                font.pixelSize: 18
                color: ChiTheme.colors.onSurfaceVariant
            }
            MouseArea {
                id: themeMouse
                anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: ChiTheme.toggleDarkMode()
            }
        }
    }
}
