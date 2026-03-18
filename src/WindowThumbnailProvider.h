#ifndef WINDOWTHUMBNAILPROVIDER_H
#define WINDOWTHUMBNAILPROVIDER_H

#include <QObject>
#include <QQuickImageProvider>
#include <QImage>
#include <QHash>
#include <QTimer>
#include <QMutex>

class WayfireIPC;
class WindowTracker;

/*
 * Captures per-window thumbnails using:
 *   1. Wayfire IPC "window-rules/list-views" → get window geometry
 *   2. grim -g "x,y wxh" -t png -l 0 -s 0.5 /tmp/chi-thumb-{id}.png
 *      (region screencopy at half scale for performance)
 *   3. QQuickImageProvider serves the cached images to QML
 *
 * The capture is driven by startCapture(viewId)/stopCapture(viewId)
 * and refreshes on a timer while active.
 */
class WindowThumbnailProvider : public QQuickImageProvider
{
    Q_OBJECT

public:
    explicit WindowThumbnailProvider(QObject *parent = nullptr);

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

    void setWayfireIPC(WayfireIPC *ipc);
    void setWindowTracker(WindowTracker *tracker);

    // Start/stop live capture for a specific app
    Q_INVOKABLE void startCapture(const QString &appId);
    Q_INVOKABLE void stopCapture();
    Q_INVOKABLE void captureAllForApp(const QString &appId);

private slots:
    void refreshCaptures();

private:
    struct ViewGeometry {
        int x = 0, y = 0, w = 0, h = 0;
        QString outputName;
        int viewId = -1;
    };

    QList<ViewGeometry> getViewGeometries(const QString &appId);
    void captureRegion(const ViewGeometry &geo, int index);

    WayfireIPC    *m_ipc     = nullptr;
    WindowTracker *m_tracker = nullptr;

    QString        m_activeAppId;
    QTimer         m_refreshTimer;
    QMutex         m_imageMutex;
    QHash<QString, QImage> m_images;  // "appId-index" → image
};

#endif // WINDOWTHUMBNAILPROVIDER_H
