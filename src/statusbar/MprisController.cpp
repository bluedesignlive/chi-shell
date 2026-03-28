#include "MprisController.h"

#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusConnectionInterface>
#include <QDateTime>
#include <QDebug>

static const char *MPRIS_PREFIX = "org.mpris.MediaPlayer2.";
static const char *MPRIS_PATH   = "/org/mpris/MediaPlayer2";
static const char *PLAYER_IFACE = "org.mpris.MediaPlayer2.Player";

MprisController::MprisController(QObject *parent)
    : QObject(parent)
{
    // Watch for MPRIS players appearing / disappearing
    m_watcher = new QDBusServiceWatcher(this);
    m_watcher->setConnection(QDBusConnection::sessionBus());
    m_watcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration
                           | QDBusServiceWatcher::WatchForUnregistration);

    connect(m_watcher, &QDBusServiceWatcher::serviceRegistered,
            this, &MprisController::onServiceRegistered);
    connect(m_watcher, &QDBusServiceWatcher::serviceUnregistered,
            this, &MprisController::onServiceUnregistered);

    // Position timer — ticks every second while playing
    m_posTimer = new QTimer(this);
    m_posTimer->setInterval(1000);
    connect(m_posTimer, &QTimer::timeout, this, &MprisController::positionChanged);

    findPlayers();
}

// ═════════════════════════════════════════════════════
// Discovery
// ═════════════════════════════════════════════════════

void MprisController::findPlayers()
{
    auto *iface = QDBusConnection::sessionBus().interface();
    if (!iface) return;

    QDBusReply<QStringList> reply = iface->registeredServiceNames();
    if (!reply.isValid()) return;

    for (const QString &name : reply.value()) {
        if (name.startsWith(MPRIS_PREFIX)) {
            m_watcher->addWatchedService(name);
            if (!m_knownPlayers.contains(name)) {
                m_knownPlayers.append(name);
            }
        }
    }

    // Also watch for future players with prefix match
    m_watcher->addWatchedService(
        QString(MPRIS_PREFIX) + "*");

    // Pick the first available (or most recently active)
    if (!m_knownPlayers.isEmpty() && m_currentService.isEmpty()) {
        switchTo(m_knownPlayers.last());
    }
}

void MprisController::onServiceRegistered(const QString &service)
{
    if (!service.startsWith(MPRIS_PREFIX)) return;

    qDebug() << "MPRIS: player appeared:" << service;

    if (!m_knownPlayers.contains(service))
        m_knownPlayers.append(service);

    // Switch to the new player (it's probably what the user just opened)
    switchTo(service);
}

void MprisController::onServiceUnregistered(const QString &service)
{
    if (!service.startsWith(MPRIS_PREFIX)) return;

    qDebug() << "MPRIS: player gone:" << service;
    m_knownPlayers.removeAll(service);

    if (service == m_currentService) {
        disconnectFromPlayer();

        if (!m_knownPlayers.isEmpty()) {
            switchTo(m_knownPlayers.last());
        } else {
            m_active = false;
            m_title.clear();
            m_artist.clear();
            m_album.clear();
            m_artUrl.clear();
            m_playerName.clear();
            m_playing = false;
            m_posTimer->stop();
            emit changed();
        }
    }
}

// ═════════════════════════════════════════════════════
// Player connection
// ═════════════════════════════════════════════════════

void MprisController::switchTo(const QString &service)
{
    if (service == m_currentService && m_active) return;

    disconnectFromPlayer();
    m_currentService = service;
    m_playerName = busNameToAppName(service);
    connectToPlayer();
    readProperties();
}

void MprisController::connectToPlayer()
{
    QDBusConnection::sessionBus().connect(
        m_currentService, MPRIS_PATH,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        this,
        SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
}

void MprisController::disconnectFromPlayer()
{
    if (m_currentService.isEmpty()) return;

    QDBusConnection::sessionBus().disconnect(
        m_currentService, MPRIS_PATH,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        this,
        SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));

    m_currentService.clear();
}

// ═════════════════════════════════════════════════════
// Property reading
// ═════════════════════════════════════════════════════

void MprisController::readProperties()
{
    QDBusInterface player(m_currentService, MPRIS_PATH,
                          PLAYER_IFACE,
                          QDBusConnection::sessionBus());
    if (!player.isValid()) {
        m_active = false;
        emit changed();
        return;
    }

    m_active = true;

    // Playback status
    QString status = player.property("PlaybackStatus").toString();
    m_playing = (status == "Playing");

    // Capabilities
    m_canNext = player.property("CanGoNext").toBool();
    m_canPrev = player.property("CanGoPrevious").toBool();
    m_canPlay = player.property("CanPlay").toBool();

    // Position
    m_positionUs  = player.property("Position").toLongLong();
    m_posTimestamp = QDateTime::currentMSecsSinceEpoch();

    // Metadata
    QVariant metaVar = player.property("Metadata");
    if (metaVar.isValid()) {
        QVariantMap meta = qdbus_cast<QVariantMap>(metaVar);
        readMetadata(meta);
    }

    // Position timer
    if (m_playing)
        m_posTimer->start();
    else
        m_posTimer->stop();

    emit changed();
    emit positionChanged();
}

void MprisController::readMetadata(const QVariantMap &meta)
{
    m_title = meta.value("xesam:title").toString();

    QVariant artistVar = meta.value("xesam:artist");
    if (artistVar.typeId() == QMetaType::QStringList)
        m_artist = artistVar.toStringList().join(", ");
    else
        m_artist = artistVar.toString();

    m_album  = meta.value("xesam:album").toString();
    m_artUrl = meta.value("mpris:artUrl").toString();

    qint64 lengthUs = meta.value("mpris:length", 0).toLongLong();
    m_duration = lengthUs / 1000000.0;
}

// ═════════════════════════════════════════════════════
// Signal-driven updates (no polling!)
// ═════════════════════════════════════════════════════

void MprisController::onPropertiesChanged(
    const QString &iface, const QVariantMap &props, const QStringList &)
{
    if (iface != PLAYER_IFACE) return;

    bool needEmit = false;

    if (props.contains("PlaybackStatus")) {
        QString status = props["PlaybackStatus"].toString();
        bool wasPlaying = m_playing;
        m_playing = (status == "Playing");

        if (m_playing != wasPlaying) {
            if (m_playing) {
                m_posTimestamp = QDateTime::currentMSecsSinceEpoch();
                m_posTimer->start();
            } else {
                // Freeze position
                m_positionUs += (QDateTime::currentMSecsSinceEpoch()
                                 - m_posTimestamp) * 1000;
                m_posTimestamp = QDateTime::currentMSecsSinceEpoch();
                m_posTimer->stop();
            }
            needEmit = true;
        }
    }

    if (props.contains("Metadata")) {
        QVariantMap meta = qdbus_cast<QVariantMap>(
            props["Metadata"]);
        readMetadata(meta);

        // Reset position on new track
        m_positionUs  = 0;
        m_posTimestamp = QDateTime::currentMSecsSinceEpoch();
        emit positionChanged();
        needEmit = true;
    }

    if (props.contains("CanGoNext")) {
        m_canNext = props["CanGoNext"].toBool();
        needEmit = true;
    }
    if (props.contains("CanGoPrevious")) {
        m_canPrev = props["CanGoPrevious"].toBool();
        needEmit = true;
    }

    if (props.contains("Position")) {
        m_positionUs  = props["Position"].toLongLong();
        m_posTimestamp = QDateTime::currentMSecsSinceEpoch();
        emit positionChanged();
    }

    if (needEmit) emit changed();
}

// ═════════════════════════════════════════════════════
// Position (interpolated between D-Bus updates)
// ═════════════════════════════════════════════════════

double MprisController::position() const
{
    if (!m_active) return 0;

    qint64 pos = m_positionUs;
    if (m_playing) {
        qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_posTimestamp;
        pos += elapsed * 1000;  // ms → μs
    }
    return pos / 1000000.0;  // → seconds
}

// ═════════════════════════════════════════════════════
// Controls
// ═════════════════════════════════════════════════════

void MprisController::playPause()
{
    if (m_currentService.isEmpty()) return;
    QDBusInterface player(m_currentService, MPRIS_PATH,
                          PLAYER_IFACE,
                          QDBusConnection::sessionBus());
    player.call("PlayPause");
}

void MprisController::next()
{
    if (m_currentService.isEmpty()) return;
    QDBusInterface player(m_currentService, MPRIS_PATH,
                          PLAYER_IFACE,
                          QDBusConnection::sessionBus());
    player.call("Next");
}

void MprisController::previous()
{
    if (m_currentService.isEmpty()) return;
    QDBusInterface player(m_currentService, MPRIS_PATH,
                          PLAYER_IFACE,
                          QDBusConnection::sessionBus());
    player.call("Previous");
}

void MprisController::seek(double seconds)
{
    if (m_currentService.isEmpty()) return;
    QDBusInterface player(m_currentService, MPRIS_PATH,
                          PLAYER_IFACE,
                          QDBusConnection::sessionBus());

    // SetPosition needs track ID
    QDBusInterface props(m_currentService, MPRIS_PATH,
                         PLAYER_IFACE,
                         QDBusConnection::sessionBus());
    QVariantMap meta = qdbus_cast<QVariantMap>(
        props.property("Metadata"));
    QDBusObjectPath trackId = meta.value("mpris:trackid")
                                  .value<QDBusObjectPath>();

    qint64 posUs = static_cast<qint64>(seconds * 1000000);
    player.call("SetPosition", QVariant::fromValue(trackId), posUs);

    m_positionUs  = posUs;
    m_posTimestamp = QDateTime::currentMSecsSinceEpoch();
    emit positionChanged();
}

// ═════════════════════════════════════════════════════
// Helpers
// ═════════════════════════════════════════════════════

QString MprisController::busNameToAppName(const QString &busName) const
{
    // "org.mpris.MediaPlayer2.spotify" → "Spotify"
    // "org.mpris.MediaPlayer2.firefox.instance_123" → "Firefox"
    QString name = busName.mid(QString(MPRIS_PREFIX).length());

    // Strip instance suffixes
    int dot = name.indexOf('.');
    if (dot > 0) name = name.left(dot);

    // Capitalize
    if (!name.isEmpty())
        name[0] = name[0].toUpper();

    return name;
}
