#ifndef POWERPROFILEMANAGER_H
#define POWERPROFILEMANAGER_H

#include <QObject>
#include <QDBusConnection>

class PowerProfileManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool    available     READ available     CONSTANT)
    Q_PROPERTY(QString activeProfile READ activeProfile NOTIFY profileChanged)
    Q_PROPERTY(QString icon          READ icon          NOTIFY profileChanged)
    Q_PROPERTY(QString label         READ label         NOTIFY profileChanged)

public:
    explicit PowerProfileManager(QObject *parent = nullptr);

    bool    available()     const { return m_available; }
    QString activeProfile() const { return m_activeProfile; }
    QString icon()          const;
    QString label()         const;

    Q_INVOKABLE void cycle();
    Q_INVOKABLE void setProfile(const QString &profile);

signals:
    void profileChanged();

private slots:
    void onPropertiesChanged(const QString &iface,
                              const QVariantMap &props,
                              const QStringList &invalidated);

private:
    void detect();
    void readProfile();

    bool    m_available = false;
    QString m_activeProfile = "balanced";
    QStringList m_profiles;
};

#endif
