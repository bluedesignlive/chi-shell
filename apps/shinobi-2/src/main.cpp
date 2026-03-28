#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "session/SessionManager.h"
#include "session/TerminalSession.h"
#include "rendering/TerminalWidget.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("Shinobi");
    app.setOrganizationName("ChiShell");
    app.setApplicationVersion("0.1.0");

    QQuickStyle::setStyle("Basic");

    // ── Register types manually ─────────────────────────────
    qmlRegisterType<chiterm::TerminalWidget>("Shinobi", 1, 0, "TerminalWidget");

    QQmlApplicationEngine engine;

    // Add Chi QML import path
    QString chiPath = QStringLiteral(CHI_QML_PATH);
    engine.addImportPath(chiPath);

    // Register singletons via context
    auto *sessionManager = new chiterm::SessionManager(&engine);
    engine.rootContext()->setContextProperty("SessionManager", sessionManager);

    // Load main QML
    engine.loadFromModule("Shinobi", "Main");

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
