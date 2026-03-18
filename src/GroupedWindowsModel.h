#ifndef GROUPEDWINDOWSMODEL_H
#define GROUPEDWINDOWSMODEL_H

#include <QAbstractListModel>
#include <QTimer>

class WindowTracker;
class PinnedAppsModel;

class GroupedWindowsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        TitleRole,
        IconNameRole,
        WindowCountRole,
        IsActivatedRole,
        IsMinimizedRole,
        FirstWindowIndexRole,
    };

    explicit GroupedWindowsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_groups.size(); }

    void setWindowTracker(WindowTracker *tracker);
    void setPinnedModel(PinnedAppsModel *pinned);

    Q_INVOKABLE void activate(int index);
    Q_INVOKABLE void close(int index);
    Q_INVOKABLE void closeAll(int index);
    Q_INVOKABLE void toggleMinimize(int index);
    Q_INVOKABLE int firstWindowIndex(int index) const;
    Q_INVOKABLE QString appIdAt(int index) const;

signals:
    void countChanged();

private slots:
    void scheduleRebuild();
    void scheduleRefreshState();
    void processUpdate();

private:
    void rebuild();
    void refreshState();

    struct Group {
        QString appId;
        QString title;
        QString iconName;
        int windowCount = 0;
        bool isActivated = false;
        bool isMinimized = false;
        int firstWindowIndex = -1;
    };

    QVector<Group> m_groups;
    WindowTracker *m_windowTracker = nullptr;
    PinnedAppsModel *m_pinnedModel = nullptr;
    QTimer m_updateTimer;
    bool m_needsRebuild = false;
};

#endif // GROUPEDWINDOWSMODEL_H
