#ifndef WAYFIREIPC_H
#define WAYFIREIPC_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>

class WayfireIPC : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ isConnected NOTIFY connectionChanged)

public:
    explicit WayfireIPC(QObject *parent = nullptr);
    ~WayfireIPC() override;

    bool connectToWayfire();
    bool isConnected() const;

    QJsonObject call(const QString &method, const QJsonObject &data = {});

    // window-rules
    QJsonArray listViews();
    QJsonObject getView(int viewId);
    bool setViewMinimized(int viewId, bool minimized);
    bool focusView(int viewId);
    bool closeView(int viewId);

    // compositor features
    Q_INVOKABLE bool toggleScale();
    Q_INVOKABLE bool toggleExpo();
    Q_INVOKABLE bool activateScaleForApp(const QString &appId,
                                          bool allWorkspaces = false);
    Q_INVOKABLE bool showDesktop();

    // diagnostic
    Q_INVOKABLE QJsonArray listMethods();

signals:
    void connectionChanged();

private:
    bool sendMessage(const QByteArray &json);
    QByteArray receiveMessage();
    bool ensureConnected();

    int m_sockFd = -1;
    QString m_socketPath;
};

#endif // WAYFIREIPC_H
