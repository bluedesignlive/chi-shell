#include <QGuiApplication>
#include <QDebug>

#include "ShellManager.h"

// ═══════════════════════════════════════════════════════
// Chi Shell — entry point
//
// LayerShellQt::Shell::useLayerShell() is deprecated since
// Qt 6.5 — layer-shell integration activates automatically
// when the LayerShellQt library is linked. We just need to
// configure each window BEFORE showing it.
// ═══════════════════════════════════════════════════════

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("chi-shell");
    app.setApplicationVersion("0.1.0");
    app.setOrganizationName("chi");
    app.setDesktopFileName("chi-shell");

    qDebug() << "Chi Shell v0.1.0 starting...";

    // verify Wayland
    if (app.platformName() != "wayland") {
        qCritical() << "Chi Shell requires Wayland. Current platform:"
                     << app.platformName();
        return 1;
    }

    ShellManager manager;
    if (!manager.initialize()) {
        qCritical() << "Chi Shell: failed to initialize";
        return 1;
    }

    qDebug() << "Chi Shell: running";
    return app.exec();
}
