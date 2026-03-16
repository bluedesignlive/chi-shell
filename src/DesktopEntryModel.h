#ifndef DESKTOPENTRYMODEL_H
#define DESKTOPENTRYMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QSortFilterProxyModel>

struct DesktopEntry
{
    QString id;
    QString name;
    QString genericName;
    QString comment;
    QString exec;
    QString icon;
    QStringList categories;
    QStringList keywords;
    bool noDisplay = false;
    int launchCount = 0;
};

class DesktopEntryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        NameRole,
        GenericNameRole,
        CommentRole,
        ExecRole,
        IconRole,
        CategoriesRole,
        KeywordsRole,
        LaunchCountRole
    };

    explicit DesktopEntryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_entries.size(); }

    Q_INVOKABLE void launch(int index);

public slots:
    void refresh();

signals:
    void countChanged();

private:
    void loadEntries();
    void loadDirectory(const QString &path);
    DesktopEntry parseDesktopFile(const QString &filePath);

    QVector<DesktopEntry> m_entries;
};

class AppFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit AppFilterModel(QObject *parent = nullptr);

    QString searchText() const { return m_searchText; }
    void setSearchText(const QString &text);
    int count() const { return rowCount(); }

    Q_INVOKABLE void launch(int proxyIndex);

signals:
    void searchTextChanged();
    void countChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;

private:
    QString m_searchText;
};

#endif // DESKTOPENTRYMODEL_H
