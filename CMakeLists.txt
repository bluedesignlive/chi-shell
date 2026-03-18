cmake_minimum_required(VERSION 3.16)

project(chi-shell
    VERSION 0.2.0
    DESCRIPTION "Chi Shell — Desktop shell for Wayfire"
    LANGUAGES CXX C
)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(QT_NO_PRIVATE_MODULE_WARNING ON)

# ── Qt ──────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Quick GuiPrivate Qml DBus)

# ── LayerShellQt ────────────────────────────────────────
find_package(LayerShellQt REQUIRED)

# ── Wayland ─────────────────────────────────────────────
find_package(PkgConfig REQUIRED)
pkg_check_modules(WAYLAND_CLIENT REQUIRED wayland-client)
pkg_get_variable(WAYLAND_SCANNER wayland-scanner wayland_scanner)

# ── Generate wlr-foreign-toplevel protocol ──────────────
set(PROTOCOL_XML "${CMAKE_SOURCE_DIR}/protocols/wlr-foreign-toplevel-management-unstable-v1.xml")
set(PROTOCOL_HEADER "${CMAKE_BINARY_DIR}/wlr-foreign-toplevel-protocol.h")
set(PROTOCOL_CODE "${CMAKE_BINARY_DIR}/wlr-foreign-toplevel-protocol.c")

add_custom_command(
    OUTPUT ${PROTOCOL_HEADER}
    COMMAND ${WAYLAND_SCANNER} client-header ${PROTOCOL_XML} ${PROTOCOL_HEADER}
    DEPENDS ${PROTOCOL_XML}
    VERBATIM
)

add_custom_command(
    OUTPUT ${PROTOCOL_CODE}
    COMMAND ${WAYLAND_SCANNER} private-code ${PROTOCOL_XML} ${PROTOCOL_CODE}
    DEPENDS ${PROTOCOL_XML}
    VERBATIM
)

add_library(wlr_protocol STATIC ${PROTOCOL_CODE} ${PROTOCOL_HEADER})
target_include_directories(wlr_protocol PUBLIC ${CMAKE_BINARY_DIR})
target_link_libraries(wlr_protocol PUBLIC ${WAYLAND_CLIENT_LIBRARIES})

# ── chi-shell executable ────────────────────────────────
add_executable(chi-shell
    src/main.cpp
    src/ShellSurface.cpp
    src/ShellManager.cpp
    src/WindowTracker.cpp
    src/NotificationServer.cpp
    src/SystemStatus.cpp
    src/WayfireIPC.cpp
    src/DesktopEntryModel.cpp
    src/PinnedAppsModel.cpp
    src/GroupedWindowsModel.cpp
)

target_include_directories(chi-shell PRIVATE
    src
    ${CMAKE_BINARY_DIR}
    ${WAYLAND_CLIENT_INCLUDE_DIRS}
)

target_link_libraries(chi-shell PRIVATE
    Qt6::Quick
    Qt6::GuiPrivate
    Qt6::Qml
    Qt6::DBus
    LayerShellQt::Interface
    wlr_protocol
    ${WAYLAND_CLIENT_LIBRARIES}
)

# ── Install ─────────────────────────────────────────────
install(TARGETS chi-shell RUNTIME DESTINATION bin)

install(DIRECTORY qml/
    DESTINATION share/chi-shell/qml
    FILES_MATCHING PATTERN "*.qml"
)
