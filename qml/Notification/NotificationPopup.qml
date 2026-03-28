import QtQuick
import Chi 1.0

Item {
    id: root
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.topMargin: 12
    height: popupCard.height + 16
    visible: false
    z: 999

    property int popupId: 0
    property string popupAppName: ""
    property string popupSummary: ""
    property string popupBody: ""
    property string popupAppIcon: ""
    property string popupImagePath: ""
    property int popupUrgency: 1
    property var popupActions: []
    property int displayDuration: 5000

    readonly property bool hasImage: popupImagePath !== ""
    readonly property string imageSource: hasImage ? (popupImagePath.indexOf("file://") === 0 ? popupImagePath : "file://" + popupImagePath) : ""

    // Clock target for fly animation
    readonly property real clockTargetX: 40
    readonly property real clockTargetY: -4

    signal popupDismissed()
    signal popupTapped()
    signal popupActionInvoked(int id, string actionKey)

    function show(id, appName, summary, body, appIcon, imagePath, urgency, actions) {
        popupId = id
        popupAppName = appName || ""
        popupSummary = summary || ""
        popupBody = body || ""
        popupAppIcon = appIcon || ""
        popupImagePath = imagePath || ""
        popupUrgency = urgency === undefined ? 1 : urgency
        popupActions = actions || []

        if (popupUrgency === 0)
            displayDuration = 3000
        else if (popupUrgency === 2)
            displayDuration = 0
        else
            displayDuration = 5000

        visible = true
        popupCard.x = (root.width - popupCard.width) / 2
        popupCard.y = -popupCard.height - 20
        popupCard.scale = 1
        popupCard.opacity = 0
        showAnim.restart()

        if (displayDuration > 0) {
            autoDismissTimer.interval = displayDuration
            autoDismissTimer.restart()
        } else {
            autoDismissTimer.stop()
        }
    }

    function hide(flyTowardsClock) {
        if (!visible) return
        autoDismissTimer.stop()
        if (flyTowardsClock)
            flyToClockAnim.restart()
        else
            hideAnim.restart()
    }

    Timer {
        id: autoDismissTimer
        repeat: false
        onTriggered: root.hide(true)
    }

    // Show: bouncy slide down
    ParallelAnimation {
        id: showAnim
        NumberAnimation {
            target: popupCard; property: "y"
            from: -popupCard.height - 20; to: 4
            duration: 350; easing.type: Easing.OutBack
        }
        NumberAnimation {
            target: popupCard; property: "opacity"
            from: 0; to: 1; duration: 250
        }
    }

    // Hide: slide up (on tap)
    ParallelAnimation {
        id: hideAnim
        NumberAnimation {
            target: popupCard; property: "y"
            to: -popupCard.height - 20
            duration: 250; easing.type: Easing.InCubic
        }
        NumberAnimation {
            target: popupCard; property: "opacity"
            to: 0; duration: 200
        }
        onFinished: {
            root.visible = false
            root.popupDismissed()
        }
    }

    // Hide: fly to clock (auto-dismiss)
    ParallelAnimation {
        id: flyToClockAnim
        NumberAnimation {
            target: popupCard; property: "x"
            to: root.clockTargetX
            duration: 400; easing.type: Easing.InOutCubic
        }
        NumberAnimation {
            target: popupCard; property: "y"
            to: root.clockTargetY
            duration: 400; easing.type: Easing.InOutCubic
        }
        NumberAnimation {
            target: popupCard; property: "scale"
            to: 0.05
            duration: 400; easing.type: Easing.InCubic
        }
        NumberAnimation {
            target: popupCard; property: "opacity"
            to: 0; duration: 350
        }
        onFinished: {
            popupCard.scale = 1
            root.visible = false
            root.popupDismissed()
        }
    }

    Rectangle {
        id: popupCard
        width: Math.min(parent.width - 16, 404)
        height: contentCol.height + 24
        radius: 24
        color: ChiTheme.colors.surfaceContainerHigh
        border.width: 1
        border.color: Qt.rgba(1, 1, 1, 0.06)
        y: -height - 20
        opacity: 0

        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.hide(false)
                root.popupTapped()
            }
        }

        Rectangle {
            visible: root.popupUrgency === 2
            anchors.left: parent.left; anchors.top: parent.top; anchors.bottom: parent.bottom
            width: 4; radius: 2; color: ChiTheme.colors.error
        }

        Column {
            id: contentCol
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 8

            Row {
                width: parent.width
                spacing: 12

                NotificationLeadingIcon {
                    iconSource: root.popupAppIcon
                    anchors.verticalCenter: parent.verticalCenter
                }

                Column {
                    width: parent.width - 52 - (root.hasImage ? 60 : 0)
                    spacing: 4

                    Row {
                        spacing: 4
                        Text { text: root.popupAppName; color: ChiTheme.colors.onSurfaceVariant; font { pixelSize: 12; weight: 600; family: ChiTheme.fontFamily } }
                        Text { text: "·"; color: ChiTheme.colors.onSurfaceVariant; font { pixelSize: 12; family: ChiTheme.fontFamily } }
                        Text { text: "now"; color: ChiTheme.colors.onSurfaceVariant; font { pixelSize: 12; family: ChiTheme.fontFamily } }
                    }

                    Text {
                        text: root.popupSummary
                        color: ChiTheme.colors.onSurface
                        font { pixelSize: 16; weight: 600; family: ChiTheme.fontFamily }
                        width: parent.width
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2; elide: Text.ElideRight
                    }

                    Text {
                        visible: text !== ""
                        text: root.popupBody
                        color: ChiTheme.colors.onSurfaceVariant
                        font { pixelSize: 14; family: ChiTheme.fontFamily }
                        width: parent.width
                        wrapMode: Text.WordWrap
                        maximumLineCount: 2; elide: Text.ElideRight
                    }
                }

                // Rounded popup thumbnail
                Rectangle {
                    visible: root.hasImage
                    width: 48; height: 48; radius: 10
                    color: ChiTheme.colors.surfaceContainerHighest
                    clip: true
                    anchors.verticalCenter: parent.verticalCenter

                    Image {
                        anchors.fill: parent
                        source: root.imageSource
                        fillMode: Image.PreserveAspectCrop
                    }
                }
            }

            Row {
                visible: root.popupActions && root.popupActions.length > 0
                spacing: 8
                width: parent.width

                Repeater {
                    model: {
                        var out = []
                        if (root.popupActions) {
                            for (var i = 0; i + 1 < root.popupActions.length; i += 2)
                                out.push({ key: root.popupActions[i], label: root.popupActions[i + 1] })
                        }
                        return out.slice(0, 2)
                    }

                    delegate: Rectangle {
                        width: Math.min(label.implicitWidth + 24, 120)
                        height: 36; radius: 18
                        color: Qt.rgba(ChiTheme.colors.primary.r, ChiTheme.colors.primary.g, ChiTheme.colors.primary.b, 0.10)

                        Text {
                            id: label; anchors.centerIn: parent
                            text: modelData.label
                            color: ChiTheme.colors.primary
                            font { pixelSize: 13; weight: 600; family: ChiTheme.fontFamily }
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: { root.popupActionInvoked(root.popupId, modelData.key); root.hide(false) }
                        }
                    }
                }
            }
        }
    }
}
