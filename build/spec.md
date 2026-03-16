# Chi Shell — Desktop Shell Specification

Version 0.1.0 — MVP
A Material 3 Expressive desktop shell for Wayfire, built with Chi UI (Qt/QML).


## 1. Overview

Chi Shell is the desktop environment for ChiOS, built on top of the
Wayfire compositor. It provides a modern, mobile-inspired desktop
experience using Chi UI components and Material 3 Expressive design.

The shell targets modern users — creatives, students, Gen-Z — who
expect their desktop to feel as alive and intuitive as their phone.


### 1.1 Design Principles

- descriptive naming: every element name says what it does
- speed first: idle CPU near zero, startup under 2 seconds
- mobile-familiar: patterns people already know from their phones
- M3 Expressive: spring physics, dynamic color, expressive shapes
- standards-based: freedesktop specs, Wayland protocols, D-Bus


### 1.2 Naming Convention

Every user-facing element has a self-descriptive name. If you read
the name, you know what it does.

  StatusBar       — shows system status (top bar)
  Taskbar         — monitors and manages tasks (bottom bar)
  QuickSettings   — quick access to system settings
  NotificationCenter — center for all notifications
  AppLauncher     — launches applications
  Desktop         — the desktop itself (wallpaper + widgets)
  LiveInfo        — displays live information (contextual pill)
  BrightnessSlider — slides to adjust brightness
  SettingsTile    — a tile that controls a setting
  MediaPlayer     — plays and controls media

No abstract or clever names. No branding in component names.


### 1.3 Why Wayfire

Wayfire is a Wayland compositor built on wlroots. It provides IPC
via a socket (WAYFIRE_SOCKET env var) and a plugin API for custom
compositor-level animations and window management.

Required Wayfire plugins:
- ipc + ipc-rules — socket-based compositor control
- wsets — workspace sets (virtual desktops)
- scale — window overview
- foreign-toplevel — window list for Taskbar
- wayfire-shell — extra shell protocol for panels/docks
- blur — background blur for QuickSettings


## 2. Architecture


### 2.1 Directory Structure

    chi-shell/
    |-- daemon/
    |   |-- main.cpp                entry point, creates all surfaces
    |   |-- WaylandBackend          layer-shell surface management
    |   |-- WindowTracker           wlr-foreign-toplevel client
    |   |-- NotificationServer      org.freedesktop.Notifications
    |   |-- SystemServices          D-Bus: network, bluetooth, audio, power
    |   |-- WayfireIPC              WAYFIRE_SOCKET JSON client
    |
    |-- qml/
    |   |-- StatusBar.qml
    |   |-- Taskbar.qml
    |   |-- QuickSettings.qml
    |   |-- NotificationCenter.qml
    |   |-- AppLauncher.qml
    |   |-- Desktop.qml
    |   |-- LiveInfo.qml
    |
    |-- plugins/
    |   |-- chi-minimize/           squeeze-to-taskbar Wayfire plugin
    |
    |-- CMakeLists.txt


### 2.2 Layer Shell Surfaces

Every visible shell element is a wlr-layer-shell surface. The
layer-shell is organized into four discrete layers: background,
bottom, top, and overlay, rendered in that order. Between bottom
and top, application windows are displayed.

Each surface declares an anchor (which edges it sticks to) and an
exclusive zone (space reserved so apps don't overlap it).

  Surface              Layer       Anchor        Exclusive Zone   Keyboard
  -------              -----       ------        --------------   --------
  Desktop              background  all edges     -1 (stretch)     none
  Taskbar              bottom      bottom+L+R    bar height       on-demand
  StatusBar            top         top+L+R       bar height       none
  LiveInfo             top         top (center)  0 (float)        on-demand
  QuickSettings        overlay     top+R         0 (float)        exclusive
  NotificationCenter   overlay     top+R         0 (float)        exclusive
  AppLauncher          overlay     all edges     0 (float)        exclusive

For Qt/QML integration with layer-shell, KDE provides layer-shell-qt.
It offers LayerShellQt::Shell::useLayerShell() to enable layer-shell
before any windows are created, and LayerShellQt::Window for per-surface
settings (layer, anchor, exclusive zone, margins).


### 2.3 Wayfire IPC

Wayfire provides a JSON socket at WAYFIRE_SOCKET. Chi Shell uses it for:
- listing open windows (supplement to foreign-toplevel)
- activating scale view filtered by app_id
- switching workspaces
- querying taskbar icon position for minimize animation
- setting window rules


### 2.4 D-Bus Services

  Bus       Interface                        Purpose
  ---       ---------                        -------
  session   org.freedesktop.Notifications    receive notifications
  system    org.freedesktop.NetworkManager   wifi/ethernet status
  system    org.bluez                        bluetooth status
  system    org.freedesktop.UPower           battery info
  session   org.freedesktop.portal.Desktop   file dialogs, etc.
  session   org.mpris.MediaPlayer2           now-playing info
  session   org.freedesktop.login1           power actions
  session   PulseAudio / PipeWire            volume control


## 3. Color System

Chi Shell inherits from ChiTheme. All surfaces read ChiTheme.colors
which adapts to dark/light mode and user-selected primary color.


### 3.1 Core Tokens (Dark Mode Default)

From ChiTheme._seedDark with seed #673AB7:

  Token                     Hex        Usage
  -----                     ---        -----
  primary                   #D3BCFD    active toggles, accents
  onPrimary                 #38265C    text on primary
  primaryContainer          #4F3D74    active tile background
  onPrimaryContainer        #EBDDFF    text on active tile
  surface                   #151218    shell background
  onSurface                 #E7E0E8    primary text
  surfaceContainer          #211F24    taskbar/statusbar background
  surfaceContainerHigh      #2C292F    card/tile inactive background
  surfaceContainerHighest   #36343B    elevated surfaces
  onSurfaceVariant          #CBC4CF    secondary text, icons
  outline                   #948F99    borders, dividers
  outlineVariant            #49454E    subtle dividers
  error                     #FFB4AB    error states
  secondaryContainer        #4B4358    secondary active tiles
  tertiaryContainer         #643B46    tertiary accents


### 3.2 Quick Settings Tile States

  State               Background                  Icon       Text
  -----               ----------                  ----       ----
  active              primaryContainer             onPrimary  onPrimary
  inactive            surfaceContainerHigh         onSurface  onSurface
  icon-only active    primaryContainer             onPrimary  —
  icon-only inactive  surfaceContainerHigh         onSurface  —
  disabled            surfaceContainerHigh @ 0.38  onSurface @ 0.38  —


### 3.3 Dynamic Color

When user changes primary color, ChiTheme._genDark() or _genLight()
algorithmically generates the full palette. The entire shell recolors
instantly — every surface reads from ChiTheme.colors.


## 4. Motion System

M3 Expressive replaces duration-based animation with spring physics.
A spring animation is defined by stiffness (how fast) and damping
ratio (how much bounce). This makes animations interruptible and
natural — when a gesture redirects mid-animation, the spring
seamlessly calculates a new trajectory.


### 4.1 Motion Schemes

Chi Shell defaults to the Expressive scheme.

  Expressive: lower damping = visible overshoot and bounce.
              suited for hero moments and key interactions.

  Standard:   higher damping = minimal bounce.
              suited for utilitarian applications.


### 4.2 Spring Tokens

Each token type is available in three speeds. The speed is chosen
based on the size of the component or distance covered.

  Token                  Stiffness  Damping   Use Case
  -----                  ---------  -------   --------
  springFastSpatial      1400       0.75      small: toggles, switches, icons
  springFastEffects      1400       1.0       fade/color on small elements
  springDefaultSpatial   700        0.80      medium: tiles, cards, panels
  springDefaultEffects   700        1.0       fade/color on medium elements
  springSlowSpatial      300        0.85      large: fullscreen, page transitions
  springSlowEffects      300        1.0       fade/color on large overlays

QML implementation in ChiTheme:

    readonly property var spring: ({
        fastSpatial:    { stiffness: 1400, damping: 0.75 },
        fastEffects:    { stiffness: 1400, damping: 1.0  },
        defaultSpatial: { stiffness: 700,  damping: 0.80 },
        defaultEffects: { stiffness: 700,  damping: 1.0  },
        slowSpatial:    { stiffness: 300,  damping: 0.85 },
        slowEffects:    { stiffness: 300,  damping: 1.0  }
    })

QML usage:

    Behavior on y {
        SpringAnimation {
            spring: 7.0   // stiffness / 100
            damping: 0.8
        }
    }


### 4.3 Shell Motion Map

  Action                         Spring Token        Notes
  ------                         ------------        -----
  QuickSettings slide down       defaultSpatial      slight bounce on open
  QuickSettings close            defaultEffects      no bounce on close
  notification arrive            fastSpatial         slide in from top
  notification dismiss (swipe)   fastSpatial         follow finger velocity
  Taskbar icon press             fastSpatial         scale 0.9 -> 1.0
  AppLauncher open               slowSpatial         fullscreen morph
  AppLauncher close              slowEffects         fade + scale to 0.96
  tile toggle on/off             fastSpatial         icon + bg color change
  LiveInfo expand                defaultSpatial      pill morph wider
  LiveInfo collapse              defaultEffects      no bounce on close
  BrightnessSlider drag          fastEffects         track fill follows thumb
  minimize to Taskbar            slowSpatial         compositor-level plugin


### 4.4 Blur

QuickSettings and NotificationCenter use background blur.
Wayfire provides blur as a built-in plugin.

Blur values from design:
- StatusBar: 75px blur radius (light blur, transparent)
- QuickSettings scrim: wallpaper blur 10px + white hard-light @ 0.6
- MediaPlayer: 500px blur (heavy, effectively solid tinted surface)
- small widgets (icons, output chip): 75px blur


## 5. Shell Components


### 5.1 StatusBar

The top bar. Always visible. Shows essential system info.

    +------------------------------------------------------------+
    |  9:41  Mon, Jun 15     alarm vpn vibr   wifi cell  bat 78% |
    |  <- time + date          settings        connectivity batt->|
    +------------------------------------------------------------+

  layer: top
  anchor: top + left + right
  exclusive zone: 36px (compact) or 48px (with expanded date)
  height: 36-48px

Layout (from Figma):
- left group: time (weight 500, 14px) + date (weight 500, 14px)
- center: reserved (camera/notch area on mobile, empty on desktop)
- right group:
  - settings icons: alarm, vpn, vibrate (18x18 each, 4px gap)
  - connectivity: wifi + cell signal (18x18, -2px overlap)
  - battery: icon (18x18) + percentage label (weight 500, 14px)

Font: ChiTheme.typography.labelLarge (14px, weight 500)
Icon size: 18x18px
Gap between groups: 2px between icon groups

Interaction:
- tap time/date area: calendar popup (post-MVP)
- tap right area: open QuickSettings
- swipe down from top edge: open QuickSettings


### 5.2 Taskbar

The bottom bar. Launches apps, shows running apps, system tray.

    +-------------------------------------------------------------+
    |  (home) (search)  | [app] [app] [app] [app] |  tray icons   |
    +-------------------------------------------------------------+

  layer: bottom
  anchor: bottom + left + right
  exclusive zone: 56px
  height: 56px
  corner radius: 0 (edge-to-edge) or 28px top corners (floating)
  background: surfaceContainer @ 0.9 opacity + blur

Sections:
1. LEFT: home button (show desktop) + search button (open AppLauncher)
2. CENTER: running app icons from wlr-foreign-toplevel
   - each icon 40x40 with 8px gap
   - dot indicator below focused app (primary color, 6x3px pill)
   - click: focus window
   - middle-click: close window
   - hover: live thumbnail (post-MVP)
3. RIGHT: system tray via StatusNotifierItem D-Bus protocol

Minimize-to-Taskbar Animation:
Custom Wayfire plugin (chi-minimize) intercepts minimize request,
queries Taskbar via IPC for target icon position, animates surface
scale + translate toward that position using slowSpatial spring.


### 5.3 QuickSettings

Slides down from top-right when tapping status icons or swiping
down from StatusBar right side.

  layer: overlay
  anchor: top + right
  exclusive zone: 0 (floating)
  keyboard: exclusive when open
  width: 412px (including 16px padding each side = 380px content)
  corner radius: 28px bottom corners
  open animation: slide down with defaultSpatial spring
  close animation: slide up with defaultEffects (no bounce)

Structure (top to bottom):

  A) EXPANDED STATUS BAR — 116px
     - large time: weight 500, 36px font, left aligned
     - carrier label: weight 500, 14px
     - date row: weight 500, 14px
     - system status row: same as StatusBar right group but expanded
       with full labels (e.g. "78% — Charging")
     - backdrop-filter: blur(500px)

  B) BRIGHTNESS SLIDER — 52px
     M3 Expressive style, full width (380px)
     - active track: primaryContainer, radius 12px left / 2px right
     - inactive track: surfaceContainerHigh, radius 2px left / 12px right
     - handle: 4px wide, 52px tall, primaryContainer, radius 8px
     - trailing icon: brightness (24x24) on inactive track right side

  C) TILE GRID — 4 rows x 2 columns, each row 72px, 8px gap
     Paginated — swipe left/right for more tiles.

     Tile types:

     WIDE TILE (186x72px, border-radius 20px):
     - leading icon: 56x56 container, 32x32 icon
       - active state: icon bg matches tile bg, 1000px radius
       - inactive state: icon in 56x56 rounded rect, primaryContainer, 16px radius
     - labels container: title + description, 4px vertical padding
       - title: weight 500, 14px (active: weight 700, 16px)
       - description: weight 400-500, 14px
     - active: primaryContainer bg, white icon, white text
     - inactive: surfaceContainerHigh bg (mapped as #F0ECF4 light), onSurface text

     ICON-ONLY TILE (89x72px):
     - active: primaryContainer bg, border-radius 20px, 32x32 white icon
     - inactive: surfaceContainerHigh bg, border-radius 1000px (pill), 32x32 onSurface icon

     ROW LAYOUT: two elements per row. Either:
     - 2 wide tiles (186 + 8 + 186 = 380px)
     - 1 wide tile + 2 icon-only tiles (186 + 8 + 89 + 8 + 89 = 380px)
     - 4 icon-only tiles (89 + 8 + 89 + 8 + 89 + 8 + 89 = 380px)

     Default tile grid page 1:
     Row 1: Internet (wide, active) + Bluetooth (wide, inactive)
     Row 2: Wallet (wide, active) + Airplane (icon) + VPN (icon, active)
     Row 3: Modes (wide, inactive) + Flashlight (icon) + Auto-rotate (icon)
     Row 4: Night Light (icon) + Song Search (icon) + QR Scanner (icon) + Screen Record (icon)

  D) PAGINATION — 40px
     - active dot: 16x6px pill, primary color
     - inactive dot: 6x6px circle, primary @ 0.38 opacity
     - edit button: 32x32 icon in circle, right side

  E) FOOTER — 64px
     - notice pill: height 40px, surfaceContainerHigh bg, radius 40px
       - leading icon 20px + label (weight 500, 14px) + trailing chevron 20px
     - settings button: 40x40 circle, surfaceContainerHigh bg, settings icon
     - power button: 40x40 circle, primaryContainer bg, white power icon


### 5.4 MediaPlayer (inside QuickSettings page 2)

Appears in QuickSettings page 2 or below tiles when media is active.

  size: 380x184px
  border-radius: 28px

  Background layers (bottom to top):
  1. album art image, stretched, offset -4px each side
  2. scrim: radial-gradient black->transparent (50% center, 191% spread)
  3. surface: color-burn blend + blur(500px) = tinted frosted glass

  Header (60px):
  - app icon: 32x32, blur(75px) backdrop
  - output chip: pill 24px height, color-burn bg, blur(75px)
    - speaker icon 16px + label 10px weight 500

  Body (124px, 16px padding, 24px gap):
  - labels row:
    - details: title (weight 500, 16px, white) + subtitle (14px, white @ 0.7)
    - play FAB: 56x40px, radius 48px, color-burn bg, play icon 24px
  - scrubber: hidden by default, shown on expand (post-MVP)

  Data source: MPRIS D-Bus (org.mpris.MediaPlayer2)
  Controls: play/pause, next, previous via MPRIS


### 5.5 NotificationCenter

Opens alongside or below QuickSettings.

  layer: overlay
  anchor: top + right (or top + left on wide screens)
  exclusive zone: 0
  width: 412px
  background: secondaryContainer @ 0.9 + blur, radius 28px top corners

Content:
- section label: "Notifications" (weight 500, 14px, onSurfaceVariant)
- grouped by app
- each notification card:
  - app icon + app name + timestamp
  - title (weight 500, 14px) + body (weight 400, 14px)
  - action buttons from notification spec
- swipe right to dismiss (fastSpatial spring, momentum from gesture)
- clear all button at bottom

D-Bus Implementation:
The shell daemon IS the notification server. It owns the
org.freedesktop.Notifications name on session bus and implements:
- GetCapabilities -> ["body", "actions", "icon-static", "persistence"]
- Notify -> displays Chi-styled notification card
- CloseNotification -> dismisses with animation
- GetServerInformation -> "Chi Shell", "chi", "0.1.0"


### 5.6 LiveInfo

A contextual status pill at the top-center of the screen. Displays
live, in-progress activity. The name says what it does: live info.

  layer: top
  anchor: top (centered horizontally)
  exclusive zone: 0 (floats, does not push content)
  height: 32px collapsed, up to 120px expanded
  corner radius: 1000px (pill) collapsed, 28px expanded
  background: surfaceContainerHighest + blur

States:
1. IDLE: hidden or minimal pill showing only clock
2. SINGLE ACTIVITY: pill shows icon + brief text
   examples: timer 02:34, recording 00:45, downloading 73%
3. TWO ACTIVITIES: pill splits into two halves
4. EXPANDED: tap to expand into a card with full controls
5. MEDIA: special layout with album art mini, title, play/pause

How activities register:
- timer/stopwatch: shell's built-in timer via D-Bus signal
- screen recording: PipeWire screencast session
- media: MPRIS D-Bus
- downloads: portal or custom D-Bus signal

Animation:
- expand: defaultSpatial spring (slight bounce)
- collapse: defaultEffects (no bounce)
- activity arrive: fastSpatial (slide/morph in)


### 5.7 AppLauncher

Full-screen overlay for launching applications.

  layer: overlay
  anchor: all edges
  exclusive zone: 0
  keyboard: exclusive (captures all input)

Layout:
- search bar at top (Chi SearchBar component, 56px height)
- app grid below, scrollable
  - 6 columns on desktop, 4 on small screens
  - each app cell: 64x64 icon + label (labelMedium, 12px) below
  - 16px gap between cells
- sorted by frecency (frequency + recency) by default
- fuzzy search filters as you type
- category chips at top for filtering (post-MVP)

App data sources (.desktop files from XDG paths):
- /usr/share/applications/
- ~/.local/share/applications/
- /var/lib/flatpak/exports/share/applications/
- ~/.local/share/flatpak/exports/share/applications/

Read fields: Name, Exec, Icon, Categories, Keywords, Comment.
Icon resolution follows XDG icon theme spec.

Animation:
- open: fade in + scale from 0.96 to 1.0 (slowSpatial)
- close: fade out + scale to 0.96 (slowEffects)


### 5.8 Desktop

The wallpaper surface with optional widgets.

  layer: background
  anchor: all edges
  exclusive zone: -1 (stretches behind everything)

MVP:
- renders wallpaper (image file, solid color, or gradient)
- basic centered clock widget

Post-MVP:
- widget grid (drag to rearrange, like mobile home screen)
- pages (swipe left/right for widget pages)
- file drop support via xdg file manager portal
- live wallpapers (shader-based or video)
- app shortcuts on desktop


## 6. Typography

From ChiTheme, M3 type scale:

  Role            Size  Weight  Usage
  ----            ----  ------  -----
  displayLarge    57px  400     clock on lock screen
  headlineSmall   24px  400     QuickSettings expanded time
  titleMedium     16px  500     tile title (active, bold uses 700)
  titleSmall      14px  500     tile title (inactive)
  bodyMedium      14px  400     tile description, notification body
  bodySmall       12px  400     timestamps, secondary info
  labelLarge      14px  500     StatusBar text, battery percentage
  labelMedium     12px  500     pagination labels, tiny labels
  labelSmall      11px  500     badges

Font family: "Roboto" (ChiTheme.fontFamily)
Icon font: "Material Icons" (ChiTheme.iconFamily)

M3 Expressive adds "emphasized" variants with bolder weight for
hero text moments (active tile titles use weight 700 instead of 500).


## 7. Shapes

M3 Expressive shape tokens used in the shell:

  Token             Radius     Usage
  -----             ------     -----
  none              0px        edge-to-edge bars
  extraSmall        4px        slider handle, pagination dots
  small             8px        small chips
  medium            12px       slider track corners (active side)
  large             16px       inactive icon container
  largeEnd          20px       tiles, active icon-only tiles
  extraLarge        28px       QuickSettings panel, MediaPlayer card
  extraLargeTop     28px top   notification panel (rounded top only)
  full              1000px     pills, StatusBar elements, icon-only inactive tiles


## 8. Accessibility

- all text meets WCAG 2.1 AA contrast (4.5:1 body, 3:1 large)
- M3 color tokens guarantee this by design
- keyboard navigation for all interactive elements
- screen reader labels on all buttons and tiles
- respect prefers-reduced-motion: swap springs for instant/linear
- large touch targets: minimum 48x48px for all interactive areas
- text resizing: support up to 200% system font scale
- high contrast mode: M3 supports Standard, Medium, High contrast


## 9. MVP Scope


### v0.1.0 — ship this first

  [ ] daemon: layer-shell surface creation + lifecycle
  [ ] StatusBar: time, date, battery icon, wifi icon
  [ ] Taskbar: running apps via foreign-toplevel, click to focus
  [ ] Taskbar: launcher button that opens AppLauncher
  [ ] AppLauncher: search + app grid from .desktop files
  [ ] QuickSettings: wifi toggle, bluetooth toggle, dark mode toggle,
      volume tile, do-not-disturb tile (6 tiles max)
  [ ] QuickSettings: BrightnessSlider
  [ ] NotificationCenter: basic notification display + swipe dismiss
  [ ] Desktop: wallpaper rendering
  [ ] dark/light mode toggle synced across entire shell


### v0.2.0

  [ ] full QuickSettings tile grid (paginated, all tiles)
  [ ] MediaPlayer card (MPRIS integration)
  [ ] LiveInfo: media now-playing pill
  [ ] notification actions + grouping by app
  [ ] minimize-to-Taskbar animation (chi-minimize Wayfire plugin)
  [ ] system tray (StatusNotifierItem D-Bus protocol)


### v0.3.0

  [ ] Desktop widgets (clock, weather, notes)
  [ ] Desktop file management (drag/drop files)
  [ ] LiveInfo: timer, screen recording, download progress
  [ ] Taskbar hover thumbnails
  [ ] lock screen
  [ ] touchpad gestures (swipe for workspaces, QuickSettings)


## 10. Build and Dependencies


### System Dependencies

- wayfire 0.8+
- qt6-base, qt6-declarative (6.5+)
- layer-shell-qt (KDE)
- wayland-client, wayland-protocols
- wlr-protocols (layer-shell, foreign-toplevel)
- dbus-1 (libdbus or Qt6DBus)
- chi (Chi UI library, installed to QT_QML_INSTALL_DIR/Chi)


### Build

    cd chi-shell
    mkdir build && cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr
    make -j$(nproc)
    sudo make install


### Run

In wayfire.ini:

    [core]
    plugins = ipc ipc-rules foreign-toplevel wayfire-shell \
              wsets scale blur animate

    [autostart]
    shell = chi-shell


## 11. Configuration

    ~/.config/chi-shell/
    |-- config.json
    |-- wallpapers/
    |-- widgets/

config.json:

    {
      "darkMode": true,
      "primaryColor": "#673AB7",
      "taskbarPosition": "bottom",
      "taskbarHeight": 56,
      "statusBarHeight": 36,
      "wallpaper": "/path/to/image.jpg",
      "widgets": []
    }


## 12. Freedesktop Standards Reference

  Standard                          Purpose
  --------                          -------
  org.freedesktop.Notifications     receive notifications from all apps
  Desktop Entry Spec (.desktop)     app launcher, menu generation
  XDG Base Directory Spec           where to find configs, data, cache
  XDG Icon Theme Spec               resolve app icons
  MPRIS D-Bus                       media player controls for LiveInfo
  StatusNotifierItem                system tray protocol
  wlr-layer-shell                   all shell surfaces
  wlr-foreign-toplevel-management   Taskbar window list
  xdg-activation-v1                 app launch focus token
  NetworkManager D-Bus              network status in QuickSettings
  BlueZ D-Bus                       bluetooth status in QuickSettings
  UPower D-Bus                      battery info in StatusBar
