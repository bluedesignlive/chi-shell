import QtQuick
import Chi 1.0

Item {
    id: wifiPanel
    signal close()

    Column {
        anchors.fill: parent
        spacing: 0

        // ── Header ───────────────────────────────────
        Item {
            width: parent.width; height: 56

            // Back button
            Rectangle {
                id: backBtn
                width: 40; height: 40; radius: 1000
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                color: backMouse.containsMouse
                       ? ChiTheme.colors.surfaceContainerHigh
                       : "transparent"

                Icon {
                    anchors.centerIn: parent
                    source: "arrow_back"; size: 22
                    color: ChiTheme.colors.onSurface
                }

                MouseArea {
                    id: backMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: wifiPanel.close()
                }
            }

            Text {
                text: "Wi-Fi"
                anchors.left: backBtn.right
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                color: ChiTheme.colors.onSurface
                font { pixelSize: 20; weight: 500; family: ChiTheme.fontFamily }
            }

            // Toggle pill
            Rectangle {
                id: toggle
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: 52; height: 32; radius: 16
                color: wifiManager.enabled
                       ? ChiTheme.colors.primary
                       : ChiTheme.colors.surfaceContainerHighest

                Behavior on color { ColorAnimation { duration: 200 } }

                Rectangle {
                    x: wifiManager.enabled ? parent.width - width - 4 : 4
                    y: 4; width: 24; height: 24; radius: 12
                    color: wifiManager.enabled
                           ? ChiTheme.colors.onPrimary
                           : ChiTheme.colors.outline

                    Behavior on x {
                        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: wifiManager.enabled = !wifiManager.enabled
                }
            }
        }

        // ── Separator ────────────────────────────────
        Rectangle {
            width: parent.width; height: 1
            color: ChiTheme.colors.outlineVariant; opacity: 0.2
        }

        // ── WiFi off message ─────────────────────────
        Item {
            width: parent.width
            height: parent.height - 57
            visible: !wifiManager.enabled

            Column {
                anchors.centerIn: parent
                spacing: 12

                Icon {
                    source: "wifi_off"; size: 48
                    color: ChiTheme.colors.onSurfaceVariant
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "Wi-Fi is turned off"
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 16; weight: 400; family: ChiTheme.fontFamily }
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }
        }

        // ── Network list ─────────────────────────────
        Item {
            width: parent.width
            height: parent.height - 57
            visible: wifiManager.enabled

            // Scanning indicator
            Row {
                id: scanBar
                anchors.top: parent.top
                anchors.topMargin: 8
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 6
                visible: wifiManager.scanning
                opacity: wifiManager.scanning ? 1 : 0

                Behavior on opacity { NumberAnimation { duration: 200 } }

                Icon {
                    source: "wifi_find"; size: 16
                    color: ChiTheme.colors.primary
                    anchors.verticalCenter: parent.verticalCenter

                    SequentialAnimation on opacity {
                        loops: Animation.Infinite; running: wifiManager.scanning
                        NumberAnimation { to: 0.3; duration: 600 }
                        NumberAnimation { to: 1.0; duration: 600 }
                    }
                }
                Text {
                    text: "Scanning..."
                    color: ChiTheme.colors.primary
                    font { pixelSize: 13; weight: 500; family: ChiTheme.fontFamily }
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            ListView {
                id: networkList
                anchors.fill: parent
                anchors.topMargin: wifiManager.scanning ? 32 : 8
                clip: true
                spacing: 2
                model: wifiManager

                property int expandedIndex: -1

                delegate: Column {
                    id: delegateRoot
                    width: networkList.width

                    property bool isExpanded: networkList.expandedIndex === index
                    property string errorMsg: ""

                    // ── Network row ──────────────────
                    Rectangle {
                        width: parent.width; height: 56; radius: 16
                        color: model.connected
                               ? ChiTheme.colors.primaryContainer
                               : rowMouse.containsMouse
                                 ? ChiTheme.colors.surfaceContainerHigh
                                 : "transparent"

                        Behavior on color { ColorAnimation { duration: 100 } }

                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 12

                            // Signal icon
                            Icon {
                                source: model.signalIcon; size: 22
                                color: model.connected
                                       ? ChiTheme.colors.onPrimaryContainer
                                       : ChiTheme.colors.onSurfaceVariant
                                anchors.verticalCenter: parent.verticalCenter
                            }

                            // SSID + subtitle
                            Column {
                                anchors.verticalCenter: parent.verticalCenter
                                width: parent.width - 80
                                spacing: 0

                                Text {
                                    text: model.ssid
                                    width: parent.width
                                    elide: Text.ElideRight
                                    color: model.connected
                                           ? ChiTheme.colors.onPrimaryContainer
                                           : ChiTheme.colors.onSurface
                                    font {
                                        pixelSize: 14; weight: 500
                                        family: ChiTheme.fontFamily
                                    }
                                }
                                Text {
                                    visible: model.connected || model.saved
                                    text: model.connected ? "Connected"
                                                          : model.saved ? "Saved" : ""
                                    color: model.connected
                                           ? ChiTheme.colors.onPrimaryContainer
                                           : ChiTheme.colors.onSurfaceVariant
                                    font {
                                        pixelSize: 12; weight: 400
                                        family: ChiTheme.fontFamily
                                    }
                                    opacity: 0.7
                                }
                            }

                            // Lock icon
                            Icon {
                                source: "lock"; size: 18
                                color: model.connected
                                       ? ChiTheme.colors.onPrimaryContainer
                                       : ChiTheme.colors.onSurfaceVariant
                                visible: model.secured
                                anchors.verticalCenter: parent.verticalCenter
                                opacity: 0.6
                            }
                        }

                        MouseArea {
                            id: rowMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (model.connected) {
                                    // toggle expand for disconnect/forget
                                    networkList.expandedIndex =
                                        delegateRoot.isExpanded ? -1 : index
                                } else if (!model.secured || model.saved) {
                                    // open or saved → connect directly
                                    wifiManager.connectToNetwork(index)
                                } else {
                                    // secured → expand for password
                                    networkList.expandedIndex =
                                        delegateRoot.isExpanded ? -1 : index
                                    delegateRoot.errorMsg = ""
                                }
                            }
                        }
                    }

                    // ── Expanded panel ───────────────
                    Rectangle {
                        width: parent.width - 16
                        anchors.horizontalCenter: parent.horizontalCenter
                        height: delegateRoot.isExpanded ? expandedContent.height + 16 : 0
                        radius: 12
                        color: ChiTheme.colors.surfaceContainerHigh
                        clip: true
                        visible: height > 0

                        Behavior on height {
                            NumberAnimation {
                                duration: 200
                                easing.type: Easing.OutCubic
                            }
                        }

                        Column {
                            id: expandedContent
                            width: parent.width - 16
                            anchors.centerIn: parent
                            spacing: 8

                            // ── Connected actions ────
                            Row {
                                spacing: 8
                                visible: model.connected

                                Rectangle {
                                    width: disconnectText.width + 24
                                    height: 36; radius: 18
                                    color: ChiTheme.colors.error

                                    Text {
                                        id: disconnectText
                                        anchors.centerIn: parent
                                        text: "Disconnect"
                                        color: ChiTheme.colors.onError
                                        font {
                                            pixelSize: 13; weight: 500
                                            family: ChiTheme.fontFamily
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            wifiManager.disconnectFromNetwork()
                                            networkList.expandedIndex = -1
                                        }
                                    }
                                }

                                Rectangle {
                                    width: forgetText.width + 24
                                    height: 36; radius: 18
                                    color: ChiTheme.colors.surfaceContainerHighest
                                    visible: model.saved

                                    Text {
                                        id: forgetText
                                        anchors.centerIn: parent
                                        text: "Forget"
                                        color: ChiTheme.colors.onSurface
                                        font {
                                            pixelSize: 13; weight: 500
                                            family: ChiTheme.fontFamily
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            wifiManager.forgetNetwork(index)
                                            networkList.expandedIndex = -1
                                        }
                                    }
                                }
                            }

                            // ── Password input ───────
                            Column {
                                spacing: 8
                                visible: !model.connected && model.secured && !model.saved

                                Rectangle {
                                    width: expandedContent.width
                                    height: 44; radius: 12
                                    color: ChiTheme.colors.surfaceContainerLowest
                                    border.width: pwInput.activeFocus ? 2 : 1
                                    border.color: pwInput.activeFocus
                                                  ? ChiTheme.colors.primary
                                                  : ChiTheme.colors.outline

                                    TextInput {
                                        id: pwInput
                                        anchors.fill: parent
                                        anchors.leftMargin: 14
                                        anchors.rightMargin: 44
                                        verticalAlignment: TextInput.AlignVCenter
                                        echoMode: showPw.checked
                                                  ? TextInput.Normal
                                                  : TextInput.Password
                                        color: ChiTheme.colors.onSurface
                                        font {
                                            pixelSize: 14
                                            family: ChiTheme.fontFamily
                                        }
                                        clip: true

                                        onAccepted: {
                                            if (text.length > 0)
                                                wifiManager.connectToNetwork(index, text)
                                        }

                                        Text {
                                            visible: !pwInput.text && !pwInput.activeFocus
                                            text: "Password"
                                            color: ChiTheme.colors.onSurfaceVariant
                                            font: pwInput.font
                                            anchors.verticalCenter: parent.verticalCenter
                                        }
                                    }

                                    // show/hide toggle
                                    Icon {
                                        id: showPw
                                        property bool checked: false
                                        anchors.right: parent.right
                                        anchors.rightMargin: 10
                                        anchors.verticalCenter: parent.verticalCenter
                                        source: checked ? "visibility" : "visibility_off"
                                        size: 20
                                        color: ChiTheme.colors.onSurfaceVariant

                                        MouseArea {
                                            anchors.fill: parent
                                            anchors.margins: -6
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: showPw.checked = !showPw.checked
                                        }
                                    }
                                }

                                // Error message
                                Text {
                                    visible: delegateRoot.errorMsg !== ""
                                    text: delegateRoot.errorMsg
                                    color: ChiTheme.colors.error
                                    font {
                                        pixelSize: 12; weight: 400
                                        family: ChiTheme.fontFamily
                                    }
                                }

                                // Connect button
                                Rectangle {
                                    width: connectLabel.width + 32
                                    height: 36; radius: 18
                                    color: wifiManager.connecting
                                           ? ChiTheme.colors.surfaceContainerHighest
                                           : ChiTheme.colors.primary

                                    Text {
                                        id: connectLabel
                                        anchors.centerIn: parent
                                        text: wifiManager.connecting
                                              ? "Connecting..."
                                              : "Connect"
                                        color: wifiManager.connecting
                                               ? ChiTheme.colors.onSurfaceVariant
                                               : ChiTheme.colors.onPrimary
                                        font {
                                            pixelSize: 13; weight: 500
                                            family: ChiTheme.fontFamily
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        enabled: !wifiManager.connecting
                                        onClicked: {
                                            if (pwInput.text.length >= 8) {
                                                wifiManager.connectToNetwork(
                                                    index, pwInput.text)
                                            } else {
                                                delegateRoot.errorMsg =
                                                    "Password must be at least 8 characters"
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ── Empty state ──────────────────────────
            Column {
                anchors.centerIn: parent
                spacing: 8
                visible: networkList.count === 0 && !wifiManager.scanning

                Icon {
                    source: "wifi_find"; size: 40
                    color: ChiTheme.colors.onSurfaceVariant
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Text {
                    text: "No networks found"
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 14; family: ChiTheme.fontFamily }
                    anchors.horizontalCenter: parent.horizontalCenter
                }
                Rectangle {
                    width: scanLabel.width + 24; height: 36; radius: 18
                    color: ChiTheme.colors.primaryContainer
                    anchors.horizontalCenter: parent.horizontalCenter

                    Text {
                        id: scanLabel
                        anchors.centerIn: parent
                        text: "Scan"
                        color: ChiTheme.colors.onPrimaryContainer
                        font { pixelSize: 13; weight: 500; family: ChiTheme.fontFamily }
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: wifiManager.scan()
                    }
                }
            }
        }
    }

    // ── Connect result handler ───────────────────────
    Connections {
        target: wifiManager
        function onConnectResult(success, message) {
            if (success) {
                networkList.expandedIndex = -1
            } else {
                // find expanded delegate and set error
                if (networkList.expandedIndex >= 0) {
                    var item = networkList.itemAtIndex(networkList.expandedIndex)
                    if (item) item.errorMsg = message
                }
            }
        }
    }

    // ── On open: scan ────────────────────────────────
    onVisibleChanged: {
        if (visible) {
            networkList.expandedIndex = -1
            wifiManager.scan()
        }
    }
}
