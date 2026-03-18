#include "GroupedWindowsModel.h"
#include "WindowTracker.h"
#include "PinnedAppsModel.h"

#include <QSet>

GroupedWindowsModel::GroupedWindowsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_updateTimer.setSingleShot(true);
    m_updateTimer.setInterval(0);
    connect(&m_updateTimer, &QTimer::timeout, this, &GroupedWindowsModel::processUpdate);
}

void GroupedWindowsModel::setWindowTracker(WindowTracker *tracker)
{
    if (m_windowTracker)
        disconnect(m_windowTracker, nullptr, this, nullptr);
    m_windowTracker = tracker;
    if (m_windowTracker) {
        // Structural changes (windows added/removed) → full rebuild
        connect(m_windowTracker, &QAbstractItemModel::rowsInserted,
                this, &GroupedWindowsModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::rowsRemoved,
                this, &GroupedWindowsModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::modelReset,
                this, &GroupedWindowsModel::scheduleRebuild);
        // State changes (title, activated, minimized) → lightweight refresh
        connect(m_windowTracker, &QAbstractItemModel::dataChanged,
                this, &GroupedWindowsModel::scheduleRefreshState);
    }
    rebuild();
}

void GroupedWindowsModel::setPinnedModel(PinnedAppsModel *pinned)
{
    if (m_pinnedModel)
        disconnect(m_pinnedModel, nullptr, this, nullptr);
    m_pinnedModel = pinned;
    if (m_pinnedModel) {
        connect(m_pinnedModel, &PinnedAppsModel::pinnedAppsChanged,
                this, &GroupedWindowsModel::scheduleRebuild);
    }
    rebuild();
}

int GroupedWindowsModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_groups.size();
}

QVariant GroupedWindowsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_groups.size())
        return {};
    const auto &g = m_groups.at(index.row());
    switch (role) {
    case AppIdRole:            return g.appId;
    case TitleRole:            return g.title;
    case IconNameRole:         return g.iconName;
    case WindowCountRole:      return g.windowCount;
    case IsActivatedRole:      return g.isActivated;
    case IsMinimizedRole:      return g.isMinimized;
    case FirstWindowIndexRole: return g.firstWindowIndex;
    }
    return {};
}

QHash<int, QByteArray> GroupedWindowsModel::roleNames() const
{
    return {
        { AppIdRole,            "appId" },
        { TitleRole,            "title" },
        { IconNameRole,         "iconName" },
        { WindowCountRole,      "windowCount" },
        { IsActivatedRole,      "isActivated" },
        { IsMinimizedRole,      "isMinimized" },
        { FirstWindowIndexRole, "firstWindowIndex" },
    };
}

// ── Update scheduling ────────────────────────────────────────

void GroupedWindowsModel::scheduleRebuild()
{
    m_needsRebuild = true;
    if (!m_updateTimer.isActive())
        m_updateTimer.start();
}

void GroupedWindowsModel::scheduleRefreshState()
{
    // Only schedule lightweight refresh if a rebuild isn't already pending
    if (!m_needsRebuild && !m_updateTimer.isActive())
        m_updateTimer.start();
}

void GroupedWindowsModel::processUpdate()
{
    if (m_needsRebuild) {
        m_needsRebuild = false;
        rebuild();
    } else {
        refreshState();
    }
}

// ── Lightweight state refresh (no model reset) ───────────────
//
// Called when window title/activated/minimized changes.
// Updates existing groups IN-PLACE via dataChanged — no QML
// delegate destruction, no flicker, no hover loss.

void GroupedWindowsModel::refreshState()
{
    if (!m_windowTracker || m_groups.isEmpty()) return;

    QSet<QString> pinnedIds;
    if (m_pinnedModel)
        pinnedIds = m_pinnedModel->pinnedAppIds();

    // Single pass over all windows → collect fresh state per appId
    struct FreshState {
        int windowCount      = 0;
        bool isActivated     = false;
        bool allMinimized    = true;
        int firstWindowIndex = -1;
        QString activeTitle;
        QString firstTitle;
        QString iconName;
    };
    QHash<QString, FreshState> stateMap;

    for (int i = 0; i < m_windowTracker->rowCount(); ++i) {
        auto idx = m_windowTracker->index(i);
        QString aid = m_windowTracker->data(idx, WindowTracker::AppIdRole).toString();
        if (pinnedIds.contains(aid))
            continue;

        auto &s = stateMap[aid];
        s.windowCount++;
        if (s.firstWindowIndex < 0) {
            s.firstWindowIndex = i;
            s.firstTitle = m_windowTracker->data(idx, WindowTracker::TitleRole).toString();
            s.iconName = m_windowTracker->data(idx, WindowTracker::IconNameRole).toString();
        }
        if (m_windowTracker->data(idx, WindowTracker::ActivatedRole).toBool()) {
            s.isActivated = true;
            s.activeTitle = m_windowTracker->data(idx, WindowTracker::TitleRole).toString();
        }
        if (!m_windowTracker->data(idx, WindowTracker::MinimizedRole).toBool())
            s.allMinimized = false;
    }

    // If group structure changed (new app appeared, or app gone) → full rebuild
    bool structureChanged = (stateMap.size() != m_groups.size());
    if (!structureChanged) {
        for (const auto &g : m_groups) {
            if (!stateMap.contains(g.appId)) {
                structureChanged = true;
                break;
            }
        }
    }
    if (structureChanged) {
        rebuild();
        return;
    }

    // Update each group in-place
    for (int gi = 0; gi < m_groups.size(); ++gi) {
        auto &g = m_groups[gi];
        const auto &s = stateMap.value(g.appId);

        bool changed = false;
        QString newTitle = s.isActivated ? s.activeTitle : s.firstTitle;

        if (g.windowCount != s.windowCount) {
            g.windowCount = s.windowCount;
            changed = true;
        }
        if (g.isActivated != s.isActivated) {
            g.isActivated = s.isActivated;
            changed = true;
        }
        if (g.isMinimized != s.allMinimized) {
            g.isMinimized = s.allMinimized;
            changed = true;
        }
        if (g.firstWindowIndex != s.firstWindowIndex) {
            g.firstWindowIndex = s.firstWindowIndex;
            changed = true;
        }
        if (g.title != newTitle) {
            g.title = newTitle;
            changed = true;
        }
        if (g.iconName != s.iconName && !s.iconName.isEmpty()) {
            g.iconName = s.iconName;
            changed = true;
        }

        if (changed)
            emit dataChanged(index(gi), index(gi));
    }
}

// ── Full rebuild (structural changes only) ───────────────────

void GroupedWindowsModel::rebuild()
{
    QSet<QString> pinnedIds;
    if (m_pinnedModel)
        pinnedIds = m_pinnedModel->pinnedAppIds();

    QVector<Group> newGroups;
    QHash<QString, int> appIdToGroupIdx;

    if (m_windowTracker) {
        for (int i = 0; i < m_windowTracker->rowCount(); ++i) {
            auto idx = m_windowTracker->index(i);
            QString aid = m_windowTracker->data(idx, WindowTracker::AppIdRole).toString();

            if (pinnedIds.contains(aid))
                continue;

            bool activated = m_windowTracker->data(idx, WindowTracker::ActivatedRole).toBool();
            bool minimized = m_windowTracker->data(idx, WindowTracker::MinimizedRole).toBool();

            if (appIdToGroupIdx.contains(aid)) {
                int gi = appIdToGroupIdx[aid];
                newGroups[gi].windowCount++;
                if (activated) {
                    newGroups[gi].isActivated = true;
                    newGroups[gi].title = m_windowTracker->data(idx, WindowTracker::TitleRole).toString();
                }
                if (!minimized)
                    newGroups[gi].isMinimized = false;
            } else {
                Group g;
                g.appId = aid;
                g.title = m_windowTracker->data(idx, WindowTracker::TitleRole).toString();
                g.iconName = m_windowTracker->data(idx, WindowTracker::IconNameRole).toString();
                g.windowCount = 1;
                g.isActivated = activated;
                g.isMinimized = minimized;
                g.firstWindowIndex = i;
                appIdToGroupIdx[aid] = newGroups.size();
                newGroups.append(g);
            }
        }
    }

    beginResetModel();
    m_groups = newGroups;
    endResetModel();
    emit countChanged();
}

// ── Actions ──────────────────────────────────────────────────

void GroupedWindowsModel::activate(int index)
{
    if (index < 0 || index >= m_groups.size() || !m_windowTracker) return;
    m_windowTracker->activate(m_groups[index].firstWindowIndex);
}

void GroupedWindowsModel::close(int index)
{
    if (index < 0 || index >= m_groups.size() || !m_windowTracker) return;
    m_windowTracker->close(m_groups[index].firstWindowIndex);
}

void GroupedWindowsModel::closeAll(int index)
{
    if (index < 0 || index >= m_groups.size() || !m_windowTracker) return;
    m_windowTracker->closeAllForApp(m_groups[index].appId);
}

void GroupedWindowsModel::toggleMinimize(int index)
{
    if (index < 0 || index >= m_groups.size() || !m_windowTracker) return;
    m_windowTracker->toggleMinimize(m_groups[index].firstWindowIndex);
}

int GroupedWindowsModel::firstWindowIndex(int index) const
{
    if (index < 0 || index >= m_groups.size()) return -1;
    return m_groups[index].firstWindowIndex;
}

QString GroupedWindowsModel::appIdAt(int index) const
{
    if (index < 0 || index >= m_groups.size()) return {};
    return m_groups[index].appId;
}
