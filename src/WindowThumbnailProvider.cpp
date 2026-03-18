#include "WindowThumbnailProvider.h"
#include "WayfireIPC.h"
#include "WindowTracker.h"

#include <QProcess>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QThread>

WindowThumbnailProvider::WindowThumbnailProvider(QObject *parent)
    : QQuickImageProvider(QQuickImageProvider::Image)

{
    m_refreshTimer.setInterval(1500);  // refresh every 1.5s
    connect(&m_refreshTimer, &QTimer::timeout,
            this, &WindowThumbnailProvider::refreshCaptures);
}

void WindowThumbnailProvider::setWayfireIPC(WayfireIPC *ipc) { m_ipc = ipc; }
void WindowThumbnailProvider::setWindowTracker(WindowTracker *tracker) { m_tracker = tracker; }

QImage WindowThumbnailProvider::requestImage(const QString &id, QSize *size,
                                              const QSize &requestedSize)
{
    QMutexLocker lock(&m_imageMutex);

    // Try cached image first
    if (m_images.contains(id)) {
        QImage img = m_images.value(id);
        if (size) *size = img.size();
        if (requestedSize.isValid() && !requestedSize.isNull())
            return img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return img;
    }

    // Try file on disk as fallback
    QString path = QDir::tempPath() + "/chi-thumb-" + id + ".png";
    QImage img(path);
    if (!img.isNull()) {
        if (size) *size = img.size();
        if (requestedSize.isValid() && !requestedSize.isNull())
            return img.scaled(requestedSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        return img;
    }

    // Return empty
    QImage fallback(240, 160, QImage::Format_ARGB32);
    fallback.fill(Qt::transparent);
    if (size) *size = fallback.size();
    return fallback;
}

void WindowThumbnailProvider::startCapture(const QString &appId)
{
    m_activeAppId = appId;
    captureAllForApp(appId);
    m_refreshTimer.start();
}

void WindowThumbnailProvider::stopCapture()
{
    m_refreshTimer.stop();
    m_activeAppId.clear();
}

void WindowThumbnailProvider::captureAllForApp(const QString &appId)
{
    auto geos = getViewGeometries(appId);
    for (int i = 0; i < geos.size(); ++i)
        captureRegion(geos[i], i);
}

void WindowThumbnailProvider::refreshCaptures()
{
    if (m_activeAppId.isEmpty()) return;
    captureAllForApp(m_activeAppId);
}

QList<WindowThumbnailProvider::ViewGeometry>
WindowThumbnailProvider::getViewGeometries(const QString &appId)
{
    QList<ViewGeometry> result;
    if (!m_tracker) return result;

    // Use Wayfire IPC to get view geometry if available
    if (m_ipc) {
        QJsonObject reply = m_ipc->callSync("window-rules/list-views", QJsonObject());
        if (reply.contains("result") || reply.contains("view-id")) {
            // Wayfire returns array at top level or under a key
            QJsonArray views;
            if (reply.contains("result"))
                views = reply.value("result").toArray();
            else if (reply.value("ok").toBool()) {
                // Try the full reply as the data
                views = QJsonDocument::fromJson(
                    QJsonDocument(reply).toJson()).array();
            }

            for (const auto &v : views) {
                QJsonObject vo = v.toObject();
                if (vo.value("app-id").toString() == appId) {
                    ViewGeometry g;
                    QJsonObject geo = vo.value("geometry").toObject();
                    g.x = geo.value("x").toInt();
                    g.y = geo.value("y").toInt();
                    g.w = geo.value("width").toInt();
                    g.h = geo.value("height").toInt();
                    g.viewId = vo.value("id").toInt(-1);
                    g.outputName = vo.value("output-name").toString();
                    if (g.w > 0 && g.h > 0)
                        result.append(g);
                }
            }
        }
    }

    // Fallback: just count windows from tracker, no geometry
    if (result.isEmpty()) {
        for (int i = 0; i < m_tracker->rowCount(); ++i) {
            QString aid = m_tracker->data(m_tracker->index(i),
                              WindowTracker::AppIdRole).toString();
            if (aid == appId) {
                ViewGeometry g;
                g.viewId = i;
                result.append(g);
            }
        }
    }

    return result;
}

void WindowThumbnailProvider::captureRegion(const ViewGeometry &geo, int index)
{
    QString key = m_activeAppId + "-" + QString::number(index);
    QString path = QDir::tempPath() + "/chi-thumb-" + key + ".png";

    // If we have geometry, use grim with region capture at half scale
    if (geo.w > 0 && geo.h > 0) {
        QString region = QString("%1,%2 %3x%4")
            .arg(geo.x).arg(geo.y).arg(geo.w).arg(geo.h);

        QStringList args;
        args << "-g" << region
             << "-t" << "png"
             << "-l" << "0"       // fastest compression
             << "-s" << "0.5"     // half scale for thumbnails
             << path;

        // Add output if known
        if (!geo.outputName.isEmpty())
            args.prepend(geo.outputName), args.prepend("-o");

        QProcess *proc = new QProcess(this);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, [this, proc, key, path](int exitCode, QProcess::ExitStatus) {
            proc->deleteLater();
            if (exitCode == 0) {
                QImage img(path);
                if (!img.isNull()) {
                    QMutexLocker lock(&m_imageMutex);
                    m_images[key] = img;
                }
            }
        });
        proc->start("grim", args);
    }
}
