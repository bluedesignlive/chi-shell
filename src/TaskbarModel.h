#ifndef TASKBARMODEL_H
#define TASKBARMODEL_H

#include <QAbstractListModel>
#include <QTimer>
#include <QSet>

class WindowTracker;
class PinnedAppsModel;
class DesktopEntryModel;

class TaskbarModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        NameRole,
        IconNameRole,
        IsPinnedRole,
        IsRunningRole,
        IsActivatedRole,
        WindowCountRole,
        SectionRole,       // "pinned" or "running" — for optional divider
    };

    explicit TaskbarModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_entries.size(); }

    void setWindowTracker(WindowTracker *tracker);
    void setPinnedModel(PinnedAppsModel *pinned);
    void setDesktopEntryModel(DesktopEntryModel *entries);

    // Actions — called from QML
    Q_INVOKABLE void toggleMinimize(int index);
    Q_INVOKABLE void closeWindow(int index);
    Q_INVOKABLE void closeAllWindows(int index);
    Q_INVOKABLE void launch(int index);
    Q_INVOKABLE void launchNew(int index);

    // Pin management — delegates to PinnedAppsModel
    Q_INVOKABLE void pin(int index);
    Q_INVOKABLE void unpin(int index);
    Q_INVOKABLE bool isPinnedAt(int index) const;
    Q_INVOKABLE QString appIdAt(int index) const;
    Q_INVOKABLE QString nameAt(int index) const;
    Q_INVOKABLE int windowCountAt(int index) const;
    Q_INVOKABLE bool isRunningAt(int index) const;
    Q_INVOKABLE bool isActivatedAt(int index) const;

    // Drag reorder — only within pinned section
    Q_INVOKABLE void move(int from, int to);

    // For context menus
    Q_INVOKABLE int pinnedIndexForRow(int row) const;

signals:
    void countChanged();

private slots:
    void scheduleRebuild();
    void rebuild();

private:
    struct Entry {
        QString appId;
        QString name;
        QString iconName;
        bool isPinned    = false;
        bool isRunning   = false;
        bool isActivated = false;
        int  windowCount = 0;
    };

    QString resolveAppName(const QString &appId) const;
    QString resolveIconForApp(const QString &appId) const;

    QVector<Entry>   m_entries;
    WindowTracker   *m_windowTracker  = nullptr;
    PinnedAppsModel *m_pinnedModel    = nullptr;
    DesktopEntryModel *m_desktopEntries = nullptr;
    QTimer           m_rebuildTimer;
    int              m_pinnedCount = 0; // how many entries are pinned
};

#endif // TASKBARMODEL_H
