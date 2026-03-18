#ifndef SHELLMANAGER_H
#define SHELLMANAGER_H

#include <QObject>
#include <QQmlEngine>

class ShellSurface;
class WayfireIPC;
class WindowTracker;
class NotificationServer;
class SystemStatus;
class DesktopEntryModel;
class AppFilterModel;
class PinnedAppsModel;
class UnpinnedWindowsModel;
class GroupedWindowsModel;

class ShellManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool quickSettingsOpen READ quickSettingsOpen
               WRITE setQuickSettingsOpen NOTIFY quickSettingsOpenChanged)
    Q_PROPERTY(bool notificationCenterOpen READ notificationCenterOpen
               WRITE setNotificationCenterOpen NOTIFY notificationCenterOpenChanged)
    Q_PROPERTY(bool appLauncherOpen READ appLauncherOpen
               WRITE setAppLauncherOpen NOTIFY appLauncherOpenChanged)
    Q_PROPERTY(bool taskbarPopupActive READ taskbarPopupActive
               WRITE setTaskbarPopupActive NOTIFY taskbarPopupActiveChanged)

public:
    explicit ShellManager(QObject *parent = nullptr);
    ~ShellManager() override;

    bool initialize();

    bool quickSettingsOpen() const { return m_quickSettingsOpen; }
    void setQuickSettingsOpen(bool open);
    bool notificationCenterOpen() const { return m_notificationCenterOpen; }
    void setNotificationCenterOpen(bool open);
    bool appLauncherOpen() const { return m_appLauncherOpen; }
    void setAppLauncherOpen(bool open);

    bool taskbarPopupActive() const { return m_taskbarPopupActive; }
    void setTaskbarPopupActive(bool active);

signals:
    void quickSettingsOpenChanged();
    void notificationCenterOpenChanged();
    void appLauncherOpenChanged();
    void taskbarPopupActiveChanged();

private:
    void createSurfaces();
    void registerContextProperties();
    void updateTaskbarInputMask();
    QUrl qmlPath(const QString &filename) const;

    QQmlEngine *m_engine = nullptr;

    WayfireIPC            *m_wayfireIPC         = nullptr;
    WindowTracker         *m_windowTracker      = nullptr;
    NotificationServer    *m_notificationServer = nullptr;
    SystemStatus          *m_systemStatus       = nullptr;
    DesktopEntryModel     *m_desktopEntries     = nullptr;
    AppFilterModel        *m_appFilter          = nullptr;
    PinnedAppsModel       *m_pinnedApps         = nullptr;
    UnpinnedWindowsModel  *m_unpinnedWindows    = nullptr;
    GroupedWindowsModel   *m_groupedWindows     = nullptr;

    ShellSurface *m_desktop            = nullptr;
    ShellSurface *m_taskbar            = nullptr;
    ShellSurface *m_statusBar          = nullptr;
    ShellSurface *m_quickSettings      = nullptr;
    ShellSurface *m_notificationCenter = nullptr;
    ShellSurface *m_appLauncher        = nullptr;

    bool m_quickSettingsOpen      = false;
    bool m_notificationCenterOpen = false;
    bool m_appLauncherOpen        = false;
    bool m_taskbarPopupActive     = false;
    int  m_screenWidth            = 1920;
};

#endif // SHELLMANAGER_H
