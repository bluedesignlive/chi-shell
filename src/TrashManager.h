#ifndef TRASHMANAGER_H
#define TRASHMANAGER_H

#include <QObject>
#include <QAbstractListModel>
#include <QFileSystemWatcher>
#include <QStringList>

struct TrashEntry {
    QString trashName;     // filename inside Trash/files/
    QString originalPath;  // where it came from
    QString deletionDate;  // ISO date string
    qint64  size;          // bytes
    bool    isDir;
};

class TrashListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TrashNameRole = Qt::UserRole + 1,
        DisplayNameRole,
        OriginalPathRole,
        DeletionDateRole,
        SizeRole,
        SizeTextRole,
        IsDirRole,
        IconNameRole,
    };

    explicit TrashListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    int count() const { return m_entries.size(); }

    void refresh(const QString &filesPath, const QString &infoPath);

signals:
    void countChanged();

private:
    static QString humanSize(qint64 bytes);
    static QString iconForFile(const QString &name, bool isDir);

    QVector<TrashEntry> m_entries;
};

class TrashManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY countChanged)
    Q_PROPERTY(QString iconName READ iconName NOTIFY countChanged)
    Q_PROPERTY(TrashListModel* items READ items CONSTANT)

public:
    explicit TrashManager(QObject *parent = nullptr);

    int count() const { return m_count; }
    bool isEmpty() const { return m_count == 0; }
    QString iconName() const { return m_count > 0 ? "delete" : "delete_outline"; }
    TrashListModel *items() { return &m_listModel; }

    Q_INVOKABLE void openTrash();
    Q_INVOKABLE void emptyTrash();
    Q_INVOKABLE bool moveToTrash(const QString &filePath);
    Q_INVOKABLE bool restoreItem(int index);
    Q_INVOKABLE bool restoreLast();
    Q_INVOKABLE bool deleteItem(int index);
    Q_INVOKABLE bool canRestore() const { return !m_restoreStack.isEmpty(); }
    Q_INVOKABLE QString trashItemOriginalPath(int index) const;

signals:
    void countChanged();

private slots:
    void refresh();

private:
    QString trashFilesPath() const;
    QString trashInfoPath() const;
    void ensureTrashDirs();
    TrashEntry readTrashInfo(const QString &trashName) const;

    QFileSystemWatcher m_watcher;
    TrashListModel m_listModel;
    int m_count = 0;
    QStringList m_restoreStack;
};

#endif // TRASHMANAGER_H
