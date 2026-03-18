#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include "WayfireIPC.h"

#include <QJsonDocument>
#include <QProcessEnvironment>
#include <QDebug>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cerrno>

WayfireIPC::WayfireIPC(QObject *parent)
    : QObject(parent)
{
    m_socketPath = QProcessEnvironment::systemEnvironment()
                       .value("WAYFIRE_SOCKET");
}

WayfireIPC::~WayfireIPC()
{
    if (m_sockFd >= 0)
        ::close(m_sockFd);
}

bool WayfireIPC::connectToWayfire()
{
    if (m_socketPath.isEmpty()) {
        qWarning() << "WayfireIPC: WAYFIRE_SOCKET not set";
        return false;
    }

    if (m_sockFd >= 0) {
        ::close(m_sockFd);
        m_sockFd = -1;
    }

    m_sockFd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (m_sockFd < 0) {
        qWarning() << "WayfireIPC: socket() failed";
        return false;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_socketPath.toUtf8().constData(),
                 sizeof(addr.sun_path) - 1);

    if (::connect(m_sockFd, reinterpret_cast<struct sockaddr*>(&addr),
                  sizeof(addr)) < 0) {
        qWarning() << "WayfireIPC: connect() failed to" << m_socketPath;
        ::close(m_sockFd);
        m_sockFd = -1;
        return false;
    }

    qDebug() << "WayfireIPC: connected to" << m_socketPath;
    emit connectionChanged();
    return true;
}

bool WayfireIPC::isConnected() const
{
    return m_sockFd >= 0;
}

bool WayfireIPC::ensureConnected()
{
    if (m_sockFd >= 0) {
        struct pollfd pfd;
        pfd.fd = m_sockFd;
        pfd.events = POLLOUT;
        if (poll(&pfd, 1, 0) >= 0 && !(pfd.revents & (POLLERR | POLLHUP)))
            return true;
        ::close(m_sockFd);
        m_sockFd = -1;
    }
    return connectToWayfire();
}

bool WayfireIPC::sendMessage(const QByteArray &json)
{
    if (m_sockFd < 0) return false;
    uint32_t len = static_cast<uint32_t>(json.size());
    if (::write(m_sockFd, &len, sizeof(len)) != sizeof(len)) return false;
    if (::write(m_sockFd, json.constData(), len) != static_cast<ssize_t>(len)) return false;
    return true;
}

QByteArray WayfireIPC::receiveMessage()
{
    if (m_sockFd < 0) return {};

    // Safety: don't block forever if compositor hangs.
    // Local unix socket responds in <1ms normally.
    struct pollfd pfd;
    pfd.fd = m_sockFd;
    pfd.events = POLLIN;
    if (poll(&pfd, 1, 50) <= 0) return {};

    uint32_t len = 0;
    if (::read(m_sockFd, &len, sizeof(len)) != sizeof(len)) return {};
    if (len > 1024 * 1024) return {};

    QByteArray buf(static_cast<int>(len), '\0');
    ssize_t totalRead = 0;
    while (totalRead < static_cast<ssize_t>(len)) {
        ssize_t n = ::read(m_sockFd, buf.data() + totalRead, len - totalRead);
        if (n <= 0) return {};
        totalRead += n;
    }
    return buf;
}

QJsonObject WayfireIPC::call(const QString &method, const QJsonObject &data)
{
    if (!ensureConnected())
        return {{"error", "not connected"}};

    QJsonObject msg;
    msg["method"] = method;
    if (!data.isEmpty())
        msg["data"] = data;

    QByteArray json = QJsonDocument(msg).toJson(QJsonDocument::Compact);

    if (!sendMessage(json)) {
        ::close(m_sockFd); m_sockFd = -1;
        return {{"error", "send failed"}};
    }

    QByteArray response = receiveMessage();
    if (response.isEmpty()) {
        ::close(m_sockFd); m_sockFd = -1;
        return {{"error", "no response"}};
    }

    QJsonObject resp = QJsonDocument::fromJson(response).object();
    if (resp.contains("error"))
        qWarning() << "WayfireIPC:" << method << "→" << resp["error"].toString();
    return resp;
}

QJsonArray WayfireIPC::listViews()
{
    return call("window-rules/list-views").value("result").toArray();
}

QJsonObject WayfireIPC::getView(int viewId)
{
    QJsonObject d; d["id"] = viewId;
    return call("window-rules/get-view", d).value("result").toObject();
}

bool WayfireIPC::focusView(int viewId)
{
    QJsonObject d; d["id"] = viewId;
    return call("window-rules/focus-view", d).value("result").toBool();
}

bool WayfireIPC::setViewMinimized(int viewId, bool minimized)
{
    QJsonObject d; d["id"] = viewId; d["minimized"] = minimized;
    return call("window-rules/set-minimized", d).value("result").toBool();
}

bool WayfireIPC::closeView(int viewId)
{
    QJsonObject d; d["id"] = viewId;
    return call("window-rules/close-view", d).value("result").toBool();
}

bool WayfireIPC::toggleScale()
{
    return !call("scale/toggle").contains("error");
}

bool WayfireIPC::toggleExpo()
{
    return !call("expo/toggle").contains("error");
}

bool WayfireIPC::activateScaleForApp(const QString &appId, bool allWorkspaces)
{
    if (appId.isEmpty()) return false;
    QJsonObject d;
    d["app_id"] = appId;
    d["all_workspaces"] = allWorkspaces;
    return !call("scale_ipc_filter/activate_appid", d).contains("error");
}

bool WayfireIPC::showDesktop()
{
    return !call("wm-actions/toggle_showdesktop").contains("error");
}

QJsonArray WayfireIPC::listMethods()
{
    return call("list-methods").value("methods").toArray();
}



void WayfireIPC::captureViewThumbnails(const QString &appId)
{
    // Direct Wayfire IPC socket call to get view geometries
    // Protocol: 4-byte LE length prefix + JSON body
    QString socketPath = qEnvironmentVariable("WAYFIRE_SOCKET");
    if (socketPath.isEmpty()) {
        qDebug() << "captureViewThumbnails: WAYFIRE_SOCKET not set";
        return;
    }

    // Connect to Wayfire IPC
    int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return;

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.toUtf8().constData(), sizeof(addr.sun_path) - 1);

    if (::connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        ::close(fd);
        return;
    }

    // Build request
    QJsonObject msg;
    msg["method"] = QString("window-rules/list-views");
    msg["data"] = QJsonObject();
    QByteArray payload = QJsonDocument(msg).toJson(QJsonDocument::Compact);
    uint32_t len = static_cast<uint32_t>(payload.size());

    // Send: 4-byte length + JSON
    ::write(fd, &len, 4);
    ::write(fd, payload.constData(), payload.size());

    // Read response: 4-byte length + JSON
    uint32_t respLen = 0;
    ssize_t n = ::read(fd, &respLen, 4);
    if (n != 4 || respLen == 0 || respLen > 1024 * 1024) {
        ::close(fd);
        return;
    }

    QByteArray respData(respLen, '\0');
    ssize_t totalRead = 0;
    while (totalRead < (ssize_t)respLen) {
        n = ::read(fd, respData.data() + totalRead, respLen - totalRead);
        if (n <= 0) break;
        totalRead += n;
    }
    ::close(fd);

    if (totalRead < (ssize_t)respLen) return;

    // Parse view list
    QJsonDocument doc = QJsonDocument::fromJson(respData);
    QJsonArray views;
    if (doc.isArray())
        views = doc.array();
    else if (doc.isObject())
        views = doc.object().value("result").toArray();

    // Capture each matching view with grim
    int idx = 0;
    for (const auto &v : views) {
        QJsonObject vo = v.toObject();
        QString viewAppId = vo.value("app-id").toString();
        if (viewAppId != appId) continue;

        QJsonObject geo = vo.value("geometry").toObject();
        int x = geo.value("x").toInt();
        int y = geo.value("y").toInt();
        int w = geo.value("width").toInt();
        int h = geo.value("height").toInt();

        if (w <= 0 || h <= 0) { idx++; continue; }

        QString region = QString("%1,%2 %3x%4").arg(x).arg(y).arg(w).arg(h);
        QString path = QString("/tmp/chi-thumb-%1-%2.png").arg(appId).arg(idx);

        // Optional: get output name for multi-monitor
        QString output = vo.value("output-name").toString();

        QStringList args;
        if (!output.isEmpty())
            args << "-o" << output;
        args << "-g" << region << "-t" << "png" << "-l" << "0" << "-s" << "0.5" << path;

        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                proc, &QProcess::deleteLater);
        proc->start("grim", args);

        idx++;
    }

    if (idx == 0) {
        qDebug() << "captureViewThumbnails: no views found for" << appId;
    }
}
