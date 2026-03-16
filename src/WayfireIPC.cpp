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
