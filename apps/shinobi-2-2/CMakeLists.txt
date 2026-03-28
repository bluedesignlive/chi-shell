cmake_minimum_required(VERSION 3.19)
project(shinobi VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.5)

if(COMMAND qt_policy)
    qt_policy(SET QTP0004 OLD)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(VTERM REQUIRED IMPORTED_TARGET vterm)

set(CHI_QML_PATH "${CMAKE_SOURCE_DIR}/../../../chi/qml"
    CACHE PATH "Path to Chi QML modules")

set(SHINOBI_SOURCES
    src/main.cpp
    src/Log.cpp
    src/Log.h
    src/terminal/PtyManager.cpp
    src/terminal/PtyManager.h
    src/terminal/VtBridge.cpp
    src/terminal/VtBridge.h
    src/terminal/Cell.h
    src/rendering/TerminalWidget.cpp
    src/rendering/TerminalWidget.h
    src/session/TerminalSession.cpp
    src/session/TerminalSession.h
    src/session/SessionManager.cpp
    src/session/SessionManager.h
)

set(SHINOBI_QML_FILES
    qml/Main.qml
    qml/TerminalTabBar.qml
    qml/TerminalPane.qml
    qml/TerminalStatusBar.qml
)

qt_add_executable(shinobi ${SHINOBI_SOURCES})

qt_add_qml_module(shinobi
    URI Shinobi
    VERSION 1.0
    QML_FILES ${SHINOBI_QML_FILES}
    RESOURCE_PREFIX /
    NO_GENERATE_QMLTYPES
    NO_PLUGIN
)

target_include_directories(shinobi PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(shinobi PRIVATE
    Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    PkgConfig::VTERM
)

target_compile_definitions(shinobi PRIVATE
    CHI_QML_PATH="${CHI_QML_PATH}"
)
