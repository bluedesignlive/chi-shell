#include "WindowTracker.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <cstring>

// ── Wayland listener structs ──────────────────────────────────────

static const struct wl_registry_listener registryListener = {
    .global        = WindowTracker::handleGlobalAdd,
    .global_remove = WindowTracker::handleGlobalRemove,
};

static const struct zwlr_foreign_toplevel_manager_v1_listener managerListener = {
    .toplevel = WindowTracker::handleToplevel,
    .finished = WindowTracker::handleManagerFinished,
};

static const struct zwlr_foreign_toplevel_handle_v1_listener handleListener = {
    .title        = WindowTracker::handleTitle,
    .app_id       = WindowTracker::handleAppId,
    .output_enter = WindowTracker::handleOutputEnter,
    .output_leave = WindowTracker::handleOutputLeave,
    .state        = WindowTracker::handleState,
    .done         = WindowTracker::handleDone,
    .closed       = WindowTracker::handleClosed,
    .parent       = WindowTracker::handleParent,
};

// ── Lifecycle ─────────────────────────────────────────────────────

WindowTracker::WindowTracker(QObject *parent)
    : QAbstractListModel(parent)
{
}

WindowTracker::~WindowTracker()
{
    for (auto &w : m_windows) {
        if (w.handle)
            zwlr_foreign_toplevel_handle_v1_destroy(w.handle);
    }
    if (m_manager) {
        zwlr_foreign_toplevel_manager_v1_stop(m_manager);
        zwlr_foreign_toplevel_manager_v1_destroy(m_manager);
        m_manager = nullptr;
    }
    if (m_seat) {
        wl_seat_destroy(m_seat);
        m_seat = nullptr;
    }
    if (m_registry) {
        wl_registry_destroy(m_registry);
        m_registry = nullptr;
    }
    // Note: m_display is owned by Qt — do NOT destroy it
}

bool WindowTracker::init()
{
    auto *native = QGuiApplication::platformNativeInterface();
    if (!native) {
        qWarning() << "WindowTracker: no platform native interface";
        return false;
    }

    m_display = static_cast<wl_display*>(
        native->nativeResourceForIntegration("wl_display"));
    if (!m_display) {
        qWarning() << "WindowTracker: could not get wl_display";
        return false;
    }

    m_registry = wl_display_get_registry(m_display);
    wl_registry_add_listener(m_registry, &registryListener, this);
    wl_display_roundtrip(m_display);

    if (!m_manager) {
        qWarning() << "WindowTracker: compositor does not support"
                    << "wlr-foreign-toplevel-management";
        return false;
    }
    if (!m_seat)
        qWarning() << "WindowTracker: no wl_seat found";

    wl_display_roundtrip(m_display);
    qDebug() << "WindowTracker: initialized with" << m_windows.size() << "windows";
    return true;
}

// ── QAbstractListModel ───────────────────────────────────────────

int WindowTracker::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_windows.size();
}

QVariant WindowTracker::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_windows.size())
        return {};
    const auto &w = m_windows.at(index.row());
    switch (role) {
    case AppIdRole:      return w.appId;
    case TitleRole:      return w.title;
    case ActivatedRole:  return w.activated;
    case MinimizedRole:  return w.minimized;
    case MaximizedRole:  return w.maximized;
    case FullscreenRole: return w.fullscreen;
    case IconNameRole:   return w.iconName;
    }
    return {};
}

QHash<int, QByteArray> WindowTracker::roleNames() const
{
    return {
        { AppIdRole,      "appId" },
        { TitleRole,      "title" },
        { ActivatedRole,  "activated" },
        { MinimizedRole,  "minimized" },
        { MaximizedRole,  "maximized" },
        { FullscreenRole, "fullscreen" },
        { IconNameRole,   "iconName" },
    };
}

// ── Actions ──────────────────────────────────────────────────────

void WindowTracker::activate(int index)
{
    if (index < 0 || index >= m_windows.size() || !m_seat) return;
    zwlr_foreign_toplevel_handle_v1_activate(m_windows[index].handle, m_seat);
}

void WindowTracker::close(int index)
{
    if (index < 0 || index >= m_windows.size()) return;
    zwlr_foreign_toplevel_handle_v1_close(m_windows[index].handle);
}

void WindowTracker::setMinimized(int index, bool minimized)
{
    if (index < 0 || index >= m_windows.size()) return;
    if (minimized)
        zwlr_foreign_toplevel_handle_v1_set_minimized(m_windows[index].handle);
    else
        zwlr_foreign_toplevel_handle_v1_unset_minimized(m_windows[index].handle);
}

void WindowTracker::setMaximized(int index, bool maximized)
{
    if (index < 0 || index >= m_windows.size()) return;
    if (maximized)
        zwlr_foreign_toplevel_handle_v1_set_maximized(m_windows[index].handle);
    else
        zwlr_foreign_toplevel_handle_v1_unset_maximized(m_windows[index].handle);
}

void WindowTracker::setFullscreen(int index, bool fullscreen)
{
    if (index < 0 || index >= m_windows.size()) return;
    if (fullscreen)
        zwlr_foreign_toplevel_handle_v1_set_fullscreen(m_windows[index].handle, nullptr);
    else
        zwlr_foreign_toplevel_handle_v1_unset_fullscreen(m_windows[index].handle);
}

void WindowTracker::toggleMinimize(int index)
{
    if (index < 0 || index >= m_windows.size()) return;
    const auto &w = m_windows[index];
    if (w.activated && !w.minimized) {
        zwlr_foreign_toplevel_handle_v1_set_minimized(w.handle);
    } else if (w.minimized) {
        zwlr_foreign_toplevel_handle_v1_unset_minimized(w.handle);
        if (m_seat) zwlr_foreign_toplevel_handle_v1_activate(w.handle, m_seat);
    } else {
        if (m_seat) zwlr_foreign_toplevel_handle_v1_activate(w.handle, m_seat);
    }
}

void WindowTracker::closeAllForApp(const QString &appId)
{
    for (int i = m_windows.size() - 1; i >= 0; --i) {
        if (m_windows[i].appId == appId)
            zwlr_foreign_toplevel_handle_v1_close(m_windows[i].handle);
    }
}

int WindowTracker::firstIndexForApp(const QString &appId) const
{
    for (int i = 0; i < m_windows.size(); ++i)
        if (m_windows[i].appId == appId) return i;
    return -1;
}

int WindowTracker::windowCountForApp(const QString &appId) const
{
    int c = 0;
    for (const auto &w : m_windows)
        if (w.appId == appId) ++c;
    return c;
}

bool WindowTracker::isMaximizedAt(int index) const
{
    if (index < 0 || index >= m_windows.size()) return false;
    return m_windows[index].maximized;
}

bool WindowTracker::isMinimizedAt(int index) const
{
    if (index < 0 || index >= m_windows.size()) return false;
    return m_windows[index].minimized;
}

bool WindowTracker::isFullscreenAt(int index) const
{
    if (index < 0 || index >= m_windows.size()) return false;
    return m_windows[index].fullscreen;
}

bool WindowTracker::isActivatedAt(int index) const
{
    if (index < 0 || index >= m_windows.size()) return false;
    return m_windows[index].activated;
}

QString WindowTracker::titleAt(int index) const
{
    if (index < 0 || index >= m_windows.size()) return {};
    return m_windows[index].title;
}

// ── Wayland callbacks ────────────────────────────────────────────

void WindowTracker::handleGlobalAdd(void *data, struct wl_registry *reg,
                                     uint32_t name, const char *iface, uint32_t ver)
{
    auto self = static_cast<WindowTracker*>(data);
    if (strcmp(iface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
        self->m_manager = static_cast<zwlr_foreign_toplevel_manager_v1*>(
            wl_registry_bind(reg, name,
                             &zwlr_foreign_toplevel_manager_v1_interface,
                             qMin(ver, 3u)));
        zwlr_foreign_toplevel_manager_v1_add_listener(
            self->m_manager, &managerListener, self);
    } else if (strcmp(iface, "wl_seat") == 0 && !self->m_seat) {
        self->m_seat = static_cast<wl_seat*>(
            wl_registry_bind(reg, name, &wl_seat_interface, 1));
    }
}

void WindowTracker::handleGlobalRemove(void *, struct wl_registry *, uint32_t)
{ /* TODO: handle seat / manager removal if needed */ }

void WindowTracker::handleToplevel(void *data,
                                    zwlr_foreign_toplevel_manager_v1 *,
                                    zwlr_foreign_toplevel_handle_v1 *handle)
{
    auto self = static_cast<WindowTracker*>(data);
    ToplevelWindow win;
    win.handle = handle;
    self->beginInsertRows({}, self->m_windows.size(), self->m_windows.size());
    self->m_windows.append(win);
    self->endInsertRows();
    emit self->countChanged();
    zwlr_foreign_toplevel_handle_v1_add_listener(handle, &handleListener, self);
}

void WindowTracker::handleManagerFinished(void *data, zwlr_foreign_toplevel_manager_v1 *mgr)
{
    auto self = static_cast<WindowTracker*>(data);
    zwlr_foreign_toplevel_manager_v1_destroy(mgr);
    if (self->m_manager == mgr)
        self->m_manager = nullptr;
}

void WindowTracker::handleTitle(void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *title)
{
    auto self = static_cast<WindowTracker*>(data);
    auto *win = self->windowForHandle(handle);
    if (win) win->pendingTitle = QString::fromUtf8(title);
}

void WindowTracker::handleAppId(void *data, zwlr_foreign_toplevel_handle_v1 *handle, const char *app_id)
{
    auto self = static_cast<WindowTracker*>(data);
    auto *win = self->windowForHandle(handle);
    if (win) win->pendingAppId = QString::fromUtf8(app_id);
}

void WindowTracker::handleState(void *data, zwlr_foreign_toplevel_handle_v1 *handle, struct wl_array *state)
{
    auto self = static_cast<WindowTracker*>(data);
    auto *win = self->windowForHandle(handle);
    if (!win) return;
    win->pendingActivated  = false;
    win->pendingMinimized  = false;
    win->pendingMaximized  = false;
    win->pendingFullscreen = false;
    // wl_array iteration: packed array of uint32_t values
    auto *begin = static_cast<uint32_t*>(state->data);
    auto *end   = reinterpret_cast<uint32_t*>(
                      static_cast<char*>(state->data) + state->size);
    for (auto *e = begin; e < end; ++e) {
        switch (*e) {
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED:  win->pendingActivated  = true; break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED:  win->pendingMinimized  = true; break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED:  win->pendingMaximized  = true; break;
        case ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN: win->pendingFullscreen = true; break;
        }
    }
}

void WindowTracker::handleDone(void *data, zwlr_foreign_toplevel_handle_v1 *handle)
{
    auto self = static_cast<WindowTracker*>(data);
    int idx = self->indexForHandle(handle);
    if (idx < 0) return;
    auto &win = self->m_windows[idx];

    win.title      = win.pendingTitle;
    win.activated  = win.pendingActivated;
    win.minimized  = win.pendingMinimized;
    win.maximized  = win.pendingMaximized;
    win.fullscreen = win.pendingFullscreen;

    // Only re-resolve icon when appId actually changes
    if (win.appId != win.pendingAppId) {
        win.appId = win.pendingAppId;
        win.iconName = self->resolveIconName(win.appId);
    } else if (win.iconName.isEmpty()) {
        win.iconName = self->resolveIconName(win.appId);
    }

    emit self->dataChanged(self->index(idx), self->index(idx));
}

void WindowTracker::handleClosed(void *data, zwlr_foreign_toplevel_handle_v1 *handle)
{
    auto self = static_cast<WindowTracker*>(data);
    int idx = self->indexForHandle(handle);
    if (idx < 0) return;
    self->beginRemoveRows({}, idx, idx);
    zwlr_foreign_toplevel_handle_v1_destroy(handle);
    self->m_windows.removeAt(idx);
    self->endRemoveRows();
    emit self->countChanged();
}

void WindowTracker::handleOutputEnter(void *, zwlr_foreign_toplevel_handle_v1 *, struct wl_output *) {}
void WindowTracker::handleOutputLeave(void *, zwlr_foreign_toplevel_handle_v1 *, struct wl_output *) {}
void WindowTracker::handleParent(void *, zwlr_foreign_toplevel_handle_v1 *, zwlr_foreign_toplevel_handle_v1 *) {}

// ── Private helpers ──────────────────────────────────────────────

int WindowTracker::indexForHandle(zwlr_foreign_toplevel_handle_v1 *handle) const
{
    for (int i = 0; i < m_windows.size(); ++i)
        if (m_windows[i].handle == handle) return i;
    return -1;
}

ToplevelWindow *WindowTracker::windowForHandle(zwlr_foreign_toplevel_handle_v1 *handle)
{
    int idx = indexForHandle(handle);
    return (idx >= 0) ? &m_windows[idx] : nullptr;
}

QString WindowTracker::resolveIconName(const QString &appId) const
{
    if (appId.isEmpty()) return appId;

    // Check cache first
    auto it = m_iconCache.constFind(appId);
    if (it != m_iconCache.constEnd())
        return it.value();

    static const QStringList paths = {
        "/usr/share/applications",
        QDir::homePath() + "/.local/share/applications",
        "/var/lib/flatpak/exports/share/applications",
        QDir::homePath() + "/.local/share/flatpak/exports/share/applications"
    };

    auto extractIcon = [](const QString &filePath) -> QString {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return {};
        QTextStream in(&file);
        bool inEntry = false;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line == "[Desktop Entry]") { inEntry = true; continue; }
            if (line.startsWith(QLatin1Char('['))) { inEntry = false; continue; }
            if (inEntry && line.startsWith("Icon="))
                return line.mid(5).trimmed();
        }
        return {};
    };

    // Exact match: <appId>.desktop
    for (const auto &dir : paths) {
        QString filePath = dir + "/" + appId + ".desktop";
        if (QFile::exists(filePath)) {
            QString icon = extractIcon(filePath);
            if (!icon.isEmpty()) {
                m_iconCache.insert(appId, icon);
                return icon;
            }
        }
    }

    // Fuzzy match: filename contains appId (case-insensitive)
    for (const auto &dir : paths) {
        QDir d(dir);
        if (!d.exists()) continue;
        const auto files = d.entryList({"*.desktop"}, QDir::Files);
        for (const auto &f : files) {
            if (f.toLower().contains(appId.toLower())) {
                QString icon = extractIcon(d.filePath(f));
                if (!icon.isEmpty()) {
                    m_iconCache.insert(appId, icon);
                    return icon;
                }
            }
        }
    }

    m_iconCache.insert(appId, appId);
    return appId;
}
