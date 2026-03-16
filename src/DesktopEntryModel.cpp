#include "DesktopEntryModel.h"
#include "FuzzyMatcher.h"

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

// ═════════════════════════════════════════════════════════
// DesktopEntryModel
// ═════════════════════════════════════════════════════════

DesktopEntryModel::DesktopEntryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    loadEntries();
}

int DesktopEntryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant DesktopEntryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};

    const auto &e = m_entries.at(index.row());

    switch (role) {
    case IdRole:          return e.id;
    case NameRole:        return e.name;
    case GenericNameRole: return e.genericName;
    case CommentRole:     return e.comment;
    case ExecRole:        return e.exec;
    case IconRole:        return e.icon;
    case CategoriesRole:  return e.categories;
    case KeywordsRole:    return e.keywords;
    case LaunchCountRole: return e.launchCount;
    }

    return {};
}

QHash<int, QByteArray> DesktopEntryModel::roleNames() const
{
    return {
        { IdRole,          "entryId"     },
        { NameRole,        "name"        },
        { GenericNameRole, "genericName" },
        { CommentRole,     "comment"     },
        { ExecRole,        "exec"        },
        { IconRole,        "iconName"    },
        { CategoriesRole,  "categories"  },
        { KeywordsRole,    "keywords"    },
        { LaunchCountRole, "launchCount" }
    };
}

void DesktopEntryModel::loadEntries()
{
    beginResetModel();
    m_entries.clear();

    static const QStringList dirs = {
        "/usr/share/applications",
        "/usr/local/share/applications",
        QDir::homePath() + "/.local/share/applications",
        "/var/lib/flatpak/exports/share/applications",
        QDir::homePath() + "/.local/share/flatpak/exports/share/applications"
    };

    QSet<QString> seen;

    for (const auto &dir : dirs) {
        QDir d(dir);
        if (!d.exists())
            continue;

        const auto files = d.entryList({"*.desktop"}, QDir::Files);
        for (const auto &file : files) {
            if (seen.contains(file))
                continue;
            seen.insert(file);

            DesktopEntry entry = parseDesktopFile(d.filePath(file));
            if (!entry.name.isEmpty() && !entry.noDisplay)
                m_entries.append(entry);
        }
    }

    std::sort(m_entries.begin(), m_entries.end(),
        [](const DesktopEntry &a, const DesktopEntry &b) {
            return a.name.toLower() < b.name.toLower();
        });

    endResetModel();
    emit countChanged();
    qDebug() << "DesktopEntryModel: loaded" << m_entries.size() << "apps";
}

DesktopEntry DesktopEntryModel::parseDesktopFile(const QString &filePath)
{
    DesktopEntry entry;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return entry;

    entry.id = QFileInfo(filePath).completeBaseName();

    bool inDesktopEntry = false;
    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.startsWith('[')) {
            inDesktopEntry = (line == "[Desktop Entry]");
            continue;
        }

        if (!inDesktopEntry || line.isEmpty() || line.startsWith('#'))
            continue;

        int eq = line.indexOf('=');
        if (eq < 0)
            continue;

        QString key = line.left(eq).trimmed();
        QString val = line.mid(eq + 1).trimmed();

        if (key.contains('['))
            continue;

        if (key == "Name")             entry.name = val;
        else if (key == "GenericName") entry.genericName = val;
        else if (key == "Comment")     entry.comment = val;
        else if (key == "Exec")        entry.exec = val;
        else if (key == "Icon")        entry.icon = val;
        else if (key == "Categories")  entry.categories = val.split(';', Qt::SkipEmptyParts);
        else if (key == "Keywords")    entry.keywords = val.split(';', Qt::SkipEmptyParts);
        else if (key == "NoDisplay")   entry.noDisplay = (val.toLower() == "true");
        else if (key == "Hidden")      entry.noDisplay = entry.noDisplay || (val.toLower() == "true");
    }

    return entry;
}

void DesktopEntryModel::launch(int index)
{
    if (index < 0 || index >= m_entries.size())
        return;

    auto &entry = m_entries[index];
    entry.launchCount++;

    QString cmd = entry.exec;
    cmd.remove(QRegularExpression("%[fFuUdDnNickvm]"));
    cmd = cmd.trimmed();

    qDebug() << "DesktopEntryModel: launching" << entry.name << ":" << cmd;

    QStringList args = QProcess::splitCommand(cmd);
    if (!args.isEmpty()) {
        QString program = args.takeFirst();
        QProcess::startDetached(program, args);
    }
}

void DesktopEntryModel::refresh()
{
    loadEntries();
}

// ═════════════════════════════════════════════════════════
// AppFilterModel — fuzzy scoring via FuzzyMatcher
// ═════════════════════════════════════════════════════════

AppFilterModel::AppFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setDynamicSortFilter(true);
}

void AppFilterModel::setSearchText(const QString &text)
{
    if (m_searchText == text)
        return;

    m_searchText = text;
    beginFilterChange();
    endFilterChange();
    sort(0);
    emit searchTextChanged();
    emit countChanged();
}

bool AppFilterModel::filterAcceptsRow(int sourceRow,
                                       const QModelIndex &sourceParent) const
{
    if (m_searchText.isEmpty())
        return true;

    auto model = sourceModel();
    auto idx = model->index(sourceRow, 0, sourceParent);

    QString name = model->data(idx, DesktopEntryModel::NameRole).toString();
    QString generic = model->data(idx, DesktopEntryModel::GenericNameRole).toString();
    QString comment = model->data(idx, DesktopEntryModel::CommentRole).toString();
    QString exec = model->data(idx, DesktopEntryModel::ExecRole).toString();
    QStringList keywords = model->data(idx, DesktopEntryModel::KeywordsRole).toStringList();

    if (FuzzyMatcher::matches(m_searchText, name))
        return true;
    if (FuzzyMatcher::matches(m_searchText, generic))
        return true;
    if (FuzzyMatcher::matches(m_searchText, comment))
        return true;

    QString execBase = exec.split('/').last().split(' ').first();
    if (FuzzyMatcher::matches(m_searchText, execBase))
        return true;

    for (const auto &kw : keywords) {
        if (FuzzyMatcher::matches(m_searchText, kw))
            return true;
    }

    return false;
}

bool AppFilterModel::lessThan(const QModelIndex &left,
                               const QModelIndex &right) const
{
    auto model = sourceModel();

    if (!m_searchText.isEmpty()) {
        auto fieldsFor = [&](const QModelIndex &idx) -> QStringList {
            return {
                model->data(idx, DesktopEntryModel::NameRole).toString(),
                model->data(idx, DesktopEntryModel::GenericNameRole).toString(),
                model->data(idx, DesktopEntryModel::CommentRole).toString(),
            };
        };
        int ls = FuzzyMatcher::bestScore(m_searchText, fieldsFor(left));
        int rs = FuzzyMatcher::bestScore(m_searchText, fieldsFor(right));
        if (ls != rs)
            return ls > rs;
    }

    int leftCount = model->data(left, DesktopEntryModel::LaunchCountRole).toInt();
    int rightCount = model->data(right, DesktopEntryModel::LaunchCountRole).toInt();
    if (leftCount != rightCount)
        return leftCount > rightCount;

    QString leftName = model->data(left, DesktopEntryModel::NameRole).toString();
    QString rightName = model->data(right, DesktopEntryModel::NameRole).toString();
    return leftName.toLower() < rightName.toLower();
}

void AppFilterModel::launch(int proxyIndex)
{
    auto srcIdx = mapToSource(index(proxyIndex, 0));
    auto model = qobject_cast<DesktopEntryModel*>(sourceModel());
    if (model)
        model->launch(srcIdx.row());
}
