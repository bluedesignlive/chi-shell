#pragma once

#include <QObject>

namespace chiterm {

class PtyManager;
class VtBridge;

class TerminalSession : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
    Q_PROPERTY(int lastExitCode READ lastExitCode NOTIFY exitCodeChanged)

public:
    explicit TerminalSession(QObject *parent = nullptr);
    ~TerminalSession() override;

    // Start the session with a shell
    void start(const QString &shell = {},
               const QString &workingDir = {},
               int rows = 24, int cols = 80);

    // Accessors
    QString title() const { return m_title; }
    bool isRunning() const;
    int lastExitCode() const { return m_lastExitCode; }

    VtBridge *vtBridge() const { return m_vtBridge; }

    // Actions
    void sendKey(int key, Qt::KeyboardModifiers mods,
                 const QString &text);
    void resize(int rows, int cols);
    void close();

signals:
    void titleChanged(const QString &title);
    void contentChanged();
    void runningChanged();
    void exitCodeChanged();
    void sessionEnded(int exitCode);

private slots:
    void onPtyData(const QByteArray &data);
    void onProcessExited(int exitCode);
    void onVtTitleChanged(const QString &title);

private:
    PtyManager *m_pty      = nullptr;
    VtBridge   *m_vtBridge = nullptr;
    QString     m_title    = QStringLiteral("Terminal");
    int         m_lastExitCode = 0;
};

} // namespace chiterm
