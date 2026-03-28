#ifndef MPRISCONTROLLER_H
#define MPRISCONTROLLER_H

#include <QObject>
#include <QDBusConnection>
#include <QDBusServiceWatcher>
#include <QTimer>

class MprisController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool    active      READ active      NOTIFY changed)
    Q_PROPERTY(QString playerName  READ playerName  NOTIFY changed)
    Q_PROPERTY(QString playerIcon  READ playerIcon  NOTIFY changed)
    Q_PROPERTY(QString title       READ title       NOTIFY changed)
    Q_PROPERTY(QString artist      READ artist      NOTIFY changed)
    Q_PROPERTY(QString album       READ album       NOTIFY changed)
    Q_PROPERTY(QString artUrl      READ artUrl      NOTIFY changed)
    Q_PROPERTY(bool    playing     READ playing     NOTIFY changed)
    Q_PROPERTY(bool    canNext     READ canNext     NOTIFY changed)
    Q_PROPERTY(bool    canPrev     READ canPrev     NOTIFY changed)
    Q_PROPERTY(bool    canPlay     READ canPlay     NOTIFY changed)
    Q_PROPERTY(double  position    READ position    NOTIFY positionChanged)
    Q_PROPERTY(double  duration    READ duration     NOTIFY changed)

public:
    explicit MprisController(QObject *parent = nullptr);

    bool    active()     const { return m_active; }
    QString playerName() const { return m_playerName; }
    QString playerIcon() const { return m_playerIcon; }
    QString title()      const { return m_title; }
    QString artist()     const { return m_artist; }
    QString album()      const { return m_album; }
    QString artUrl()     const { return m_artUrl; }
    bool    playing()    const { return m_playing; }
    bool    canNext()    const { return m_canNext; }
    bool    canPrev()    const { return m_canPrev; }
    bool    canPlay()    const { return m_canPlay; }
    double  position()   const;   // in seconds
    double  duration()   const { return m_duration; }

    Q_INVOKABLE void playPause();
    Q_INVOKABLE void next();
    Q_INVOKABLE void previous();
    Q_INVOKABLE void seek(double seconds);

signals:
    void changed();
    void positionChanged();

private slots:
    void onServiceRegistered(const QString &service);
    void onServiceUnregistered(const QString &service);
    void onPropertiesChanged(const QString &iface,
                              const QVariantMap &changed,
                              const QStringList &invalidated);

private:
    void findPlayers();
    void switchTo(const QString &service);
    void readProperties();
    void readMetadata(const QVariantMap &meta);
    void connectToPlayer();
    void disconnectFromPlayer();
    QString busNameToAppName(const QString &busName) const;

    QDBusServiceWatcher *m_watcher = nullptr;
    QTimer              *m_posTimer = nullptr;

    QStringList m_knownPlayers;
    QString     m_currentService;

    bool    m_active     = false;
    QString m_playerName;
    QString m_playerIcon;
    QString m_title;
    QString m_artist;
    QString m_album;
    QString m_artUrl;
    bool    m_playing    = false;
    bool    m_canNext    = false;
    bool    m_canPrev    = false;
    bool    m_canPlay    = true;
    double  m_duration   = 0;     // seconds
    qint64  m_positionUs = 0;     // microseconds from last read
    qint64  m_posTimestamp = 0;   // when we read it (ms since epoch)
};

#endif
