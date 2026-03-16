#include "PinnedAppsModel.h"
#include "WindowTracker.h"
#include "DesktopEntryModel.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QProcess>
#include <QStandardPaths>
#include <QRegularExpression>
#include <QDebug>

// ═════════════════════════════════════════════════════════
// PinnedAppsModel
// ═════════════════════════════════════════════════════════

PinnedAppsModel::PinnedAppsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(500);
    connect(&m_saveTimer, &QTimer::timeout, this, &PinnedAppsModel::save);

    // Coalesce sync: 0ms timer fires once per event loop iteration
    m_syncTimer.setSingleShot(true);
    m_syncTimer.setInterval(0);
    connect(&m_syncTimer, &QTimer::timeout, this, &PinnedAppsModel::syncRunningState);
}

int PinnedAppsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_pinned.size();
}

QVariant PinnedAppsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_pinned.size())
        return {};

    const auto &p = m_pinned.at(index.row());
    switch (role) {
    case AppIdRole:       return p.appId;
    case NameRole:        return p.name;
    case IconNameRole:    return p.iconName;
    case DesktopFileRole: return p.desktopFile;
    case ExecRole:        return p.exec;
    case IsRunningRole:   return p.isRunning;
    case WindowCountRole: return p.windowCount;
    }
    return {};
}

QHash<int, QByteArray> PinnedAppsModel::roleNames() const
{
    return {
        { AppIdRole,       "appId"       },
        { NameRole,        "name"        },
        { IconNameRole,    "iconName"    },
        { DesktopFileRole, "desktopFile" },
        { ExecRole,        "exec"        },
        { IsRunningRole,   "isRunning"   },
        { WindowCountRole, "windowCount" },
    };
}

void PinnedAppsModel::setWindowTracker(WindowTracker *tracker)
{
    if (m_windowTracker)
        disconnect(m_windowTracker, nullptr, this, nullptr);

    m_windowTracker = tracker;

    if (m_windowTracker) {
        // All signals → single coalesced sync
        connect(m_windowTracker, &QAbstractItemModel::rowsInserted,
                this, &PinnedAppsModel::scheduleSyncRunningState);
        connect(m_windowTracker, &QAbstractItemModel::rowsRemoved,
                this, &PinnedAppsModel::scheduleSyncRunningState);
        connect(m_windowTracker, &QAbstractItemModel::dataChanged,
                this, &PinnedAppsModel::scheduleSyncRunningState);
        connect(m_windowTracker, &QAbstractItemModel::modelReset,
                this, &PinnedAppsModel::scheduleSyncRunningState);
    }

    load();
    syncRunningState();
}

void PinnedAppsModel::setDesktopEntryModel(DesktopEntryModel *model)
{
    m_desktopEntries = model;
}

void PinnedAppsModel::scheduleSyncRunningState()
{
    // Multiple signals in same event loop tick → one sync
    if (!m_syncTimer.isActive())
        m_syncTimer.start();
}

void PinnedAppsModel::syncRunningState()
{
    if (!m_windowTracker)
        return;

    QHash<QString, int> running;
    for (int i = 0; i < m_windowTracker->rowCount(); ++i) {
        QString aid = m_windowTracker->data(
            m_windowTracker->index(i), WindowTracker::AppIdRole).toString();
        running[aid]++;
    }

    // Collect changed indices, emit once
    QVector<int> changed;
    for (int i = 0; i < m_pinned.size(); ++i) {
        bool wasRunning = m_pinned[i].isRunning;
        int oldCount = m_pinned[i].windowCount;

        int cnt = running.value(m_pinned[i].appId, 0);
        m_pinned[i].isRunning = (cnt > 0);
        m_pinned[i].windowCount = cnt;

        if (wasRunning != m_pinned[i].isRunning || oldCount != cnt)
            changed.append(i);
    }

    // Batch emit — one dataChanged per changed range
    if (!changed.isEmpty()) {
        int first = changed.first();
        int last = changed.last();
        emit dataChanged(index(first), index(last), { IsRunningRole, WindowCountRole });
    }
}

void PinnedAppsModel::pin(const QString &appId)
{
    if (isPinned(appId))
        return;

    PinnedApp app = resolveApp(appId);
    if (app.appId.isEmpty())
        app.appId = appId;

    int row = m_pinned.size();
    beginInsertRows({}, row, row);
    m_pinned.append(app);
    endInsertRows();
    emit countChanged();
    emit pinnedAppsChanged();

    syncRunningState();
    m_saveTimer.start();
}

void PinnedAppsModel::unpin(const QString &appId)
{
    for (int i = 0; i < m_pinned.size(); ++i) {
        if (m_pinned[i].appId == appId) {
            beginRemoveRows({}, i, i);
            m_pinned.removeAt(i);
            endRemoveRows();
            emit countChanged();
            emit pinnedAppsChanged();
            m_saveTimer.start();
            return;
        }
    }
}

void PinnedAppsModel::reorder(int from, int to)
{
    if (from < 0 || from >= m_pinned.size() ||
        to < 0 || to >= m_pinned.size() || from == to)
        return;

    int dest = (to > from) ? to + 1 : to;
    if (!beginMoveRows({}, from, from, {}, dest))
        return;

    m_pinned.move(from, to);
    endMoveRows();
    m_saveTimer.start();
}

void PinnedAppsModel::launch(int index)
{
    if (index < 0 || index >= m_pinned.size())
        return;

    const auto &app = m_pinned[index];

    if (app.isRunning && m_windowTracker) {
        for (int i = 0; i < m_windowTracker->rowCount(); ++i) {
            QString aid = m_windowTracker->data(
                m_windowTracker->index(i),
                WindowTracker::AppIdRole).toString();
            if (aid == app.appId) {
                m_windowTracker->toggleMinimize(i);
                return;
            }
        }
        return;
    }

    QString cmd = app.exec;
    cmd.remove(QRegularExpression("%[fFuUdDnNickvm]"));
    cmd = cmd.trimmed();

    if (cmd.isEmpty())
        return;

    QStringList args = QProcess::splitCommand(cmd);
    if (!args.isEmpty()) {
        QString program = args.takeFirst();
        QProcess::startDetached(program, args);
    }
}

bool PinnedAppsModel::isPinned(const QString &appId) const
{
    for (const auto &p : m_pinned)
        if (p.appId == appId) return true;
    return false;
}

QSet<QString> PinnedAppsModel::pinnedAppIds() const
{
    QSet<QString> ids;
    for (const auto &p : m_pinned) ids.insert(p.appId);
    return ids;
}

PinnedApp PinnedAppsModel::resolveApp(const QString &appId) const
{
    PinnedApp app;
    app.appId = appId;

    if (!m_desktopEntries) return app;

    for (int i = 0; i < m_desktopEntries->rowCount(); ++i) {
        QModelIndex idx = m_desktopEntries->index(i);
        if (m_desktopEntries->data(idx, DesktopEntryModel::IdRole).toString() == appId) {
            app.name = m_desktopEntries->data(idx, DesktopEntryModel::NameRole).toString();
            app.iconName = m_desktopEntries->data(idx, DesktopEntryModel::IconRole).toString();
            app.exec = m_desktopEntries->data(idx, DesktopEntryModel::ExecRole).toString();

            static const QStringList dirs = {
                "/usr/share/applications",
                "/usr/local/share/applications",
                QDir::homePath() + "/.local/share/applications",
                "/var/lib/flatpak/exports/share/applications",
                QDir::homePath() + "/.local/share/flatpak/exports/share/applications"
            };
            for (const auto &dir : dirs) {
                QString path = dir + "/" + appId + ".desktop";
                if (QFile::exists(path)) { app.desktopFile = path; break; }
            }
            return app;
        }
    }

    if (app.name.isEmpty()) app.name = appId;
    if (app.iconName.isEmpty()) app.iconName = appId;
    return app;
}

QString PinnedAppsModel::configDir() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/chi-shell";
}

QString PinnedAppsModel::configFilePath() const
{
    return configDir() + "/pinned.json";
}

void PinnedAppsModel::load()
{
    QFile file(configFilePath());
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonArray arr = QJsonDocument::fromJson(file.readAll()).object().value("pinned").toArray();

    beginResetModel();
    m_pinned.clear();
    for (const auto &val : arr) {
        QString appId = val.toString();
        if (!appId.isEmpty()) {
            PinnedApp app = resolveApp(appId);
            if (!app.appId.isEmpty()) m_pinned.append(app);
        }
    }
    endResetModel();
    emit countChanged();
    emit pinnedAppsChanged();
}

void PinnedAppsModel::save() const
{
    QDir().mkpath(configDir());
    QJsonArray arr;
    for (const auto &p : m_pinned) arr.append(p.appId);

    QJsonObject root;
    root["version"] = 1;
    root["pinned"] = arr;

    QFile file(configFilePath());
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

// ═════════════════════════════════════════════════════════
// UnpinnedWindowsModel
// ═════════════════════════════════════════════════════════

UnpinnedWindowsModel::UnpinnedWindowsModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(false);  // we control when to re-filter

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(0);
    connect(&m_refreshTimer, &QTimer::timeout, this, &UnpinnedWindowsModel::refresh);
}

void UnpinnedWindowsModel::setPinnedModel(PinnedAppsModel *pinned)
{
    if (m_pinned)
        disconnect(m_pinned, nullptr, this, nullptr);

    m_pinned = pinned;

    if (m_pinned) {
        connect(m_pinned, &PinnedAppsModel::pinnedAppsChanged,
                this, &UnpinnedWindowsModel::scheduleRefresh);
    }

    refresh();
}

void UnpinnedWindowsModel::scheduleRefresh()
{
    if (!m_refreshTimer.isActive())
        m_refreshTimer.start();
}

void UnpinnedWindowsModel::refresh()
{
    beginFilterChange();
    endFilterChange();
    emit countChanged();
}

bool UnpinnedWindowsModel::filterAcceptsRow(
    int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_pinned) return true;

    auto idx = sourceModel()->index(sourceRow, 0, sourceParent);
    QString appId = sourceModel()->data(idx, WindowTracker::AppIdRole).toString();
    return !m_pinned->isPinned(appId);
}

void UnpinnedWindowsModel::activate(int proxyIndex)
{
    auto src = mapToSource(index(proxyIndex, 0));
    auto *tracker = qobject_cast<WindowTracker*>(sourceModel());
    if (tracker) tracker->activate(src.row());
}

void UnpinnedWindowsModel::close(int proxyIndex)
{
    auto src = mapToSource(index(proxyIndex, 0));
    auto *tracker = qobject_cast<WindowTracker*>(sourceModel());
    if (tracker) tracker->close(src.row());
}

void UnpinnedWindowsModel::toggleMinimize(int proxyIndex)
{
    auto src = mapToSource(index(proxyIndex, 0));
    auto *tracker = qobject_cast<WindowTracker*>(sourceModel());
    if (tracker) tracker->toggleMinimize(src.row());
}
