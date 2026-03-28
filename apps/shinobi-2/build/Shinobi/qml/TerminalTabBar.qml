import QtQuick
import QtQuick.Layouts
import Chi 1.0

Rectangle {
    id: tabBar

    color: ChiTheme.colors.surfaceContainer
    height: 36

    // Thin bottom border
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: ChiTheme.colors.outlineVariant
        opacity: 0.4
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 2

        // ── Tab strip ───────────────────────────────────────
        ListView {
            id: tabList
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: ListView.Horizontal
            clip: true
            spacing: 2

            model: SessionManager.tabTitles

            delegate: Item {
                id: tabDelegate
                width: Math.min(180, Math.max(100, tabLabel.implicitWidth + 44))
                height: tabList.height

                readonly property bool isActive: index === SessionManager.activeTabIndex

                Rectangle {
                    anchors.fill: parent
                    anchors.topMargin: 4
                    anchors.bottomMargin: 2
                    radius: 8
                    color: {
                        if (tabDelegate.isActive)
                            return ChiTheme.colors.secondaryContainer
                        if (tabMouse.containsMouse)
                            return Qt.rgba(ChiTheme.colors.onSurface.r,
                                          ChiTheme.colors.onSurface.g,
                                          ChiTheme.colors.onSurface.b, 0.08)
                        return "transparent"
                    }

                    Behavior on color {
                        ColorAnimation { duration: 150 }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 4
                        spacing: 4

                        // Tab title
                        Text {
                            id: tabLabel
                            Layout.fillWidth: true
                            text: modelData || ("Tab " + (index + 1))
                            font.family: ChiTheme.typography.labelMedium.family
                            font.pixelSize: 12
                            font.weight: tabDelegate.isActive
                                         ? Font.Medium : Font.Normal
                            color: tabDelegate.isActive
                                   ? ChiTheme.colors.onSecondaryContainer
                                   : ChiTheme.colors.onSurfaceVariant
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        // Close button
                        Item {
                            width: 20
                            height: 20
                            visible: tabMouse.containsMouse || tabDelegate.isActive
                            opacity: closeMouseArea.containsMouse ? 1.0 : 0.5

                            Text {
                                anchors.centerIn: parent
                                text: "close"
                                font.family: "Material Symbols Rounded"
                                font.pixelSize: 14
                                color: tabDelegate.isActive
                                       ? ChiTheme.colors.onSecondaryContainer
                                       : ChiTheme.colors.onSurfaceVariant
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

        // ── New tab button ──────────────────────────────────
        Item {
            width: 28
            height: 28
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                anchors.fill: parent
                radius: 8
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
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: SessionManager.createTab()
            }
        }
    }
}
