#include "PtyManager.h"
#include "Log.h"

#include <QCoreApplication>
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
    qCInfo(logPty) << "Spawning shell:" << shell
                   << "cwd:" << workingDir
                   << "grid:" << cols << "x" << rows;

    struct winsize ws {};
    ws.ws_row = static_cast<unsigned short>(rows);
    ws.ws_col = static_cast<unsigned short>(cols);

    pid_t pid = forkpty(&m_masterFd, nullptr, nullptr, &ws);

    if (pid < 0) {
        qCCritical(logPty) << "forkpty failed:" << strerror(errno);
        return false;
    }

    if (pid == 0) {
        // ── Child process ───────────────────────────────────
        if (!workingDir.isEmpty()) {
            if (chdir(workingDir.toUtf8().constData()) != 0) {
                const char *home = getenv("HOME");
                if (home) chdir(home);
            }
        }

        setenv("TERM", "xterm-256color", 1);
        setenv("COLORTERM", "truecolor", 1);

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
        execlp("/bin/sh", "/bin/sh", nullptr);
        _exit(127);
    }

    // ── Parent process ──────────────────────────────────────
    m_childPid = pid;
    m_running = true;

    qCInfo(logPty) << "Shell spawned — pid:" << m_childPid
                   << "masterFd:" << m_masterFd;

    int flags = fcntl(m_masterFd, F_GETFL, 0);
    fcntl(m_masterFd, F_SETFL, flags | O_NONBLOCK);

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
            qCWarning(logPty) << "Write error:" << strerror(errno);
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
    qCDebug(logPty) << "Resized to" << cols << "x" << rows;
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
        qCInfo(logPty) << "Closing PTY — killing pid:" << m_childPid;
        kill(m_childPid, SIGHUP);
        int status = 0;
        waitpid(m_childPid, &status, WNOHANG);
        m_running = false;
        m_childPid = -1;
    }
}

void PtyManager::onReadReady()
{
    static constexpr int READ_BUFFER_SIZE = 65536;
    char buf[READ_BUFFER_SIZE];

    while (true) {
        ssize_t n = ::read(m_masterFd, buf, READ_BUFFER_SIZE);

        if (n > 0) {
            qCDebug(logPty).nospace() << "Read " << n << " bytes from PTY";
            emit dataReceived(QByteArray(buf, static_cast<int>(n)));
        } else if (n == 0) {
            m_running = false;
            int status = 0;
            int exitCode = -1;
            if (waitpid(m_childPid, &status, WNOHANG) > 0)
                exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            qCInfo(logPty) << "PTY EOF — process exited with code:" << exitCode;
            emit processExited(exitCode);
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            if (errno == EIO) {
                m_running = false;
                int status = 0;
                waitpid(m_childPid, &status, WNOHANG);
                int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
                qCInfo(logPty) << "PTY EIO — process exited with code:" << exitCode;
                emit processExited(exitCode);
            }
            break;
        }
    }
}

} // namespace chiterm
