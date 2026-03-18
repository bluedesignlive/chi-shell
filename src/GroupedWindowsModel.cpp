#include "GroupedWindowsModel.h"
#include "WindowTracker.h"
#include "PinnedAppsModel.h"

#include <QSet>

GroupedWindowsModel::GroupedWindowsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    m_rebuildTimer.setSingleShot(true);
    m_rebuildTimer.setInterval(0);
    connect(&m_rebuildTimer, &QTimer::timeout, this, &GroupedWindowsModel::rebuild);
}

void GroupedWindowsModel::setWindowTracker(WindowTracker *tracker)
{
    if (m_windowTracker)
        disconnect(m_windowTracker, nullptr, this, nullptr);
    m_windowTracker = tracker;
    if (m_windowTracker) {
        connect(m_windowTracker, &QAbstractItemModel::rowsInserted,
                this, &GroupedWindowsModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::rowsRemoved,
                this, &GroupedWindowsModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::dataChanged,
                this, &GroupedWindowsModel::scheduleRebuild);
        connect(m_windowTracker, &QAbstractItemModel::modelReset,
                this, &GroupedWindowsModel::scheduleRebuild);
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
    case AppIdRole:           return g.appId;
    case TitleRole:           return g.title;
    case IconNameRole:        return g.iconName;
    case WindowCountRole:     return g.windowCount;
    case IsActivatedRole:     return g.isActivated;
    case IsMinimizedRole:     return g.isMinimized;
    case FirstWindowIndexRole: return g.firstWindowIndex;
    }
    return {};
}

QHash<int, QByteArray> GroupedWindowsModel::roleNames() const
{
    return {
        { AppIdRole,           "appId" },
        { TitleRole,           "title" },
        { IconNameRole,        "iconName" },
        { WindowCountRole,     "windowCount" },
        { IsActivatedRole,     "isActivated" },
        { IsMinimizedRole,     "isMinimized" },
        { FirstWindowIndexRole, "firstWindowIndex" },
    };
}

void GroupedWindowsModel::scheduleRebuild()
{
    if (!m_rebuildTimer.isActive())
        m_rebuildTimer.start();
}

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

            // Skip pinned apps — they're shown in PinnedAppsSection
            if (pinnedIds.contains(aid))
                continue;

            bool activated = m_windowTracker->data(idx, WindowTracker::ActivatedRole).toBool();
            bool minimized = m_windowTracker->data(idx, WindowTracker::MinimizedRole).toBool();

            if (appIdToGroupIdx.contains(aid)) {
                // Update existing group
                int gi = appIdToGroupIdx[aid];
                newGroups[gi].windowCount++;
                if (activated) {
                    newGroups[gi].isActivated = true;
                    // Use active window's title
                    newGroups[gi].title = m_windowTracker->data(idx, WindowTracker::TitleRole).toString();
                }
                // Only minimized if ALL windows are minimized
                if (!minimized)
                    newGroups[gi].isMinimized = false;
            } else {
                // New group
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
    const auto &g = m_groups[index];
    if (g.windowCount > 1) {
        // Multiple windows — use scale to pick
        // For now, just toggle first window
        m_windowTracker->toggleMinimize(g.firstWindowIndex);
    } else {
        m_windowTracker->toggleMinimize(g.firstWindowIndex);
    }
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
