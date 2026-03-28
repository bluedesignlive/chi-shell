#include "PtyManager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

namespace chiterm {

PtyManager::PtyManager(QObject *parent)
    : QObject(parent)
{
}

PtyManager::~PtyManager()
{
    close();
}

bool PtyManager::spawn(const QString &shell,
                        const QStringList &args,
                        const QString &workingDir,
                        int rows, int cols)
{
    // Set up window size
    struct winsize ws {};
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);

    pid_t pid = forkpty(&m_masterFd, nullptr, nullptr, &ws);

    if (pid < 0) {
        qWarning("forkpty failed: %s", strerror(errno));
        return false;
    }

    if (pid == 0) {
        // ── Child process ───────────────────────────────────
        if (!workingDir.isEmpty()) {
            if (chdir(workingDir.toUtf8().constData()) != 0) {
                // Fall back to HOME
                const char *home = getenv("HOME");
                if (home) chdir(home);
            }
        }

        // Set environment
        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

        // Build argv
        QByteArray shellBytes = shell.toUtf8();
        QList<QByteArray> argBytes;
        QList<char *> argv;

        argv.append(shellBytes.data());
        for (const auto &arg : args) {
            argBytes.append(arg.toUtf8());
            argv.append(argBytes.last().data());
        }
        argv.append(nullptr);

        execvp(argv[0], argv.data());

        // If exec fails, try /bin/sh
        execlp("/bin/sh", "/bin/sh", nullptr);
        _exit(127);
    }

    // ── Parent process ──────────────────────────────────────
    m_childPid = pid;
    m_running = true;

    // Set master FD to non-blocking
    int flags = fcntl(m_masterFd, F_GETFL, 0);
    fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);

    // Watch for data from the PTY
    m_readNotifier = new QSocketNotifier(m_masterFd,
                                          QSocketNotifier::Read, this);
    connect(m_readNotifier, &QSocketNotifier::activated,
            this, &PtyManager::onReadReady);

    return true;
}

void PtyManager::writeToPty(const QByteArray &data)
{
    if (m_masterFd < 0 || !m_running)
        return;

    const char *buf = data.constData();
    qint64 remaining = data.size();

    while (remaining > 0) {
        ssize_t written = ::write(m_masterFd, buf, remaining);
        if (written < 0) {
            if (errno == EAGAIN || errno == EINTR)
                continue;
            qWarning("PTY write error: %s", strerror(errno));
            return;
        }
        buf += written;
        remaining -= written;
    }
}

void PtyManager::resize(int rows, int cols)
{
    if (m_masterFd < 0)
        return;

    struct winsize ws {};
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);

    ioctl(m_masterFd, TIOCSWINSZ, &ws);
}

void PtyManager::close()
{
    if (m_readNotifier) {
        m_readNotifier->setEnabled(false);
        delete m_readNotifier;
        m_readNotifier = nullptr;
    }

    if (m_masterFd >= 0) {
        ::close(m_masterFd);
        m_masterFd = -1;
    }

    if (m_childPid > 0 && m_running) {
        kill(m_childPid, SIGHUP);
        int status = 0;
        waitpid(m_childPid, &status, WNOHANG);
        m_running = false;
        m_childPid = -1;
    }
}

void PtyManager::onReadReady()
{
    // Read in chunks
    static constexpr int READ_BUFFER_SIZE = 65536;
    char buf[READ_BUFFER_SIZE];

    while (true) {
        ssize_t n = ::read(m_masterFd, buf, READ_BUFFER_SIZE);

        if (n > 0) {
            emit dataReceived(QByteArray(buf, static_cast<int>(n)));
        } else if (n == 0) {
            // EOF — child exited
            m_running = false;
            int status = 0;
            if (waitpid(m_childPid, &status, WNOHANG) > 0) {
                int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                emit processExited(exitCode);
            } else {
                emit processExited(-1);
            }
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break; // No more data right now
            if (errno == EIO) {
                // Child exited — normal on Linux
                m_running = false;
                int status = 0;
                waitpid(m_childPid, &status, WNOHANG);
                int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                emit processExited(exitCode);
            }
            break;
        }
    }
}

} // namespace chiterm
