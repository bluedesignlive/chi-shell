import QtQuick
import Chi 1.0

// Action buttons row for expanded notification
// actions is a QStringList: ["key1", "Label1", "key2", "Label2", ...]
Item {
    id: root
    width: parent.width
    height: 48

    property var    actions: []
    property int   notifId: 0
    property bool   showSnooze: true

    signal actionInvoked(int id, string actionKey)
    signal snoozeRequested(int id)

    Row {
        id: actionsRow
        anchors.left: parent.left
        anchors.right: snoozeBtn.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: 8

        Repeater {
            // actions come in pairs: [key, label, key, label, ...]
            model: {
                var result = [];
                if (root.actions) {
                    for (var i = 0; i + 1 < root.actions.length; i += 2) {
                        result.push({ key: root.actions[i], label: root.actions[i+1] });
                    }
                }
                // Max 3 action buttons
                return result.slice(0, 3);
            }

            delegate: Rectangle {
                width: Math.min(label.implicitWidth + 24, 96)
                height: 44
                radius: 40
                color: "transparent"
                border.width: 0

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: modelData.label
                    color: ChiTheme.colors.primary
                    font {
                        pixelSize: 14; weight: Font.Medium
                        family: ChiTheme.fontFamily
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    hoverEnabled: true
                    onEntered: parent.color = Qt.rgba(
                        ChiTheme.colors.primary.r,
                        ChiTheme.colors.primary.g,
                        ChiTheme.colors.primary.b, 0.08)
                    onExited:  parent.color = "transparent"
                    onClicked: root.actionInvoked(root.notifId, modelData.key)
                }
            }
        }
    }

    // Snooze button
    Item {
        id: snoozeBtn
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: 48; height: 48
        visible: root.showSnooze

        Rectangle {
            anchors.centerIn: parent
            width: 40; height: 40
            radius: 24
            color: "transparent"

            Icon {
                anchors.centerIn: parent
                source: "snooze"
                size: 24
                color: ChiTheme.colors.primary
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                hoverEnabled: true
                onEntered: parent.color = Qt.rgba(
                    ChiTheme.colors.primary.r,
                    ChiTheme.colors.primary.g,
                    ChiTheme.colors.primary.b, 0.08)
                onExited:  parent.color = "transparent"
                onClicked: root.snoozeRequested(root.notifId)
            }
        }
    }
}
