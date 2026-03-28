#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QElapsedTimer>

#include "Log.h"
#include "session/SessionManager.h"
#include "session/TerminalSession.h"
#include "rendering/TerminalWidget.h"

static void setupLogging()
{
    // Enable all shinobi.* categories at debug level
    // Override with QT_LOGGING_RULES env var for filtering, e.g.:
    //   QT_LOGGING_RULES="shinobi.pty.debug=false;shinobi.vt.debug=false"
    QLoggingCategory::setFilterRules(QStringLiteral(
        "shinobi.*.debug=true\n"
        "shinobi.*.info=true\n"
        "shinobi.*.warning=true\n"
    ));

    // Custom message format
    qSetMessagePattern(
        "%{time HH:mm:ss.zzz} "
        "%{if-debug}\033[90mDBG\033[0m%{endif}"
        "%{if-info}\033[36mINF\033[0m%{endif}"
        "%{if-warning}\033[33mWRN\033[0m%{endif}"
        "%{if-critical}\033[31mERR\033[0m%{endif}"
        "%{if-fatal}\033[31;1mFAT\033[0m%{endif}"
        " \033[90m[%{category}]\033[0m %{message}"
    );
}

int main(int argc, char *argv[])
{
    QElapsedTimer startupTimer;
    startupTimer.start();

    setupLogging();

    qCInfo(logApp) << "═══════════════════════════════════════════";
    qCInfo(logApp) << "  忍  Shinobi Terminal v0.1.0";
    qCInfo(logApp) << "═══════════════════════════════════════════";

    QGuiApplication app(argc, argv);
    app.setApplicationName("Shinobi");
    app.setOrganizationName("ChiShell");
    app.setApplicationVersion("0.1.0");

    QQuickStyle::setStyle("Basic");

    qCInfo(logApp) << "Qt" << qVersion()
                   << "| Platform:" << app.platformName();

    // ── Register types ──────────────────────────────────────
    qmlRegisterType<chiterm::TerminalWidget>("Shinobi", 1, 0, "TerminalWidget");
    qCDebug(logApp) << "QML types registered";

    QQmlApplicationEngine engine;

    // Chi QML import path
    QString chiPath = QStringLiteral(CHI_QML_PATH);
    engine.addImportPath(chiPath);
    qCInfo(logApp) << "Chi QML path:" << chiPath;

    // Session manager
    auto *sessionManager = new chiterm::SessionManager(&engine);
    engine.rootContext()->setContextProperty("SessionManager", sessionManager);

    // Load QML
    qCInfo(logApp) << "Loading QML...";
    engine.loadFromModule("Shinobi", "Main");

    if (engine.rootObjects().isEmpty()) {
        qCCritical(logApp) << "Failed to load QML — no root objects";
        return -1;
    }

    qint64 startupMs = startupTimer.elapsed();
    qCInfo(logApp) << "Startup complete in" << startupMs << "ms";

    return app.exec();
}
