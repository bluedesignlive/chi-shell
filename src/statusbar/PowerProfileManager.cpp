#include "PowerProfileManager.h"

#include <QDBusInterface>
#include <QDBusArgument>
#include <QDebug>

static const char *PP_SERVICE = "net.hadess.PowerProfiles";
static const char *PP_PATH    = "/net/hadess/PowerProfiles";
static const char *PP_IFACE   = "net.hadess.PowerProfiles";

PowerProfileManager::PowerProfileManager(QObject *parent)
    : QObject(parent)
{
    detect();
    if (!m_available) return;

    readProfile();

    // Listen for profile changes (e.g. from another app or gnome-settings)
    QDBusConnection::systemBus().connect(
        PP_SERVICE, PP_PATH,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        this,
        SLOT(onPropertiesChanged(QString,QVariantMap,QStringList)));
}

void PowerProfileManager::detect()
{
    QDBusInterface iface(PP_SERVICE, PP_PATH, PP_IFACE,
                         QDBusConnection::systemBus());
    m_available = iface.isValid();

    if (m_available) {
        // Read available profiles
        QVariant profilesVar = iface.property("Profiles");
        if (profilesVar.isValid()) {
            const QDBusArgument &arg = profilesVar.value<QDBusArgument>();
            arg.beginArray();
            while (!arg.atEnd()) {
                arg.beginMap();
                while (!arg.atEnd()) {
                    arg.beginMapEntry();
                    QString key;
                    QVariant val;
                    arg >> key >> val;
                    arg.endMapEntry();
                    if (key == "Profile")
                        m_profiles.append(val.toString());
                }
                arg.endMap();
            }
            arg.endArray();
        }

        // Fallback if property parsing didn't work
        if (m_profiles.isEmpty())
            m_profiles = {"power-saver", "balanced", "performance"};
    }

    qDebug() << "PowerProfiles:" << (m_available
        ? "available, profiles: " + m_profiles.join(", ")
        : "not available");
}

void PowerProfileManager::readProfile()
{
    QDBusInterface iface(PP_SERVICE, PP_PATH, PP_IFACE,
                         QDBusConnection::systemBus());
    if (!iface.isValid()) return;

    QString profile = iface.property("ActiveProfile").toString();
    if (!profile.isEmpty() && profile != m_activeProfile) {
        m_activeProfile = profile;
        emit profileChanged();
    }
}

void PowerProfileManager::onPropertiesChanged(
    const QString &iface, const QVariantMap &props, const QStringList &)
{
    if (iface != PP_IFACE) return;

    if (props.contains("ActiveProfile")) {
        QString p = props["ActiveProfile"].toString();
        if (p != m_activeProfile) {
            m_activeProfile = p;
            emit profileChanged();
        }
    }
}

void PowerProfileManager::setProfile(const QString &profile)
{
    QDBusInterface iface(PP_SERVICE, PP_PATH,
                         "org.freedesktop.DBus.Properties",
                         QDBusConnection::systemBus());
    iface.call("Set", PP_IFACE, "ActiveProfile",
               QVariant::fromValue(QDBusVariant(profile)));
    m_activeProfile = profile;
    emit profileChanged();
}

void PowerProfileManager::cycle()
{
    if (m_profiles.isEmpty()) return;

    int idx = m_profiles.indexOf(m_activeProfile);
    int next = (idx + 1) % m_profiles.size();
    setProfile(m_profiles[next]);
}

QString PowerProfileManager::icon() const
{
    if (m_activeProfile == "performance") return "bolt";
    if (m_activeProfile == "power-saver") return "eco";
    return "speed";
}

QString PowerProfileManager::label() const
{
    if (m_activeProfile == "performance") return "Performance";
    if (m_activeProfile == "power-saver") return "Power Saver";
    return "Balanced";
}
