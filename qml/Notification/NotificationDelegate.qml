import QtQuick
import Chi 1.0

Item {
    id: root
    width: parent ? parent.width : 380
    height: card.height
    clip: true

    property int nId: 0
    property string nAppName: ""
    property string nAppIcon: ""
    property string nResolvedIcon: ""
    property string nDesktopEntry: ""
    property string nImagePath: ""
    property string nSummary: ""
    property string nBody: ""
    property var nActions: []
    property var nTimestamp: new Date()
    property int nUrgency: 1
    property bool nIsGrouped: false

    property bool expanded: false

    readonly property bool hasImage: nImagePath !== ""
    readonly property string imageSource: hasImage ? (nImagePath.indexOf("file://") === 0 ? nImagePath : "file://" + nImagePath) : ""

    readonly property bool expandable: (nBody !== "") || (nActions && nActions.length > 0) || hasImage
    readonly property string iconSource: nResolvedIcon !== "" ? nResolvedIcon
                                      : nAppIcon !== "" ? nAppIcon
                                      : nDesktopEntry !== "" ? nDesktopEntry
                                      : nAppName !== "" ? nAppName
                                      : "dialog-information"

    readonly property string displayType: {
        if (nIsGrouped) return "grouped";
        if (nUrgency === 0) return "low";
        return expanded ? "expanded" : "collapsed";
    }

    signal dismissed(int id)
    signal actionInvoked(int id, string actionKey)
    signal snoozeRequested(int id)

    Rectangle {
        id: card
        width: parent.width
        radius: 16
        color: ChiTheme.colors.surfaceContainerLow
        clip: true

        height: {
            switch (root.displayType) {
            case "grouped": return 24;
            case "low": return 56;
            case "expanded": return expandedCol.implicitHeight + 32;
            default: return 80;
            }
        }

        Behavior on height { NumberAnimation { duration: 180; easing.type: Easing.OutCubic } }
        Behavior on x { SpringAnimation { spring: 14.0; damping: 0.80 } }

        Rectangle {
            visible: root.nUrgency === 2
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 4
            radius: 2
            color: ChiTheme.colors.error
        }

        // --- GROUPED ---
        Item {
            visible: root.displayType === "grouped"
            anchors.fill: parent

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 52
                anchors.right: parent.right
                anchors.rightMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Text {
                    text: root.nSummary
                    color: ChiTheme.colors.onSurface
                    font { pixelSize: 16; weight: 600; family: ChiTheme.fontFamily }
                    anchors.verticalCenter: parent.verticalCenter
                    elide: Text.ElideRight
                    width: Math.min(implicitWidth, 110)
                }

                Text {
                    text: root.nBody
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 14; family: ChiTheme.fontFamily }
                    anchors.verticalCenter: parent.verticalCenter
                    width: parent.width - 120
                    elide: Text.ElideRight
                }
            }
        }

        // --- LOW ---
        Item {
            visible: root.displayType === "low"
            anchors.fill: parent

            NotificationLeadingIcon {
                id: lIcon
                iconSource: root.iconSource
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            Row {
                anchors.left: lIcon.right
                anchors.leftMargin: 14
                anchors.right: parent.right
                anchors.rightMargin: root.expandable ? 56 : 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    text: root.nSummary
                    color: ChiTheme.colors.onSurface
                    font { pixelSize: 16; weight: 600; family: ChiTheme.fontFamily }
                    elide: Text.ElideRight
                    width: Math.min(implicitWidth, parent.width - 60)
                }

                Text {
                    text: "·"
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 12; family: ChiTheme.fontFamily }
                }

                Text {
                    text: Qt.formatTime(root.nTimestamp, "h:mm")
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 12; family: ChiTheme.fontFamily }
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: { if (root.expandable) root.expanded = true }
            }
        }

        // --- COLLAPSED ---
        Item {
            visible: root.displayType === "collapsed"
            anchors.fill: parent

            NotificationLeadingIcon {
                id: cIcon
                iconSource: root.iconSource
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
            }

            Rectangle {
                id: cThumb
                visible: root.hasImage
                anchors.right: parent.right
                anchors.rightMargin: root.expandable ? 56 : 16
                anchors.verticalCenter: parent.verticalCenter
                width: 48
                height: 48
                radius: 10
                color: ChiTheme.colors.surfaceContainerHighest
                clip: true

                Image {
                    anchors.fill: parent
                    source: root.imageSource
                    fillMode: Image.PreserveAspectCrop
                }
            }

            Column {
                anchors.left: cIcon.right
                anchors.leftMargin: 14
                anchors.right: root.hasImage ? cThumb.left : parent.right
                anchors.rightMargin: root.hasImage ? 14 : (root.expandable ? 56 : 16)
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Row {
                    spacing: 4
                    width: parent.width

                    Text {
                        text: root.nSummary
                        color: ChiTheme.colors.onSurface
                        font { pixelSize: 16; weight: 600; family: ChiTheme.fontFamily }
                        elide: Text.ElideRight
                        width: Math.min(implicitWidth, parent.width - 60)
                    }

                    Text {
                        text: "·"
                        color: ChiTheme.colors.onSurfaceVariant
                        font { pixelSize: 12; family: ChiTheme.fontFamily }
                    }

                    Text {
                        text: Qt.formatTime(root.nTimestamp, "h:mm")
                        color: ChiTheme.colors.onSurfaceVariant
                        font { pixelSize: 12; family: ChiTheme.fontFamily }
                    }
                }

                Text {
                    visible: text !== ""
                    text: root.nBody
                    color: ChiTheme.colors.onSurfaceVariant
                    font { pixelSize: 14; family: ChiTheme.fontFamily }
                    width: parent.width
                    elide: Text.ElideRight
                    maximumLineCount: 1
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: { if (root.expandable) root.expanded = true }
            }
        }

        // --- EXPANDED ---
        Column {
            id: expandedCol
            visible: root.displayType === "expanded"
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 16
            spacing: 12

            Row {
                width: parent.width - 40
                spacing: 14

                NotificationLeadingIcon {
                    iconSource: root.iconSource
                }

                Column {
                    width: parent.width - 54
                    spacing: 4

                    Row {
                        spacing: 4
                        Text {
                            text: root.nAppName
                            color: ChiTheme.colors.onSurface
                            font { pixelSize: 12; weight: 600; family: ChiTheme.fontFamily }
                        }
                        Text {
                            text: "·"
                            color: ChiTheme.colors.onSurfaceVariant
                            font { pixelSize: 12; family: ChiTheme.fontFamily }
                        }
                        Text {
                            text: Qt.formatTime(root.nTimestamp, "h:mm")
                            color: ChiTheme.colors.onSurfaceVariant
                            font { pixelSize: 12; family: ChiTheme.fontFamily }
                        }
                    }

                    Text {
                        text: root.nSummary
                        color: ChiTheme.colors.onSurface
                        font { pixelSize: 16; weight: 600; family: ChiTheme.fontFamily }
                        width: parent.width
                        wrapMode: Text.WordWrap
                    }

                    Text {
                        visible: text !== ""
                        text: root.nBody
                        color: ChiTheme.colors.onSurfaceVariant
                        font { pixelSize: 14; family: ChiTheme.fontFamily }
                        width: parent.width
                        wrapMode: Text.WordWrap
                    }
                }
            }

            Rectangle {
                visible: root.hasImage
                width: parent.width
                height: width * (199 / 294)
                radius: 16
                color: ChiTheme.colors.surfaceContainerHighest
                clip: true

                Image {
                    anchors.fill: parent
                    source: root.imageSource
                    fillMode: Image.PreserveAspectCrop
                }
            }

            NotificationActions {
                visible: root.nActions && root.nActions.length > 0
                width: parent.width
                actions: root.nActions
                notifId: root.nId
                onActionInvoked: (id, key) => root.actionInvoked(id, key)
                onSnoozeRequested: (id) => root.snoozeRequested(id)
            }
        }

        // Body click to collapse when expanded
        MouseArea {
            visible: root.displayType === "expanded"
            anchors.fill: parent
            z: -1
            onClicked: root.expanded = false
        }

        // --- EXPAND/COLLAPSE TOGGLE BUTTON ---
        Rectangle {
            visible: root.displayType !== "grouped" && root.expandable
            z: 3
            anchors.right: parent.right
            anchors.rightMargin: 16
            anchors.top: parent.top
            anchors.topMargin: root.displayType === "low" ? (parent.height - height) / 2 : 16
            width: 36
            height: 24
            radius: 28
            color: ChiTheme.colors.surface

            Icon {
                anchors.centerIn: parent
                source: root.expanded ? "keyboard_arrow_up" : "keyboard_arrow_down"
                size: 24
                color: ChiTheme.colors.onSurface
            }

            MouseArea {
                anchors.fill: parent
                onClicked: root.expanded = !root.expanded
            }
        }
    }

    DragHandler {
        id: swipe
        target: card
        xAxis.enabled: true
        yAxis.enabled: false

        onActiveChanged: {
            if (!active) {
                if (Math.abs(card.x) > root.width * 0.40)
                    root.dismissed(root.nId)
                else
                    card.x = 0
            }
        }
    }
}
