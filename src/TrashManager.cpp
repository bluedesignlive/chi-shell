#include "TrashManager.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QProcess>
#include <QStandardPaths>
#include <QDebug>
#include <algorithm>

// ═════════════════════════════════════════════════════════
// TrashListModel
// ═════════════════════════════════════════════════════════

TrashListModel::TrashListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TrashListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

QVariant TrashListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size())
        return {};
    const auto &e = m_entries.at(index.row());
    switch (role) {
    case TrashNameRole:    return e.trashName;
    case DisplayNameRole:  return QFileInfo(e.trashName).fileName();
    case OriginalPathRole: return e.originalPath;
    case DeletionDateRole: return e.deletionDate;
    case SizeRole:         return e.size;
    case SizeTextRole:     return humanSize(e.size);
    case IsDirRole:        return e.isDir;
    case IconNameRole:     return iconForFile(e.trashName, e.isDir);
    }
    return {};
}

QHash<int, QByteArray> TrashListModel::roleNames() const
{
    return {
        { TrashNameRole,    "trashName" },
        { DisplayNameRole,  "displayName" },
        { OriginalPathRole, "originalPath" },
        { DeletionDateRole, "deletionDate" },
        { SizeRole,         "fileSize" },
        { SizeTextRole,     "sizeText" },
        { IsDirRole,        "isDir" },
        { IconNameRole,     "iconName" },
    };
}

void TrashListModel::refresh(const QString &filesPath, const QString &infoPath)
{
    QDir filesDir(filesPath);
    QDir infoDir(infoPath);

    QVector<TrashEntry> entries;

    for (const auto &fi : filesDir.entryInfoList(
             QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System,
             QDir::Time)) {
        TrashEntry e;
        e.trashName = fi.fileName();
        e.isDir = fi.isDir();
        e.size = fi.isDir() ? 0 : fi.size();

        // Read .trashinfo
        QString infoFile = infoPath + "/" + e.trashName + ".trashinfo";
        QFile f(infoFile);
        if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&f);
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (line.startsWith("Path="))
                    e.originalPath = line.mid(5);
                else if (line.startsWith("DeletionDate="))
                    e.deletionDate = line.mid(13);
            }
        }

        if (e.originalPath.isEmpty())
            e.originalPath = fi.absoluteFilePath();
        if (e.deletionDate.isEmpty())
            e.deletionDate = fi.lastModified().toString(Qt::ISODate);

        entries.append(e);
    }

    beginResetModel();
    m_entries = entries;
    endResetModel();
    emit countChanged();
}

QString TrashListModel::humanSize(qint64 bytes)
{
    if (bytes < 0) return "?";
    if (bytes == 0) return "folder";
    if (bytes < 1024) return QString::number(bytes) + " B";
    if (bytes < 1024 * 1024) return QString::number(bytes / 1024) + " KB";
    if (bytes < 1024LL * 1024 * 1024)
        return QString::number(bytes / (1024 * 1024)) + " MB";
    return QString::number(bytes / (1024LL * 1024 * 1024)) + " GB";
}

QString TrashListModel::iconForFile(const QString &name, bool isDir)
{
    if (isDir) return "folder";
    QString ext = QFileInfo(name).suffix().toLower();
    if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "svg"
        || ext == "gif" || ext == "webp" || ext == "bmp")
        return "image";
    if (ext == "mp4" || ext == "mkv" || ext == "avi" || ext == "mov"
        || ext == "webm")
        return "movie";
    if (ext == "mp3" || ext == "flac" || ext == "ogg" || ext == "wav"
        || ext == "m4a")
        return "music_note";
    if (ext == "pdf") return "picture_as_pdf";
    if (ext == "txt" || ext == "md" || ext == "log") return "description";
    if (ext == "zip" || ext == "tar" || ext == "gz" || ext == "xz"
        || ext == "7z" || ext == "rar")
        return "archive";
    if (ext == "sh" || ext == "py" || ext == "cpp" || ext == "h"
        || ext == "js" || ext == "rs" || ext == "go" || ext == "qml")
        return "code";
    if (ext == "desktop") return "apps";
    return "draft";
}

// ═════════════════════════════════════════════════════════
// TrashManager
// ═════════════════════════════════════════════════════════

TrashManager::TrashManager(QObject *parent)
    : QObject(parent)
{
    ensureTrashDirs();

    connect(&m_watcher, &QFileSystemWatcher::directoryChanged,
            this, &TrashManager::refresh);

    QString filesDir = trashFilesPath();
    if (QDir(filesDir).exists())
        m_watcher.addPath(filesDir);

    refresh();
}

QString TrashManager::trashFilesPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/Trash/files";
}

QString TrashManager::trashInfoPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
           + "/Trash/info";
}

void TrashManager::ensureTrashDirs()
{
    QDir().mkpath(trashFilesPath());
    QDir().mkpath(trashInfoPath());
}

void TrashManager::refresh()
{
    m_listModel.refresh(trashFilesPath(), trashInfoPath());
    int newCount = m_listModel.count();
    if (newCount != m_count) {
        m_count = newCount;
        emit countChanged();
    }
}

void TrashManager::openTrash()
{
    if (!QProcess::startDetached("xdg-open", {"trash:///"}))
        QProcess::startDetached("xdg-open", {trashFilesPath()});
}

void TrashManager::emptyTrash()
{
    QDir filesDir(trashFilesPath());
    QDir infoDir(trashInfoPath());

    for (const auto &entry : filesDir.entryInfoList(
             QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System)) {
        if (entry.isDir())
            QDir(entry.absoluteFilePath()).removeRecursively();
        else
            QFile::remove(entry.absoluteFilePath());
    }

    for (const auto &entry : infoDir.entryInfoList({"*.trashinfo"}, QDir::Files))
        QFile::remove(entry.absoluteFilePath());

    m_restoreStack.clear();
    refresh();
    qDebug() << "TrashManager: emptied trash";
}

bool TrashManager::moveToTrash(const QString &filePath)
{
    QFileInfo fi(filePath);
    if (!fi.exists()) return false;

    ensureTrashDirs();

    QString baseName = fi.fileName();
    QString trashName = baseName;
    QString filesPath = trashFilesPath();
    int counter = 1;
    while (QFile::exists(filesPath + "/" + trashName)) {
        trashName = fi.completeBaseName()
                    + "." + QString::number(counter++)
                    + (fi.suffix().isEmpty() ? "" : "." + fi.suffix());
    }

    QString infoFilePath = trashInfoPath() + "/" + trashName + ".trashinfo";
    QFile infoFile(infoFilePath);
    if (!infoFile.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&infoFile);
    out << "[Trash Info]\n";
    out << "Path=" << fi.absoluteFilePath() << "\n";
    out << "DeletionDate=" << QDateTime::currentDateTime().toString(Qt::ISODate) << "\n";
    infoFile.close();

    QString dest = filesPath + "/" + trashName;
    bool ok;
    if (fi.isDir())
        ok = QDir().rename(fi.absoluteFilePath(), dest);
    else
        ok = QFile::rename(fi.absoluteFilePath(), dest);

    if (!ok) {
        QFile::remove(infoFilePath);
        return false;
    }

    m_restoreStack.append(trashName);
    // refresh() will fire from watcher
    qDebug() << "TrashManager: trashed" << fi.absoluteFilePath() << "as" << trashName;
    return true;
}

bool TrashManager::restoreItem(int index)
{
    // Get trash name from model
    auto idx = m_listModel.index(index);
    if (!idx.isValid()) return false;

    QString trashName = m_listModel.data(idx, TrashListModel::TrashNameRole).toString();
    QString originalPath = m_listModel.data(idx, TrashListModel::OriginalPathRole).toString();

    if (trashName.isEmpty() || originalPath.isEmpty()) return false;

    QString trashedFilePath = trashFilesPath() + "/" + trashName;
    QString infoFilePath = trashInfoPath() + "/" + trashName + ".trashinfo";

    QDir().mkpath(QFileInfo(originalPath).absolutePath());

    bool ok;
    QFileInfo fi(trashedFilePath);
    if (fi.isDir())
        ok = QDir().rename(trashedFilePath, originalPath);
    else
        ok = QFile::rename(trashedFilePath, originalPath);

    if (ok) {
        QFile::remove(infoFilePath);
        qDebug() << "TrashManager: restored" << trashName << "to" << originalPath;
        return true;
    }
    return false;
}

bool TrashManager::restoreLast()
{
    if (m_restoreStack.isEmpty()) {
        // Fall back to most recent item
        if (m_listModel.count() > 0)
            return restoreItem(0);  // model sorted by time, 0 = newest
        return false;
    }

    QString trashName = m_restoreStack.takeLast();

    // Find it in the model
    for (int i = 0; i < m_listModel.count(); ++i) {
        auto idx = m_listModel.index(i);
        if (m_listModel.data(idx, TrashListModel::TrashNameRole).toString() == trashName)
            return restoreItem(i);
    }
    return false;
}

bool TrashManager::deleteItem(int index)
{
    auto idx = m_listModel.index(index);
    if (!idx.isValid()) return false;

    QString trashName = m_listModel.data(idx, TrashListModel::TrashNameRole).toString();
    bool isDir = m_listModel.data(idx, TrashListModel::IsDirRole).toBool();

    QString trashedFilePath = trashFilesPath() + "/" + trashName;
    QString infoFilePath = trashInfoPath() + "/" + trashName + ".trashinfo";

    bool ok;
    if (isDir)
        ok = QDir(trashedFilePath).removeRecursively();
    else
        ok = QFile::remove(trashedFilePath);

    if (ok) {
        QFile::remove(infoFilePath);
        qDebug() << "TrashManager: permanently deleted" << trashName;
        return true;
    }
    return false;
}

QString TrashManager::trashItemOriginalPath(int index) const
{
    auto idx = m_listModel.index(index);
    if (!idx.isValid()) return {};
    return m_listModel.data(idx, TrashListModel::OriginalPathRole).toString();
}
