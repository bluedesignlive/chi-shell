import QtQuick
import Chi 1.0
import "../"

Item {
    id: trashItem
    width: 44; height: 48

    property bool _menuOpen: false

    Column {
        anchors.centerIn: parent; spacing: 2

        Rectangle {
            id: trashBg
            width: 36; height: 36; radius: 10
            anchors.horizontalCenter: parent.horizontalCenter
            color: trashArea.containsMouse
                       ? ChiTheme.colors.surfaceContainerHighest
                       : "transparent"
            Behavior on color { ColorAnimation { duration: 150 } }

            Icon {
                anchors.centerIn: parent
                source: trashManager.iconName; size: 24
                color: trashManager.isEmpty
                           ? ChiTheme.colors.onSurfaceVariant
                           : ChiTheme.colors.onSurface
            }

            Rectangle {
                visible: !trashManager.isEmpty
                anchors { top: parent.top; right: parent.right;
                          topMargin: -2; rightMargin: -2 }
                width: Math.max(16, trashBadge.implicitWidth + 6)
                height: 16; radius: 8
                color: ChiTheme.colors.error

                Text {
                    id: trashBadge
                    anchors.centerIn: parent
                    text: trashManager.count > 99 ? "99+" : trashManager.count.toString()
                    color: "#FFF"
                    font { pixelSize: 9; weight: 600; family: ChiTheme.fontFamily }
                }
            }
        }

        Rectangle {
            anchors.horizontalCenter: parent.horizontalCenter
            width: 6; height: 3; radius: 1.5
            color: ChiTheme.colors.outlineVariant
            visible: !trashManager.isEmpty
        }
        Item { width: 1; height: 3; visible: trashManager.isEmpty }
    }

    scale: trashArea.pressed ? 0.9 : trashArea.containsMouse ? 1.05 : 1.0
    Behavior on scale { SpringAnimation { spring: 16; damping: 0.65 } }

    MouseArea {
        id: trashArea
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor

        onClicked: function(mouse) {
            trashItem._menuOpen = !trashItem._menuOpen
        }

        onContainsMouseChanged: {
            if (containsMouse) trashTip.showTip()
            else { trashTip.hideTip() }
        }
    }

    // ── Tooltip ──────────────────────────────────────────
    Item {
        id: trashTip
        property bool _vis: false
        function showTip() { _tipDelay.restart() }
        function hideTip() { _tipDelay.stop(); _vis = false }
        Timer { id: _tipDelay; interval: 400; onTriggered: trashTip._vis = true }

        Item {
            visible: trashTip._vis && !trashItem._menuOpen
            parent: trashItem.parent ? trashItem.parent.parent : trashItem
            x: trashItem.x + trashItem.width / 2 - tipBody.width / 2
            y: trashItem.parent ? trashItem.parent.y - tipBody.height - 20 : -40

            Rectangle {
                id: tipBody
                width: tipLbl.implicitWidth + 16; height: 24; radius: 8
                color: ChiTheme.colors.inverseSurface
                Text {
                    id: tipLbl; anchors.centerIn: parent
                    text: trashManager.isEmpty ? "Trash (empty)"
                              : "Trash (" + trashManager.count + " items)"
                    color: ChiTheme.colors.inverseOnSurface
                    font.pixelSize: 11; font.weight: 500
                    font.family: ChiTheme.fontFamily
                }
            }
        }
    }

    // ── Trash panel (popup) ──────────────────────────────
    Item {
        visible: trashItem._menuOpen
        parent: trashItem.parent ? trashItem.parent.parent : trashItem
        z: 200

        MouseArea {
            x: 0; y: 0
            width: parent ? parent.width : 0
            height: parent ? parent.height : 0
            onClicked: trashItem._menuOpen = false
        }

        Rectangle {
            id: trashPanel
            width: 320
            radius: 16
            color: ChiTheme.colors.surfaceContainerHighest
            border.color: Qt.rgba(1,1,1,0.06); border.width: 1
            height: Math.min(trashCol.height + 16, 420)
            clip: true

            x: Math.max(4, Math.min(
                trashItem.x + trashItem.width / 2 - width / 2,
                (parent ? parent.width : 800) - width - 4))
            y: trashItem.parent ? trashItem.parent.y - height - 12 : -height - 12

            scale: trashItem._menuOpen ? 1.0 : 0.92
            opacity: trashItem._menuOpen ? 1.0 : 0.0
            transformOrigin: Item.Bottom
            Behavior on scale   { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: 120 } }

            MouseArea { anchors.fill: parent }

            Flickable {
                anchors.fill: parent; anchors.margins: 8
                contentHeight: trashCol.height
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Column {
                    id: trashCol; width: parent.width; spacing: 0

                    // Header
                    Item {
                        width: parent.width; height: 40
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 8
                            spacing: 8
                            Icon {
                                source: trashManager.iconName; size: 20
                                color: ChiTheme.colors.onSurface
                                anchors.verticalCenter: parent.verticalCenter
                            }
                            Text {
                                text: trashManager.isEmpty ? "Trash is empty"
                                          : "Trash (" + trashManager.count + ")"
                                color: ChiTheme.colors.onSurface
                                font.pixelSize: 13; font.weight: 600
                                font.family: ChiTheme.fontFamily
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }
                    }

                    Rectangle {
                        width: parent.width; height: 1
                        color: Qt.rgba(1,1,1,0.06)
                        visible: !trashManager.isEmpty
                    }

                    // Action buttons
                    Row {
                        width: parent.width; spacing: 4
                        visible: !trashManager.isEmpty
                        leftPadding: 4; topPadding: 4; bottomPadding: 4

                        Rectangle {
                            width: openBtn.width + 16; height: 28; radius: 14
                            color: openMA.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                            Row {
                                id: openBtn; anchors.centerIn: parent; spacing: 4
                                Icon { source: "folder_open"; size: 14;
                                       color: ChiTheme.colors.primary
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { text: "Open"; color: ChiTheme.colors.primary
                                       font.pixelSize: 11; font.weight: 500
                                       font.family: ChiTheme.fontFamily
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                            MouseArea { id: openMA; anchors.fill: parent; hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: { trashManager.openTrash(); trashItem._menuOpen = false } }
                        }

                        Rectangle {
                            width: restoreBtn.width + 16; height: 28; radius: 14
                            color: restoreMA.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                            visible: trashManager.canRestore()
                            Row {
                                id: restoreBtn; anchors.centerIn: parent; spacing: 4
                                Icon { source: "undo"; size: 14;
                                       color: ChiTheme.colors.onSurface
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { text: "Undo"; color: ChiTheme.colors.onSurface
                                       font.pixelSize: 11; font.weight: 500
                                       font.family: ChiTheme.fontFamily
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                            MouseArea { id: restoreMA; anchors.fill: parent; hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: trashManager.restoreLast() }
                        }

                        Item { width: 1; height: 1 }

                        Rectangle {
                            width: emptyBtn.width + 16; height: 28; radius: 14
                            color: emptyMA.containsMouse ? ChiTheme.colors.errorContainer : "transparent"
                            Row {
                                id: emptyBtn; anchors.centerIn: parent; spacing: 4
                                Icon { source: "delete_forever"; size: 14;
                                       color: ChiTheme.colors.error
                                       anchors.verticalCenter: parent.verticalCenter }
                                Text { text: "Empty"; color: ChiTheme.colors.error
                                       font.pixelSize: 11; font.weight: 500
                                       font.family: ChiTheme.fontFamily
                                       anchors.verticalCenter: parent.verticalCenter }
                            }
                            MouseArea { id: emptyMA; anchors.fill: parent; hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: trashManager.emptyTrash() }
                        }
                    }

                    Rectangle {
                        width: parent.width; height: 1
                        color: Qt.rgba(1,1,1,0.06)
                        visible: !trashManager.isEmpty
                    }

                    // Empty state
                    Item {
                        width: parent.width; height: 80
                        visible: trashManager.isEmpty
                        Column {
                            anchors.centerIn: parent; spacing: 8
                            Icon { source: "delete_outline"; size: 32
                                   color: ChiTheme.colors.outlineVariant
                                   anchors.horizontalCenter: parent.horizontalCenter }
                            Text {
                                text: "Nothing in the trash"
                                color: ChiTheme.colors.onSurfaceVariant
                                font.pixelSize: 12; font.family: ChiTheme.fontFamily
                                anchors.horizontalCenter: parent.horizontalCenter
                            }
                        }
                    }

                    // File list
                    Repeater {
                        model: trashManager.items

                        Rectangle {
                            id: trashRow
                            width: trashCol.width; height: 44; radius: 8
                            color: rowMA.containsMouse ? Qt.rgba(1,1,1,0.06) : "transparent"

                            required property int index
                            required property string displayName
                            required property string originalPath
                            required property string sizeText
                            required property string iconName
                            required property bool isDir

                            Row {
                                anchors.fill: parent; anchors.margins: 8; spacing: 8

                                Icon {
                                    source: trashRow.iconName; size: 20
                                    color: ChiTheme.colors.onSurfaceVariant
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                Column {
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: parent.width - 80

                                    Text {
                                        text: trashRow.displayName
                                        color: ChiTheme.colors.onSurface
                                        font.pixelSize: 12; font.weight: 500
                                        font.family: ChiTheme.fontFamily
                                        elide: Text.ElideMiddle
                                        width: parent.width
                                    }
                                    Text {
                                        text: trashRow.sizeText
                                        color: ChiTheme.colors.onSurfaceVariant
                                        font.pixelSize: 10
                                        font.family: ChiTheme.fontFamily
                                    }
                                }

                                // Restore button
                                Rectangle {
                                    width: 24; height: 24; radius: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: restoreItemMA.containsMouse
                                               ? Qt.rgba(1,1,1,0.12) : "transparent"
                                    visible: rowMA.containsMouse
                                    Icon { anchors.centerIn: parent
                                           source: "undo"; size: 14
                                           color: ChiTheme.colors.primary }
                                    MouseArea {
                                        id: restoreItemMA; anchors.fill: parent
                                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                        onClicked: trashManager.restoreItem(trashRow.index)
                                    }
                                }

                                // Delete permanently button
                                Rectangle {
                                    width: 24; height: 24; radius: 12
                                    anchors.verticalCenter: parent.verticalCenter
                                    color: deleteItemMA.containsMouse
                                               ? ChiTheme.colors.errorContainer : "transparent"
                                    visible: rowMA.containsMouse
                                    Icon { anchors.centerIn: parent
                                           source: "close"; size: 14
                                           color: ChiTheme.colors.error }
                                    MouseArea {
                                        id: deleteItemMA; anchors.fill: parent
                                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                        onClicked: trashManager.deleteItem(trashRow.index)
                                    }
                                }
                            }

                            MouseArea {
                                id: rowMA; anchors.fill: parent
                                hoverEnabled: true; z: -1
                            }
                        }
                    }
                }
            }
        }
    }
}
