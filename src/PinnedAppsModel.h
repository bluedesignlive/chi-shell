#ifndef PINNEDAPPSMODEL_H
#define PINNEDAPPSMODEL_H

#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QSet>
#include <QTimer>

class WindowTracker;
class DesktopEntryModel;

struct PinnedApp
{
    QString appId;
    QString name;
    QString iconName;
    QString desktopFile;
    QString exec;
    bool isRunning   = false;
    bool isActivated = false;
    int  windowCount = 0;
};

class PinnedAppsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        AppIdRole = Qt::UserRole + 1,
        NameRole,
        IconNameRole,
        DesktopFileRole,
        ExecRole,
        IsRunningRole,
        IsActivatedRole,
        WindowCountRole
    };

    explicit PinnedAppsModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_pinned.size(); }

    void setWindowTracker(WindowTracker *tracker);
    void setDesktopEntryModel(DesktopEntryModel *model);

    Q_INVOKABLE void pin(const QString &appId);
    Q_INVOKABLE void unpin(const QString &appId);
    Q_INVOKABLE void reorder(int from, int to);
    Q_INVOKABLE void launch(int index);
    Q_INVOKABLE void launchNew(const QString &appId);
    Q_INVOKABLE bool isPinned(const QString &appId) const;
    Q_INVOKABLE QString execForApp(const QString &appId) const;

    QSet<QString> pinnedAppIds() const;

public slots:
    void scheduleSyncRunningState();
    void syncRunningState();

signals:
    void countChanged();
    void pinnedAppsChanged();

private:
    void load();
    void save() const;
    PinnedApp resolveApp(const QString &appId) const;
    QString configDir() const;
    QString configFilePath() const;

    QVector<PinnedApp>  m_pinned;
    WindowTracker      *m_windowTracker  = nullptr;
    DesktopEntryModel  *m_desktopEntries = nullptr;
    QTimer              m_saveTimer;
    QTimer              m_syncTimer;
};

class UnpinnedWindowsModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit UnpinnedWindowsModel(QObject *parent = nullptr);

    void setPinnedModel(PinnedAppsModel *pinned);
    int count() const { return rowCount(); }

    Q_INVOKABLE void activate(int proxyIndex);
    Q_INVOKABLE void close(int proxyIndex);
    Q_INVOKABLE void toggleMinimize(int proxyIndex);
    Q_INVOKABLE int  sourceIndex(int proxyIndex) const;

public slots:
    void refresh();
    void scheduleRefresh();

signals:
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow,
                          const QModelIndex &sourceParent) const override;

private:
    PinnedAppsModel *m_pinned = nullptr;
    QTimer m_refreshTimer;
};

#endif // PINNEDAPPSMODEL_H
