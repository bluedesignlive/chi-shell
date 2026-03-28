import QtQuick
import Chi 1.0

Column {
    id: grid
    width: parent.width
    spacing: 8

    // ── Build visible tile list from data ────────────
    property var allTiles: []

    // ── Visible indices (filters out hidden hardware) ─
    property var visibleIndices: {
        var r = [];
        for (var i = 0; i < allTiles.length; i++) {
            if (allTiles[i].vis()) r.push(i);
        }
        return r;
    }

    // ── Rebuild on any property change ───────────────
    onVisibleIndicesChanged: _build()
    Component.onCompleted: _build()

    // ── Tile data definitions ────────────────────────
    // Each tile: { icon, title, desc, active, wide, vis, toggle }
    // vis() returns true if hardware is present
    // toggle() fires on click

    // helpers to avoid binding loops in allTiles
    property bool _wifi:     systemStatus.wifiEnabled && systemStatus.wifiConnected
    property bool _wifiEn:   systemStatus.wifiEnabled
    property string _ssid:   systemStatus.wifiSSID
    property bool _bt:       systemStatus.bluetoothEnabled
    property bool _dark:     ChiTheme.isDarkMode
    property bool _dnd:      systemStatus.dndEnabled
    property bool _muted:    systemStatus.muted

    on_WifiChanged:   _rebuildData()
    on_WifiEnChanged: _rebuildData()
    on_SsidChanged:   _rebuildData()
    on_BtChanged:     _rebuildData()
    on_DarkChanged:   _rebuildData()
    on_DndChanged:    _rebuildData()
    on_MutedChanged:  _rebuildData()

    Component.onCompleted: _rebuildData()

    function _rebuildData() {
        var tiles = [];

        // WiFi
        tiles.push({
            icon: systemStatus.networkIcon,
            title: _wifi ? (_ssid || "WiFi") : "WiFi",
            desc: _wifi ? "Connected" : (_wifiEn ? "Not connected" : "Off"),
            active: _wifi,
            wide: true,
            vis: function() { return systemStatus.hasWifi },
            toggle: function() { systemStatus.wifiEnabled = !systemStatus.wifiEnabled }
        });

        // Bluetooth
        tiles.push({
            icon: systemStatus.bluetoothIcon,
            title: "Bluetooth",
            desc: _bt ? "On" : "Off",
            active: _bt,
            wide: true,
            vis: function() { return systemStatus.hasBluetooth },
            toggle: function() { systemStatus.bluetoothEnabled = !systemStatus.bluetoothEnabled }
        });

        // Dark Mode
        tiles.push({
            icon: _dark ? "dark_mode" : "light_mode",
            title: _dark ? "Dark" : "Light",
            desc: "Theme",
            active: _dark,
            wide: true,
            vis: function() { return true },
            toggle: function() { ChiTheme.toggleDarkMode() }
        });

        // DND (icon-only)
        tiles.push({
            icon: _dnd ? "do_not_disturb_on" : "do_not_disturb_off",
            title: "", desc: "",
            active: _dnd,
            wide: false,
            vis: function() { return true },
            toggle: function() { systemStatus.dndEnabled = !systemStatus.dndEnabled }
        });

        // Mute (icon-only)
        tiles.push({
            icon: _muted ? "volume_off" : systemStatus.audioIcon,
            title: "", desc: "",
            active: !_muted,
            wide: false,
            vis: function() { return true },
            toggle: function() { systemStatus.muted = !systemStatus.muted }
        });

        allTiles = tiles;
    }

    // ── Layout: builds rows from visible tiles ───────
    function _build() {
        for (var i = rep.children.length - 1; i >= 0; i--) {
            rep.children[i].destroy();
        }

        var vis = visibleIndices;
        var pos = 0;

        while (pos < vis.length) {
            // group tiles: wide = 1 slot, icon-only = pair them
            var slots = [];
            var j = pos;

            while (j < vis.length) {
                var t = allTiles[vis[j]];
                if (t.wide) {
                    // if a pending icon-only pair exists, flush it first
                    if (slots.length > 0 && slots.length % 2 !== 0) {
                        // single icon-only orphan — pad with empty
                        slots.push(null);
                    }
                    if (slots.length > 0 && slots[0].wide === false) {
                        // we had icon-only pending, flush
                        break;
                    }
                    slots.push(t);
                    j++;
                    break;
                } else {
                    slots.push(t);
                    j++;
                    if (slots.length >= 2) break;
                }
            }

            // if we stopped and have odd icon-only, pad
            if (slots.length > 0 && !slots[0].wide && slots.length % 2 !== 0) {
                slots.push(null);
            }

            // create QML row from slots
            var code = "import QtQuick\nimport Chi 1.0\nimport './statusbar' as SB\nRow {\n  width: parent.width\n  spacing: 8\n";

            for (var s = 0; s < slots.length; s++) {
                var tile = slots[s];
                if (tile === null) {
                    // empty spacer for padding
                    code += "  Item { width: (parent.width - 8) / 4 - 4; height: 72 }\n";
                } else if (tile.wide) {
                    code += "  SB.SettingsTile {\n";
                    code += "    width: (parent.width - 8) / 2; height: 72\n";
                    code += "    icon: \"" + tile.icon + "\"\n";
                    code += "    title: \"" + tile.title + "\"\n";
                    code += "    description: \"" + tile.desc + "\"\n";
                    code += "    active: " + tile.active + "\n";
                    code += "    wide: true\n";
                    code += "    onClicked: grid._click(" + vis[j - slots.length + s] + ")\n";
                    code += "  }\n";
                } else {
                    code += "  SB.SettingsTile {\n";
                    code += "    width: (parent.width - 8) / 4 - 4; height: 72\n";
                    code += "    icon: \"" + tile.icon + "\"\n";
                    code += "    active: " + tile.active + "\n";
                    code += "    wide: false\n";
                    code += "    onClicked: grid._click(" + vis[j - slots.length + s] + ")\n";
                    code += "  }\n";
                }
            }
            code += "  height: 72\n}";

            var row = Qt.createQmlObject(code, rep);
            pos = j;
        }
    }

    function _click(origIndex) {
        if (origIndex >= 0 && origIndex < allTiles.length) {
            allTiles[origIndex].toggle();
            _rebuildData();
        }
    }

    Item { id: rep; width: parent.width; height: childrenRect.height }
}
