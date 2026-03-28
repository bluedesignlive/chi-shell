#include "TerminalSession.h"
#include "terminal/PtyManager.h"
#include "terminal/VtBridge.h"
#include "Log.h"

#include <QDir>

namespace chiterm {

TerminalSession::TerminalSession(QObject *parent)
    : QObject(parent)
{
    qCDebug(logSession) << "TerminalSession created";
}

TerminalSession::~TerminalSession()
{
    close();
    qCDebug(logSession) << "TerminalSession destroyed";
}

void TerminalSession::start(const QString &shell,
                             const QString &workingDir,
                             int rows, int cols)
{
    // Use safe defaults if widget hasn't sized yet
    if (rows <= 0) rows = 24;
    if (cols <= 0) cols = 80;

    m_rows = rows;
    m_cols = cols;

    QString shellPath = shell;
    if (shellPath.isEmpty()) {
        shellPath = qEnvironmentVariable("SHELL");
        if (shellPath.isEmpty())
            shellPath = QStringLiteral("/bin/sh");
    }

    QString cwd = workingDir;
    if (cwd.isEmpty())
        cwd = QDir::homePath();

    qCInfo(logSession) << "Starting session — shell:" << shellPath
                       << "cwd:" << cwd << "grid:" << m_cols << "x" << m_rows;

    // Create VT bridge at the ACTUAL size
    m_vtBridge = new VtBridge(m_rows, m_cols, this);
    connect(m_vtBridge, &VtBridge::contentChanged,
            this, &TerminalSession::contentChanged);
    connect(m_vtBridge, &VtBridge::titleChanged,
            this, &TerminalSession::onVtTitleChanged);

    // Create PTY at the SAME size
    m_pty = new PtyManager(this);
    connect(m_pty, &PtyManager::dataReceived,
            this, &TerminalSession::onPtyData);
    connect(m_pty, &PtyManager::processExited,
            this, &TerminalSession::onProcessExited);

    if (!m_pty->spawn(shellPath, {}, cwd, m_rows, m_cols))
        qCCritical(logSession) << "Failed to spawn shell:" << shellPath;

    m_started = true;
    emit runningChanged();
}

bool TerminalSession::isRunning() const
{
    return m_pty && m_pty->isRunning();
}

void TerminalSession::sendKey(int key, Qt::KeyboardModifiers mods,
                               const QString &text)
{
    if (!m_vtBridge || !m_pty) return;

    QByteArray encoded = m_vtBridge->encodeKey(key, mods, text);
    if (!encoded.isEmpty())
        m_pty->writeToPty(encoded);
}

void TerminalSession::resize(int rows, int cols)
{
    if (rows <= 0 || cols <= 0) return;
    if (rows == m_rows && cols == m_cols) return;

    m_rows = rows;
    m_cols = cols;

    // CRITICAL: resize VT and PTY together, VT first
    if (m_vtBridge) m_vtBridge->resize(rows, cols);
    if (m_pty && m_pty->isRunning()) m_pty->resize(rows, cols);
}

void TerminalSession::close()
{
    if (m_pty) m_pty->close();
}

void TerminalSession::onPtyData(const QByteArray &data)
{
    if (m_vtBridge) m_vtBridge->feed(data);
}

void TerminalSession::onProcessExited(int exitCode)
{
    qCInfo(logSession) << "Process exited — code:" << exitCode;
    m_lastExitCode = exitCode;
    emit exitCodeChanged();
    emit runningChanged();
    emit sessionEnded(exitCode);
}

void TerminalSession::onVtTitleChanged(const QString &title)
{
    if (m_title != title) {
        m_title = title;
        qCDebug(logSession) << "Title →" << m_title;
        emit titleChanged(m_title);
    }
}

} // namespace chiterm
