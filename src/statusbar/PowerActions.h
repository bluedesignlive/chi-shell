#ifndef POWERACTIONS_H
#define POWERACTIONS_H

#include <QObject>
#include <QProcess>

class PowerActions : public QObject
{
    Q_OBJECT

public:
    explicit PowerActions(QObject *parent = nullptr) : QObject(parent) {}

    Q_INVOKABLE void shutdown() {
        QProcess::startDetached("systemctl", {"poweroff"});
    }

    Q_INVOKABLE void reboot() {
        QProcess::startDetached("systemctl", {"reboot"});
    }

    Q_INVOKABLE void suspend() {
        QProcess::startDetached("systemctl", {"suspend"});
    }

    Q_INVOKABLE void lockScreen() {
        QProcess::startDetached("swaylock", {});
    }

    Q_INVOKABLE void logout() {
        QProcess::startDetached("loginctl", {"terminate-session", "self"});
    }
};

// Header-only Q_OBJECT needs this

#endif
