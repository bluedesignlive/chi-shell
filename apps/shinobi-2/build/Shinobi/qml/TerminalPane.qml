import QtQuick
import Chi 1.0
import Shinobi 1.0

Item {
    id: pane

    property var session: null

    TerminalWidget {
        id: termWidget
        anchors.fill: parent
        session: pane.session

        // Theme-driven colors
        backgroundColor: ChiTheme.colors.surfaceContainerLowest
        foregroundColor: ChiTheme.colors.onSurface
        cursorColor: ChiTheme.colors.primary
        selectionColor: Qt.rgba(ChiTheme.colors.primaryContainer.r,
                                ChiTheme.colors.primaryContainer.g,
                                ChiTheme.colors.primaryContainer.b, 0.5)

        fontFamily: "Monospace"
        fontSize: 13
        padding: 6

        focus: true
    }

    // Click anywhere to focus the terminal
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
        if (session)
            termWidget.forceActiveFocus()
    }
}
