#ifndef WINDOWTRACKER_H
#define WINDOWTRACKER_H

#include <QObject>
#include <QAbstractListModel>
#include <QGuiApplication>
#include <QHash>

#include <qpa/qplatformnativeinterface.h>
#include <wayland-client.h>
#include "wlr-foreign-toplevel-protocol.h"

struct ToplevelWindow
{
    zwlr_foreign_toplevel_handle_v1 *handle = nullptr;
    QString appId;
    QString title;
    bool activated  = false;
    bool minimized  = false;
    bool maximized  = false;
    bool fullscreen = false;
    QString iconName;

    // Pending state (committed on "done")
    QString pendingTitle;
    QString pendingAppId;
    bool pendingActivated  = false;
    bool pendingMinimized  = false;
    bool pendingMaximized  = false;
    bool pendingFullscreen = false;
};

class WindowTracker : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        TitleRole,
        ActivatedRole,
        MinimizedRole,
        MaximizedRole,
        FullscreenRole,
        IconNameRole
    };

    explicit WindowTracker(QObject *parent = nullptr);
    ~WindowTracker() override;

    bool init();

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_windows.size(); }

    Q_INVOKABLE void activate(int index);
    Q_INVOKABLE void close(int index);
    Q_INVOKABLE void toggleMinimize(int index);
    Q_INVOKABLE void setMinimized(int index, bool minimized);
    Q_INVOKABLE void setMaximized(int index, bool maximized);
    Q_INVOKABLE void setFullscreen(int index, bool fullscreen);

    Q_INVOKABLE void closeAllForApp(const QString &appId);
    Q_INVOKABLE int  firstIndexForApp(const QString &appId) const;
    Q_INVOKABLE int  windowCountForApp(const QString &appId) const;
    Q_INVOKABLE bool isMaximizedAt(int index) const;
    Q_INVOKABLE bool isMinimizedAt(int index) const;
    Q_INVOKABLE bool isFullscreenAt(int index) const;
    Q_INVOKABLE bool isActivatedAt(int index) const;
    Q_INVOKABLE QString titleAt(int index) const;

    // Wayland listeners (static C callbacks)
    static void handleGlobalAdd(void *data, struct wl_registry *reg,
                                uint32_t name, const char *iface, uint32_t ver);
    static void handleGlobalRemove(void *data, struct wl_registry *reg,
                                   uint32_t name);
    static void handleToplevel(void *data,
                               zwlr_foreign_toplevel_manager_v1 *mgr,
                               zwlr_foreign_toplevel_handle_v1 *handle);
    static void handleManagerFinished(void *data,
                                      zwlr_foreign_toplevel_manager_v1 *mgr);
    static void handleTitle(void *data,
                            zwlr_foreign_toplevel_handle_v1 *handle,
                            const char *title);
    static void handleAppId(void *data,
                            zwlr_foreign_toplevel_handle_v1 *handle,
                            const char *app_id);
    static void handleState(void *data,
                            zwlr_foreign_toplevel_handle_v1 *handle,
                            struct wl_array *state);
    static void handleDone(void *data,
                           zwlr_foreign_toplevel_handle_v1 *handle);
    static void handleClosed(void *data,
                             zwlr_foreign_toplevel_handle_v1 *handle);
    static void handleOutputEnter(void *data,
                                  zwlr_foreign_toplevel_handle_v1 *handle,
                                  struct wl_output *output);
    static void handleOutputLeave(void *data,
                                  zwlr_foreign_toplevel_handle_v1 *handle,
                                  struct wl_output *output);
    static void handleParent(void *data,
                             zwlr_foreign_toplevel_handle_v1 *handle,
                             zwlr_foreign_toplevel_handle_v1 *parent);

signals:
    void countChanged();

private:
    int indexForHandle(zwlr_foreign_toplevel_handle_v1 *handle) const;
    ToplevelWindow *windowForHandle(zwlr_foreign_toplevel_handle_v1 *handle);
    QString resolveIconName(const QString &appId) const;

    wl_display  *m_display  = nullptr;
    wl_registry *m_registry = nullptr;
    wl_seat     *m_seat     = nullptr;
    zwlr_foreign_toplevel_manager_v1 *m_manager = nullptr;

    QVector<ToplevelWindow> m_windows;

    // Icon resolution cache: appId → iconName
    mutable QHash<QString, QString> m_iconCache;
};

#endif // WINDOWTRACKER_H
