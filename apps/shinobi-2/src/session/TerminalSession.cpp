#include "TerminalSession.h"
#include "terminal/PtyManager.h"
#include "terminal/VtBridge.h"

#include <QDir>
#include <QProcessEnvironment>

namespace chiterm {

TerminalSession::TerminalSession(QObject *parent)
    : QObject(parent)
{
}

TerminalSession::~TerminalSession()
{
    close();
}

void TerminalSession::start(const QString &shell,
                             const QString &workingDir,
                             int rows, int cols)
{
    // Determine shell
    QString shellPath = shell;
    if (shellPath.isEmpty()) {
        shellPath = qEnvironmentVariable("SHELL");
        if (shellPath.isEmpty())
            shellPath = QStringLiteral("/bin/sh");
    }

    // Determine working directory
    QString cwd = workingDir;
    if (cwd.isEmpty())
        cwd = QDir::homePath();

    // Create VT bridge
    m_vtBridge = new VtBridge(rows, cols, this);

    connect(m_vtBridge, &VtBridge::contentChanged,
            this, &TerminalSession::contentChanged);
    connect(m_vtBridge, &VtBridge::titleChanged,
            this, &TerminalSession::onVtTitleChanged);

    // Create PTY manager
    m_pty = new PtyManager(this);

    connect(m_pty, &PtyManager::dataReceived,
            this, &TerminalSession::onPtyData);
    connect(m_pty, &PtyManager::processExited,
            this, &TerminalSession::onProcessExited);

    // Spawn the shell
    if (!m_pty->spawn(shellPath, {}, cwd, rows, cols)) {
        qWarning("Failed to spawn shell: %s",
                 qPrintable(shellPath));
    }

    emit runningChanged();
}

bool TerminalSession::isRunning() const
{
    return m_pty && m_pty->isRunning();
}

void TerminalSession::sendKey(int key, Qt::KeyboardModifiers mods,
                               const QString &text)
{
    if (!m_vtBridge || !m_pty)
        return;

    QByteArray encoded = m_vtBridge->encodeKey(key, mods, text);
    if (!encoded.isEmpty())
        m_pty->writeToPty(encoded);
}

void TerminalSession::resize(int rows, int cols)
{
    if (m_vtBridge)
        m_vtBridge->resize(rows, cols);
    if (m_pty)
        m_pty->resize(rows, cols);
}

void TerminalSession::close()
{
    if (m_pty) {
        m_pty->close();
    }
}

// ─── Slots ─────────────────────────────────────────────────

void TerminalSession::onPtyData(const QByteArray &data)
{
    if (m_vtBridge)
        m_vtBridge->feed(data);
}

void TerminalSession::onProcessExited(int exitCode)
{
    m_lastExitCode = exitCode;
    emit exitCodeChanged();
    emit runningChanged();
    emit sessionEnded(exitCode);
}

void TerminalSession::onVtTitleChanged(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        emit titleChanged(m_title);
    }
}

} // namespace chiterm
