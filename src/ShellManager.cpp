#include "ShellManager.h"
#include "ShellSurface.h"
#include "WayfireIPC.h"
#include "WindowTracker.h"
#include "NotificationServer.h"
#include "SystemStatus.h"
#include "DesktopEntryModel.h"
#include "PinnedAppsModel.h"
#include "GroupedWindowsModel.h"
#include "XdgIconProvider.h"
#include "WindowThumbnailProvider.h"

#include <QGuiApplication>
#include <QQmlContext>
#include <QScreen>
#include <QDir>
#include <QIcon>
#include <QFileSystemWatcher>
#include <QDebug>

static const int BAR_H   = 52;
static const int POPUP_H = 500;
static const int TOTAL_H = BAR_H + POPUP_H;

ShellManager::ShellManager(QObject *parent)
    : QObject(parent)
{
}

ShellManager::~ShellManager()
{
    delete m_desktop;
    delete m_taskbar;
    delete m_statusBar;
    delete m_quickSettings;
    delete m_notificationCenter;
    delete m_appLauncher;
}

bool ShellManager::initialize()
{
    m_engine = new QQmlEngine(this);
    m_engine->addImportPath("/usr/lib64/qt6/qml");
    m_engine->addImportPath("/usr/lib/qt6/qml");
    m_engine->addImportPath(QCoreApplication::applicationDirPath() + "/../qml");

    m_engine->addImageProvider("icon", new XdgIconProvider());
    if (QIcon::themeName().isEmpty())
        QIcon::setThemeName("Adwaita");
    QIcon::setFallbackThemeName("hicolor");

    // ── Wayfire IPC ──────────────────────────────────────
    m_wayfireIPC = new WayfireIPC(this);
    if (m_wayfireIPC->connectToWayfire()) {
        QJsonArray methods = m_wayfireIPC->listMethods();
        if (!methods.isEmpty())
            qDebug() << "WayfireIPC:" << methods.size() << "methods available";
    }

    // ── Window tracker ───────────────────────────────────
    m_windowTracker = new WindowTracker(this);
    if (!m_windowTracker->init())
        qWarning() << "ShellManager: WindowTracker failed to init.";

    // ── Notifications ────────────────────────────────────
    m_notificationServer = new NotificationServer(this);
    m_notificationServer->registerOnBus();

    // ── System status ────────────────────────────────────
    m_systemStatus = new SystemStatus(this);

    // ── Desktop entries ──────────────────────────────────
    m_desktopEntries = new DesktopEntryModel(this);
    m_appFilter = new AppFilterModel(this);
    m_appFilter->setSourceModel(m_desktopEntries);

    // ── Pinned apps ──────────────────────────────────────
    m_pinnedApps = new PinnedAppsModel(this);
    m_pinnedApps->setDesktopEntryModel(m_desktopEntries);
    m_pinnedApps->setWindowTracker(m_windowTracker);

    // ── Unpinned windows (legacy, kept for compatibility) ─
    m_unpinnedWindows = new UnpinnedWindowsModel(this);
    m_unpinnedWindows->setSourceModel(m_windowTracker);
    m_unpinnedWindows->setPinnedModel(m_pinnedApps);

    connect(m_windowTracker, &QAbstractItemModel::rowsInserted,
            m_unpinnedWindows, &UnpinnedWindowsModel::scheduleRefresh);
    connect(m_windowTracker, &QAbstractItemModel::rowsRemoved,
            m_unpinnedWindows, &UnpinnedWindowsModel::scheduleRefresh);
    connect(m_windowTracker, &QAbstractItemModel::dataChanged,
            m_unpinnedWindows, &UnpinnedWindowsModel::scheduleRefresh);

    // ── Grouped windows (new, for taskbar) ───────────────
    m_groupedWindows = new GroupedWindowsModel(this);
    m_groupedWindows->setWindowTracker(m_windowTracker);
    m_groupedWindows->setPinnedModel(m_pinnedApps);

    // ── Desktop file watcher ─────────────────────────────
    auto *watcher = new QFileSystemWatcher(this);
    static const QStringList appDirs = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        QDir::homePath() + "/.local/share/applications",
        "/var/lib/flatpak/exports/share/applications",
        QDir::homePath() + "/.local/share/flatpak/exports/share/applications"
    };
    for (const auto &dir : appDirs) {
        if (QDir(dir).exists())
            watcher->addPath(dir);
    }

    auto *refreshTimer = new QTimer(this);
    refreshTimer->setSingleShot(true);
    refreshTimer->setInterval(1000);
    connect(refreshTimer, &QTimer::timeout, m_desktopEntries, &DesktopEntryModel::refresh);
    connect(watcher, &QFileSystemWatcher::directoryChanged,
            this, [refreshTimer](const QString &) { refreshTimer->start(); });

    createSurfaces();

    // Track screen geometry changes (resolution, rotation, etc.)
    connect(QGuiApplication::primaryScreen(), &QScreen::geometryChanged,
            this, [this](const QRect &geo) {
        if (geo.width() == m_screenWidth) return;
        m_screenWidth = geo.width();
        qDebug() << "ShellManager: screen width changed to" << m_screenWidth;
        if (m_taskbar) {
            m_taskbar->setSize(m_screenWidth, TOTAL_H);
            updateTaskbarInputMask();
        }
        if (m_statusBar)
            m_statusBar->setSize(m_screenWidth, 36);
    });

    qDebug() << "ShellManager: initialized";
    return true;
}

QUrl ShellManager::qmlPath(const QString &filename) const
{
    QString localPath = QCoreApplication::applicationDirPath()
                        + "/../qml/" + filename;
    if (QFile::exists(localPath))
        return QUrl::fromLocalFile(QFileInfo(localPath).absoluteFilePath());

    QString installedPath = "/usr/share/chi-shell/qml/" + filename;
    if (QFile::exists(installedPath))
        return QUrl::fromLocalFile(installedPath);

    qWarning() << "ShellManager: QML file not found:" << filename;
    return QUrl::fromLocalFile("qml/" + filename);
}

void ShellManager::registerContextProperties()
{
    auto *ctx = m_engine->rootContext();
    ctx->setContextProperty("shell",            this);
    ctx->setContextProperty("windowTracker",    m_windowTracker);
    ctx->setContextProperty("pinnedApps",       m_pinnedApps);
    ctx->setContextProperty("unpinnedWindows",  m_unpinnedWindows);
    ctx->setContextProperty("groupedWindows",   m_groupedWindows);
    ctx->setContextProperty("notifications",    m_notificationServer);
    ctx->setContextProperty("systemStatus",     m_systemStatus);
    ctx->setContextProperty("appEntries",       m_desktopEntries);
    ctx->setContextProperty("appFilter",        m_appFilter);
    ctx->setContextProperty("wayfireIPC",       m_wayfireIPC);
    ctx->setContextProperty("TASKBAR_BAR_H",    BAR_H);
    ctx->setContextProperty("TASKBAR_POPUP_H",  POPUP_H);
}

void ShellManager::updateTaskbarInputMask()
{
    if (!m_taskbar) return;

    if (m_taskbarPopupActive)
        m_taskbar->clearInputRegion();
    else
        m_taskbar->setInputRegion(QRect(0, POPUP_H, m_screenWidth, BAR_H));
}

void ShellManager::setTaskbarPopupActive(bool active)
{
    if (m_taskbarPopupActive == active) return;
    m_taskbarPopupActive = active;
    updateTaskbarInputMask();  // C++ side — QML also calls taskbarSurface directly
    emit taskbarPopupActiveChanged();
}

void ShellManager::createSurfaces()
{
    registerContextProperties();

    QScreen *screen = QGuiApplication::primaryScreen();
    m_screenWidth = screen->geometry().width();

    using A = LayerShellQt::Window::Anchor;
    using Anchors = LayerShellQt::Window::Anchors;

    // ── Desktop background ───────────────────────────────
    m_desktop = new ShellSurface(m_engine, this);
    m_desktop->setLayer(ShellSurface::Background);
    m_desktop->setAnchors(Anchors(A::AnchorTop | A::AnchorBottom | A::AnchorLeft | A::AnchorRight));
    m_desktop->setExclusiveZone(-1);
    m_desktop->setKeyboardMode(ShellSurface::KeyboardNone);
    m_desktop->setScope("chi-desktop");
    m_desktop->setSource(qmlPath("Desktop.qml"));
    m_desktop->show();

    // ── Status bar (top) ─────────────────────────────────
    m_statusBar = new ShellSurface(m_engine, this);
    m_statusBar->setLayer(ShellSurface::Top);
    m_statusBar->setAnchors(Anchors(A::AnchorTop | A::AnchorLeft | A::AnchorRight));
    m_statusBar->setExclusiveZone(36);
    m_statusBar->setKeyboardMode(ShellSurface::KeyboardNone);
    m_statusBar->setSize(m_screenWidth, 36);
    m_statusBar->setScope("chi-statusbar");
    m_statusBar->setSource(qmlPath("StatusBar.qml"));
    m_statusBar->show();

    // ── Taskbar (bottom) ─────────────────────────────────
    m_taskbar = new ShellSurface(m_engine, this);
    m_taskbar->setContextProperty("taskbarSurface", m_taskbar);
    m_taskbar->setLayer(ShellSurface::Top);
    m_taskbar->setAnchors(Anchors(A::AnchorBottom | A::AnchorLeft | A::AnchorRight));
    m_taskbar->setExclusiveZone(BAR_H);
    m_taskbar->setKeyboardMode(ShellSurface::KeyboardOnDemand);
    m_taskbar->setSize(m_screenWidth, TOTAL_H);
    m_taskbar->setScope("chi-taskbar");
    m_taskbar->setSource(qmlPath("Taskbar.qml"));
    m_taskbar->show();
    m_taskbar->setInputRegion(QRect(0, POPUP_H, m_screenWidth, BAR_H));

    // ── Quick settings overlay ───────────────────────────
    m_quickSettings = new ShellSurface(m_engine, this);
    m_quickSettings->setLayer(ShellSurface::Overlay);
    m_quickSettings->setAnchors(Anchors(A::AnchorTop | A::AnchorBottom | A::AnchorLeft | A::AnchorRight));
    m_quickSettings->setExclusiveZone(0);
    m_quickSettings->setKeyboardMode(ShellSurface::KeyboardExclusive);
    m_quickSettings->setSize(412, screen->geometry().height() - 36);
    m_quickSettings->setScope("chi-quicksettings");
    m_quickSettings->setSource(qmlPath("QuickSettings.qml"));

    // ── Notification center overlay ──────────────────────
    m_notificationCenter = new ShellSurface(m_engine, this);
    m_notificationCenter->setLayer(ShellSurface::Overlay);
    m_notificationCenter->setAnchors(Anchors(A::AnchorTop | A::AnchorBottom | A::AnchorLeft | A::AnchorRight));
    m_notificationCenter->setExclusiveZone(0);
    m_notificationCenter->setKeyboardMode(ShellSurface::KeyboardExclusive);
    m_notificationCenter->setScope("chi-notifications");
    m_notificationCenter->setSource(qmlPath("NotificationCenter.qml"));

    // ── App launcher overlay ─────────────────────────────
    m_appLauncher = new ShellSurface(m_engine, this);
    m_appLauncher->setLayer(ShellSurface::Overlay);
    m_appLauncher->setAnchors(Anchors(A::AnchorTop | A::AnchorBottom | A::AnchorLeft | A::AnchorRight));
    m_appLauncher->setExclusiveZone(0);
    m_appLauncher->setKeyboardMode(ShellSurface::KeyboardExclusive);
    m_appLauncher->setScope("chi-launcher");
    m_appLauncher->setSource(qmlPath("AppLauncher.qml"));
}

void ShellManager::setQuickSettingsOpen(bool open)
{
    if (m_quickSettingsOpen == open) return;
    m_quickSettingsOpen = open;
    if (open) {
        setNotificationCenterOpen(false);
        setAppLauncherOpen(false);
        m_quickSettings->show();
    } else {
        m_quickSettings->hide();
    }
    emit quickSettingsOpenChanged();
}

void ShellManager::setNotificationCenterOpen(bool open)
{
    if (m_notificationCenterOpen == open) return;
    m_notificationCenterOpen = open;
    if (open) {
        setQuickSettingsOpen(false);
        setAppLauncherOpen(false);
        m_notificationCenter->show();
    } else {
        m_notificationCenter->hide();
    }
    emit notificationCenterOpenChanged();
}

void ShellManager::setAppLauncherOpen(bool open)
{
    if (m_appLauncherOpen == open) return;
    m_appLauncherOpen = open;
    if (open) {
        setQuickSettingsOpen(false);
        setNotificationCenterOpen(false);
        m_appLauncher->show();
    } else {
        m_appLauncher->hide();
    }
    emit appLauncherOpenChanged();
}
