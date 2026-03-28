import QtQuick
import Chi 1.0

Item {
    id: card
    width: parent.width
    height: visible ? 184 : 0
    visible: mprisController.active

    Rectangle {
        id: container
        anchors.fill: parent
        radius: 28
        color: ChiTheme.colors.surfaceContainerHigh
        clip: true

        // ── Album art background (blurred) ───────────
        Image {
            id: artBg
            anchors.fill: parent
            anchors.margins: -4
            source: mprisController.artUrl
            fillMode: Image.PreserveAspectCrop
            visible: status === Image.Ready
            asynchronous: true
        }

        // Scrim over art
        Rectangle {
            anchors.fill: parent
            visible: artBg.visible
            gradient: Gradient {
                GradientStop { position: 0.0; color: Qt.rgba(0, 0, 0, 0.4) }
                GradientStop { position: 0.5; color: Qt.rgba(0, 0, 0, 0.6) }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, 0.8) }
            }
        }

        // Fallback solid background
        Rectangle {
            anchors.fill: parent
            color: ChiTheme.colors.surfaceContainerHigh
            visible: !artBg.visible
        }

        // ── Content ──────────────────────────────────
        Column {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 0

            // ── Header: app name ─────────────────────
            Item {
                width: parent.width; height: 28

                Text {
                    text: mprisController.playerName
                    color: cardTextColor
                    font { pixelSize: 11; weight: 500; family: ChiTheme.fontFamily }
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    opacity: 0.7
                }
            }

            // ── Spacer ───────────────────────────────
            Item { width: 1; height: 12 }

            // ── Body: title + artist + play button ───
            Item {
                width: parent.width
                height: 48

                Column {
                    anchors.left: parent.left
                    anchors.right: playBtn.left
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 0

                    Text {
                        text: mprisController.title || "No track"
                        width: parent.width
                        elide: Text.ElideRight
                        color: cardTextColor
                        font { pixelSize: 16; weight: 500; family: ChiTheme.fontFamily }
                    }
                    Text {
                        text: {
                            var parts = []
                            if (mprisController.artist) parts.push(mprisController.artist)
                            if (mprisController.album) parts.push(mprisController.album)
                            return parts.join(" — ")
                        }
                        width: parent.width
                        elide: Text.ElideRight
                        color: cardTextColor
                        font { pixelSize: 14; weight: 400; family: ChiTheme.fontFamily }
                        opacity: 0.7
                        visible: text !== ""
                    }
                }

                // Play/pause FAB
                Rectangle {
                    id: playBtn
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    width: 48; height: 40; radius: 20
                    color: artBg.visible
                           ? Qt.rgba(1, 1, 1, 0.2)
                           : ChiTheme.colors.primaryContainer

                    Icon {
                        anchors.centerIn: parent
                        source: mprisController.playing ? "pause" : "play_arrow"
                        size: 24
                        color: artBg.visible
                               ? "#FFFFFF"
                               : ChiTheme.colors.onPrimaryContainer
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: mprisController.playPause()
                        onPressed: playBtn.scale = 0.92
                        onReleased: playBtn.scale = 1.0
                        onCanceled: playBtn.scale = 1.0
                    }

                    Behavior on scale {
                        NumberAnimation { duration: 100 }
                    }
                }
            }

            // ── Spacer ───────────────────────────────
            Item { width: 1; height: 16 }

            // ── Scrubber ─────────────────────────────
            Item {
                width: parent.width; height: 4

                // Track background
                Rectangle {
                    anchors.fill: parent; radius: 2
                    color: artBg.visible
                           ? Qt.rgba(1, 1, 1, 0.2)
                           : ChiTheme.colors.outlineVariant
                }

                // Progress fill
                Rectangle {
                    height: parent.height; radius: 2
                    width: mprisController.duration > 0
                           ? parent.width * (mprisController.position / mprisController.duration)
                           : 0
                    color: artBg.visible
                           ? Qt.rgba(1, 1, 1, 0.8)
                           : ChiTheme.colors.primary
                }

                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -8
                    cursorShape: Qt.PointingHandCursor
                    onClicked: (mouse) => {
                        if (mprisController.duration > 0) {
                            var ratio = mouse.x / width
                            mprisController.seek(ratio * mprisController.duration)
                        }
                    }
                }
            }

            // ── Spacer ───────────────────────────────
            Item { width: 1; height: 12 }

            // ── Transport controls ───────────────────
            Row {
                anchors.horizontalCenter: parent.horizontalCenter
                spacing: 24

                // Previous
                Item {
                    width: 40; height: 40
                    opacity: mprisController.canPrev ? 1.0 : 0.3

                    Icon {
                        anchors.centerIn: parent
                        source: "skip_previous"; size: 24
                        color: cardTextColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        enabled: mprisController.canPrev
                        onClicked: mprisController.previous()
                    }
                }

                // Next
                Item {
                    width: 40; height: 40
                    opacity: mprisController.canNext ? 1.0 : 0.3

                    Icon {
                        anchors.centerIn: parent
                        source: "skip_next"; size: 24
                        color: cardTextColor
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        enabled: mprisController.canNext
                        onClicked: mprisController.next()
                    }
                }
            }
        }
    }

    // Text color: white on art, themed otherwise
    readonly property color cardTextColor:
        artBg.visible ? "#FFFFFF" : ChiTheme.colors.onSurface
}
