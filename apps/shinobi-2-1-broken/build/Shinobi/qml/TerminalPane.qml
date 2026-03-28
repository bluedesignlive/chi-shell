import QtQuick
import Chi 1.0
import Shinobi 1.0

// The pane is the terminal's window into the shell.
// It fills its parent completely — no margins, no border.
// The background color matches surfaceContainerLowest so the
// terminal grid blends seamlessly into the window.

Item {
    id: pane

    property var session: null

    TerminalWidget {
        id: termWidget
        anchors.fill: parent
        session: pane.session

        // ── Chi theme integration ───────────────────────────
        // The terminal background IS the window background —
        // using the darkest surface token so content sits naturally.
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

    // Click anywhere to grab keyboard focus
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
