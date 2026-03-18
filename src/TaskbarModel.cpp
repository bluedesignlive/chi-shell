#include "TaskbarModel.h"
#include "WindowTracker.h"
#include "PinnedAppsModel.h"
#include "DesktopEntryModel.h"

#include <QDebug>

TaskbarModel::TaskbarModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_rebuildTimer.setSingleShot(true);
    m_rebuildTimer.setInterval(0);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &TaskbarModel::rebuild);
}

// ── Data source wiring ───────────────────────────────────────────

void TaskbarModel::setWindowTracker(WindowTracker *tracker)
{
    if (m_windowTracker)
        disconnect(m_windowTracker, nullptr, this, nullptr);
    m_windowTracker = tracker;
    if (m_windowTracker) {
        connect(m_windowTracker, &QAbstractItemModel::rowsInserted,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::rowsRemoved,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::dataChanged,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::modelReset,
                this, &TaskbarModel::scheduleRebuild);
    }
    scheduleRebuild();
}

void TaskbarModel::setPinnedModel(PinnedAppsModel *pinned)
{
    if (m_pinnedModel)
        disconnect(m_pinnedModel, nullptr, this, nullptr);
    m_pinnedModel = pinned;
    if (m_pinnedModel) {
        connect(m_pinnedModel, &QAbstractItemModel::rowsInserted,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_pinnedModel, &QAbstractItemModel::rowsRemoved,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_pinnedModel, &QAbstractItemModel::dataChanged,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_pinnedModel, &QAbstractItemModel::modelReset,
                this, &TaskbarModel::scheduleRebuild);
        connect(m_pinnedModel, &PinnedAppsModel::pinnedAppsChanged,
                this, &TaskbarModel::scheduleRebuild);
    }
    scheduleRebuild();
}

void TaskbarModel::setDesktopEntryModel(DesktopEntryModel *entries)
{
    m_desktopEntries = entries;
}

// ── QAbstractListModel ──────────────────────────────────────────

int TaskbarModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant TaskbarModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const auto &e = m_entries.at(index.row());
    switch (role) {
    case AppIdRole:       return e.appId;
    case NameRole:        return e.name;
    case IconNameRole:    return e.iconName;
    case IsPinnedRole:    return e.isPinned;
    case IsRunningRole:   return e.isRunning;
    case IsActivatedRole: return e.isActivated;
    case WindowCountRole: return e.windowCount;
    case SectionRole:     return e.isPinned ? QStringLiteral("pinned")
                                            : QStringLiteral("running");
    }
    return {};
}

QHash<int, QByteArray> TaskbarModel::roleNames() const
{
    return {
        { AppIdRole,       "appId"       },
        { NameRole,        "name"        },
        { IconNameRole,    "iconName"    },
        { IsPinnedRole,    "isPinned"    },
        { IsRunningRole,   "isRunning"   },
        { IsActivatedRole, "isActivated" },
        { WindowCountRole, "windowCount" },
        { SectionRole,     "section"     },
    };
}

// ── Rebuild ─────────────────────────────────────────────────────

void TaskbarModel::scheduleRebuild()
{
    if (!m_rebuildTimer.isActive())
        m_rebuildTimer.start();
}

void TaskbarModel::rebuild()
{
    // 1. Collect running-window state from WindowTracker
    QHash<QString, int>  windowCounts;
    QHash<QString, bool> activeApps;
    QSet<QString>        runningIds;
    QHash<QString, QString> runningIcons;  // first icon seen per appId
    QHash<QString, QString> runningTitles; // first title seen per appId

    if (m_windowTracker) {
        for (int i = 0; i < m_windowTracker->rowCount(); ++i) {
            auto idx = m_windowTracker->index(i);
            QString aid = m_windowTracker->data(idx, WindowTracker::AppIdRole).toString();
            if (aid.isEmpty()) continue;
            windowCounts[aid]++;
            runningIds.insert(aid);
            if (m_windowTracker->data(idx, WindowTracker::ActivatedRole).toBool())
                activeApps[aid] = true;
            if (!runningIcons.contains(aid))
                runningIcons[aid] = m_windowTracker->data(idx, WindowTracker::IconNameRole).toString();
            if (!runningTitles.contains(aid))
                runningTitles[aid] = m_windowTracker->data(idx, WindowTracker::TitleRole).toString();
        }
    }

    // 2. Build unified entry list
    QVector<Entry> newEntries;
    QSet<QString> seen;
    int pinnedCount = 0;

    // 2a. Pinned apps in saved order
    if (m_pinnedModel) {
        for (int i = 0; i < m_pinnedModel->rowCount(); ++i) {
            auto idx = m_pinnedModel->index(i);
            Entry e;
            e.appId    = m_pinnedModel->data(idx, PinnedAppsModel::AppIdRole).toString();
            e.name     = m_pinnedModel->data(idx, PinnedAppsModel::NameRole).toString();
            e.iconName = m_pinnedModel->data(idx, PinnedAppsModel::IconNameRole).toString();
            e.isPinned    = true;
            e.isRunning   = runningIds.contains(e.appId);
            e.isActivated = activeApps.value(e.appId, false);
            e.windowCount = windowCounts.value(e.appId, 0);
            // Prefer running icon if available (may be higher quality)
            if (e.isRunning && runningIcons.contains(e.appId)
                && !runningIcons[e.appId].isEmpty()) {
                e.iconName = runningIcons[e.appId];
            }
            newEntries.append(e);
            seen.insert(e.appId);
            pinnedCount++;
        }
    }

    // 2b. Unpinned running apps, grouped by appId, in order of first appearance
    if (m_windowTracker) {
        for (int i = 0; i < m_windowTracker->rowCount(); ++i) {
            auto idx = m_windowTracker->index(i);
            QString aid = m_windowTracker->data(idx, WindowTracker::AppIdRole).toString();
            if (aid.isEmpty() || seen.contains(aid)) continue;
            seen.insert(aid);

            Entry e;
            e.appId       = aid;
            e.name        = resolveAppName(aid);
            if (e.name.isEmpty())
                e.name = runningTitles.value(aid, aid);
            e.iconName    = runningIcons.value(aid, aid);
            e.isPinned    = false;
            e.isRunning   = true;
            e.isActivated = activeApps.value(aid, false);
            e.windowCount = windowCounts.value(aid, 0);
            newEntries.append(e);
        }
    }

    // 3. Commit to model
    beginResetModel();
    m_entries = newEntries;
    m_pinnedCount = pinnedCount;
    endResetModel();
    emit countChanged();
}

QString TaskbarModel::resolveAppName(const QString &appId) const
{
    if (!m_desktopEntries) return {};
    for (int i = 0; i < m_desktopEntries->rowCount(); ++i) {
        auto idx = m_desktopEntries->index(i);
        if (m_desktopEntries->data(idx, DesktopEntryModel::IdRole).toString() == appId)
            return m_desktopEntries->data(idx, DesktopEntryModel::NameRole).toString();
    }
    return {};
}

QString TaskbarModel::resolveIconForApp(const QString &appId) const
{
    if (!m_desktopEntries) return appId;
    for (int i = 0; i < m_desktopEntries->rowCount(); ++i) {
        auto idx = m_desktopEntries->index(i);
        if (m_desktopEntries->data(idx, DesktopEntryModel::IdRole).toString() == appId)
            return m_desktopEntries->data(idx, DesktopEntryModel::IconRole).toString();
    }
    return appId;
}

// ── Actions ─────────────────────────────────────────────────────

void TaskbarModel::toggleMinimize(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_windowTracker) return;
    int wi = m_windowTracker->firstIndexForApp(m_entries[index].appId);
    if (wi >= 0) m_windowTracker->toggleMinimize(wi);
}

void TaskbarModel::closeWindow(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_windowTracker) return;
    int wi = m_windowTracker->firstIndexForApp(m_entries[index].appId);
    if (wi >= 0) m_windowTracker->close(wi);
}

void TaskbarModel::closeAllWindows(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_windowTracker) return;
    m_windowTracker->closeAllForApp(m_entries[index].appId);
}

void TaskbarModel::launch(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_pinnedModel) return;
    const auto &e = m_entries[index];
    // If pinned, use PinnedAppsModel::launch (handles toggle-minimize for running)
    if (e.isPinned) {
        int pi = pinnedIndexForRow(index);
        if (pi >= 0) m_pinnedModel->launch(pi);
        return;
    }
    // Unpinned running app — toggle minimize
    if (m_windowTracker) {
        int wi = m_windowTracker->firstIndexForApp(e.appId);
        if (wi >= 0) m_windowTracker->toggleMinimize(wi);
    }
}

void TaskbarModel::launchNew(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_pinnedModel) return;
    m_pinnedModel->launchNew(m_entries[index].appId);
}

// ── Pin management ──────────────────────────────────────────────

void TaskbarModel::pin(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_pinnedModel) return;
    m_pinnedModel->pin(m_entries[index].appId);
    // rebuild will be triggered via signal
}

void TaskbarModel::unpin(int index)
{
    if (index < 0 || index >= m_entries.size() || !m_pinnedModel) return;
    m_pinnedModel->unpin(m_entries[index].appId);
}

bool TaskbarModel::isPinnedAt(int index) const
{
    if (index < 0 || index >= m_entries.size()) return false;
    return m_entries[index].isPinned;
}

QString TaskbarModel::appIdAt(int index) const
{
    if (index < 0 || index >= m_entries.size()) return {};
    return m_entries[index].appId;
}

QString TaskbarModel::nameAt(int index) const
{
    if (index < 0 || index >= m_entries.size()) return {};
    return m_entries[index].name;
}

int TaskbarModel::windowCountAt(int index) const
{
    if (index < 0 || index >= m_entries.size()) return 0;
    return m_entries[index].windowCount;
}

bool TaskbarModel::isRunningAt(int index) const
{
    if (index < 0 || index >= m_entries.size()) return false;
    return m_entries[index].isRunning;
}

bool TaskbarModel::isActivatedAt(int index) const
{
    if (index < 0 || index >= m_entries.size()) return false;
    return m_entries[index].isActivated;
}

// ── Drag reorder (pinned only) ──────────────────────────────────

void TaskbarModel::move(int from, int to)
{
    // Only allow reordering within the pinned section
    if (from < 0 || from >= m_pinnedCount ||
        to < 0   || to >= m_pinnedCount   || from == to)
        return;
    if (m_pinnedModel)
        m_pinnedModel->reorder(from, to);
    // rebuild will be triggered via signal from PinnedAppsModel
}

int TaskbarModel::pinnedIndexForRow(int row) const
{
    if (row < 0 || row >= m_entries.size() || !m_entries[row].isPinned)
        return -1;
    // Count pinned entries before this row
    int pi = 0;
    for (int i = 0; i < row; ++i) {
        if (m_entries[i].isPinned) pi++;
    }
    return pi;
}
