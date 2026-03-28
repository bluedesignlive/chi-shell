import QtQuick
import Chi 1.0
import Shinobi 1.0

Item {
    id: pane
    property var session: null

    // The TerminalWidget paints its own background using
    // surfaceContainerLowest, filling the entire pane.
    // No wrapper rectangle needed.

    TerminalWidget {
        id: termWidget
        anchors.fill: parent
        session: pane.session

        backgroundColor: ChiTheme.colors.surfaceContainerLowest
        foregroundColor: ChiTheme.colors.onSurface
        cursorColor: ChiTheme.colors.primary
        selectionColor: Qt.rgba(ChiTheme.colors.primaryContainer.r,
                                ChiTheme.colors.primaryContainer.g,
                                ChiTheme.colors.primaryContainer.b, 0.4)

        fontFamily: "Monospace"
        fontSize: 13
        padding: 8

        focus: true
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        propagateComposedEvents: true
        onPressed: function(mouse) {
            termWidget.forceActiveFocus()
            mouse.accepted = false
        }
    }

    onSessionChanged: {
        if (session) termWidget.forceActiveFocus()
    }
}
