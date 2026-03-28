#pragma once

#include <QObject>
#include <QSocketNotifier>
#include <sys/types.h>

namespace chiterm {

class PtyManager : public QObject {
    Q_OBJECT

public:
    explicit PtyManager(QObject *parent = nullptr);
    ~PtyManager() override;

    bool spawn(const QString &shell,
               const QStringList &args,
               const QString &workingDir,
               int rows, int cols);

    void writeToPty(const QByteArray &data);
    void resize(int rows, int cols);
    void close();

    bool isRunning() const { return m_running; }
    pid_t childPid() const { return m_childPid; }
    int masterFd() const { return m_masterFd; }

signals:
    void dataReceived(const QByteArray &data);
    void processExited(int exitCode);

private slots:
    void onReadReady();

private:
    int     m_masterFd  = -1;
    pid_t   m_childPid  = -1;
    bool    m_running    = false;
    QSocketNotifier *m_readNotifier = nullptr;
};

} // namespace chiterm
