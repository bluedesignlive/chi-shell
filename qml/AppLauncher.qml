import QtQuick
import Chi 1.0

Item {
    id: launcher
    anchors.fill: parent

    property real _cardReveal: 0
    property real _wave: 0

    NumberAnimation {
        id: _cardAnim; target: launcher; property: "_cardReveal"
        from: 0; to: 1; duration: 300; easing.type: Easing.OutQuart
    }
    NumberAnimation {
        id: _waveAnim; target: launcher; property: "_wave"
        from: 0; to: 1; duration: 500
    }

    function _pop(t) {
        if (t <= 0) return 0; if (t >= 1) return 1
        var s = 1.6; var u = t - 1
        return 1 + (s + 1) * u * u * u + s * u * u
    }

    Rectangle {
        anchors.fill: parent; color: ChiTheme.colors.scrim
        opacity: launcher._cardReveal * 0.92
    }

    Keys.onEscapePressed: {
        if (ctxMenu.visible) ctxMenu.close()
        else shell.appLauncherOpen = false
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (ctxMenu.visible) ctxMenu.close()
            else shell.appLauncherOpen = false
        }
    }

    Rectangle {
        id: content
        width: Math.min(parent.width - 64, 720)
        height: parent.height - 120
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2 + (1 - launcher._cardReveal) * 30
        opacity: launcher._cardReveal
        color: ChiTheme.colors.surfaceContainer
        radius: 28

        MouseArea {
            anchors.fill: parent; acceptedButtons: Qt.AllButtons
            onClicked: if (ctxMenu.visible) ctxMenu.close()
        }

        property int selectedIndex: 0

        Column {
            anchors.fill: parent; anchors.margins: 24; spacing: 16

            Rectangle {
                id: searchBar
                width: parent.width; height: 56; radius: 28
                color: ChiTheme.colors.surfaceContainerHigh

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 16; anchors.rightMargin: 16; spacing: 12

                    Icon {
                        source: "search"; size: 24; color: ChiTheme.colors.onSurfaceVariant
                        anchors.verticalCenter: parent.verticalCenter
                    }

                    TextInput {
                        id: searchInput
                        anchors.verticalCenter: parent.verticalCenter
                        width: parent.width - 52
                        color: ChiTheme.colors.onSurface
                        font.pixelSize: ChiTheme.typography.bodyLarge.size
                        font.family: ChiTheme.fontFamily
                        clip: true; focus: true

                        onTextChanged: {
                            appFilter.searchText = text
                            content.selectedIndex = 0
                            appGrid.positionViewAtBeginning()
                        }

                        Keys.onPressed: function(event) {
                            var cols = Math.max(1, Math.floor(appGrid.width / appGrid.cellWidth))
                            var cnt = appGrid.count
                            if (cnt === 0) return
                            var si = content.selectedIndex

                            switch (event.key) {
                            case Qt.Key_Right: si = Math.min(si + 1, cnt - 1); break
                            case Qt.Key_Left:  si = Math.max(si - 1, 0); break
                            case Qt.Key_Down:  si = Math.min(si + cols, cnt - 1); break
                            case Qt.Key_Up:    si = Math.max(si - cols, 0); break
                            case Qt.Key_Tab:   si = (si + 1) % cnt; break
                            case Qt.Key_Return: case Qt.Key_Enter:
                                if (cnt > 0) { appFilter.launch(si); shell.appLauncherOpen = false }
                                event.accepted = true; return
                            default: return
                            }
                            event.accepted = true
                            content.selectedIndex = si
                            appGrid.positionViewAtIndex(si, GridView.Contain)
                        }

                        Text {
                            anchors.fill: parent; text: "Search apps..."
                            color: ChiTheme.colors.onSurfaceVariant; font: searchInput.font
                            visible: searchInput.text === ""; opacity: 0.6
                        }
                    }
                }
            }

            GridView {
                id: appGrid
                width: parent.width; height: parent.height - searchBar.height - 16
                cellWidth: width / 6; cellHeight: 100
                clip: true; cacheBuffer: 300; model: appFilter
                currentIndex: content.selectedIndex; highlightFollowsCurrentItem: false

                delegate: Item {
                    id: ad
                    width: appGrid.cellWidth; height: appGrid.cellHeight
                    property bool isSel: index === content.selectedIndex

                    property real _prog: {
                        var d = Math.min(index, 30) * 0.022
                        return Math.max(0, Math.min(1, (launcher._wave - d) / 0.35))
                    }
                    property real _eased: launcher._pop(_prog)
                    scale: (0.4 + 0.6 * _eased) * (appMA.pressed ? 0.92 : 1.0)
                    opacity: Math.min(1, _prog * 2.5)

                    Column {
                        anchors.centerIn: parent; spacing: 6

                        Rectangle {
                            width: 64; height: 64; radius: 16
                            anchors.horizontalCenter: parent.horizontalCenter
                            color: ad.isSel ? ChiTheme.colors.primaryContainer
                                            : ChiTheme.colors.surfaceContainerHighest
                            border.width: ad.isSel ? 2 : 0
                            border.color: ad.isSel ? ChiTheme.colors.primary : "transparent"
                            Behavior on color { ColorAnimation { duration: 80 } }

                            AppIcon {
                                anchors.centerIn: parent; iconName: model.iconName; size: 48
                                fallbackText: model.name ? model.name[0].toUpperCase() : "?"
                            }
                        }

                        Text {
                            text: model.name; color: ChiTheme.colors.onSurface
                            font.pixelSize: ChiTheme.typography.labelMedium.size
                            font.weight: ChiTheme.typography.labelMedium.weight
                            font.family: ChiTheme.fontFamily
                            width: appGrid.cellWidth - 8
                            horizontalAlignment: Text.AlignHCenter
                            elide: Text.ElideRight; maximumLineCount: 1
                        }
                    }

                    MouseArea {
                        id: appMA; anchors.fill: parent
                        acceptedButtons: Qt.LeftButton | Qt.RightButton
                        cursorShape: Qt.PointingHandCursor; hoverEnabled: true
                        onContainsMouseChanged: if (containsMouse) content.selectedIndex = index
                        onClicked: function(mouse) {
                            if (mouse.button === Qt.RightButton) {
                                var pos = ad.mapToItem(content, ad.width / 2, ad.height)
                                ctxMenu.showAt(pos.x, pos.y, index,
                                    model.name || "", model.iconName || "", model.entryId || "")
                            } else {
                                appFilter.launch(index); shell.appLauncherOpen = false
                            }
                        }
                    }
                }
            }
        }

        // ═════════════════════════════════════════════════
        // CONTEXT MENU — pin state updates reactively
        // ═════════════════════════════════════════════════
        Item {
            id: ctxMenu; visible: false; z: 200; anchors.fill: parent

            property int    targetIdx: -1
            property string targetName: ""
            property string targetIcon: ""
            property string targetAppId: ""
            property real   _cx: 0
            property real   _cy: 0
            property bool   _above: false

            // Force re-eval when pins change
            property int _pinVer: 0
            Connections {
                target: pinnedApps
                function onCountChanged() { ctxMenu._pinVer++ }
            }

            readonly property bool _isPinned: {
                void(ctxMenu._pinVer)
                return targetAppId !== "" && pinnedApps.isPinned(targetAppId)
            }

            function showAt(cx, cy, idx, name, icon, appId) {
                targetIdx = idx; targetName = name; targetIcon = icon; targetAppId = appId
                _cx = cx
                var cardH = 2 * 48 + 16  // always 2 items
                _above = (cy + cardH + 16) > content.height
                _cy = _above ? (cy - cardH - 12) : (cy + 4)
                visible = true
            }
            function close() { visible = false }

            MouseArea { anchors.fill: parent; onClicked: ctxMenu.close() }

            // nose
            Rectangle {
                width: 12; height: 12; rotation: 45; z: 1
                color: ChiTheme.colors.surfaceContainerHighest
                x: Math.max(ctxCard.x + 16, Math.min(ctxMenu._cx - 6, ctxCard.x + ctxCard.width - 28))
                y: ctxMenu._above ? (ctxCard.y + ctxCard.height - 6) : (ctxCard.y - 6)
                opacity: ctxCard.opacity; scale: ctxCard.scale
            }

            Rectangle {
                id: ctxCard; width: 220; z: 2
                x: Math.max(8, Math.min(ctxMenu._cx - 110, content.width - 228))
                y: ctxMenu._cy
                height: ctxCol.height + 16
                radius: 16; clip: true; color: ChiTheme.colors.surfaceContainerHighest

                scale: ctxMenu.visible ? 1.0 : 0.92
                opacity: ctxMenu.visible ? 1.0 : 0.0
                transformOrigin: ctxMenu._above ? Item.Bottom : Item.Top
                Behavior on scale { NumberAnimation { duration: 120; easing.type: Easing.OutCubic } }
                Behavior on opacity { NumberAnimation { duration: 100 } }

                Column {
                    id: ctxCol; x: 0; y: 8; width: parent.width; spacing: 0

                    // Pin / Unpin
                    Rectangle {
                        width: parent.width; height: 48
                        color: _puH.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 16; spacing: 12
                            Rectangle {
                                width: 28; height: 28; radius: 14
                                anchors.verticalCenter: parent.verticalCenter
                                color: ChiTheme.colors.surfaceContainer
                                Icon {
                                    anchors.centerIn: parent; size: 16
                                    source: ctxMenu._isPinned ? "keep_off" : "push_pin"
                                    color: ChiTheme.colors.onSurfaceVariant
                                }
                            }
                            Text {
                                text: ctxMenu._isPinned ? "Unpin from Taskbar" : "Pin to Taskbar"
                                anchors.verticalCenter: parent.verticalCenter
                                color: ChiTheme.colors.onSurface
                                font.pixelSize: 14; font.weight: 500; font.family: ChiTheme.fontFamily
                            }
                        }
                        MouseArea {
                            id: _puH; anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (ctxMenu._isPinned) pinnedApps.unpin(ctxMenu.targetAppId)
                                else pinnedApps.pin(ctxMenu.targetAppId)
                                ctxMenu.close()
                            }
                        }
                    }

                    // App info
                    Rectangle {
                        width: parent.width; height: 48
                        color: _aiH.containsMouse ? Qt.rgba(1,1,1,0.08) : "transparent"
                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left; anchors.leftMargin: 16; spacing: 12
                            Rectangle {
                                width: 28; height: 28; radius: 14
                                anchors.verticalCenter: parent.verticalCenter
                                color: ChiTheme.colors.surfaceContainer
                                Icon {
                                    anchors.centerIn: parent; source: "info"; size: 16
                                    color: ChiTheme.colors.onSurfaceVariant
                                }
                            }
                            Text {
                                text: "App info"; anchors.verticalCenter: parent.verticalCenter
                                color: ChiTheme.colors.onSurface
                                font.pixelSize: 14; font.weight: 500; font.family: ChiTheme.fontFamily
                            }
                        }
                        MouseArea {
                            id: _aiH; anchors.fill: parent; hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: { console.log("App info:", ctxMenu.targetName); ctxMenu.close() }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: shell
        function onAppLauncherOpenChanged() {
            if (shell.appLauncherOpen) {
                searchInput.text = ""; content.selectedIndex = 0; ctxMenu.visible = false
                launcher._cardReveal = 0; launcher._wave = 0
                _cardAnim.start(); _waveAnim.start()
                searchInput.forceActiveFocus()
            }
        }
    }
}
