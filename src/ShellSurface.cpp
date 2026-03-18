#include "ShellSurface.h"

#include <QQmlContext>
#include <QGuiApplication>
#include <qpa/qplatformnativeinterface.h>
#include <wayland-client.h>
#include <QDebug>

ShellSurface::ShellSurface(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    m_view = new QQuickView(engine, nullptr);
    m_view->setResizeMode(QQuickView::SizeRootObjectToView);
    m_view->setColor(Qt::transparent);
}

ShellSurface::~ShellSurface()
{
    delete m_view;
}

void ShellSurface::setLayer(Layer layer)         { m_layer = layer; }
void ShellSurface::setAnchors(LayerShellQt::Window::Anchors a) { m_anchors = a; }
void ShellSurface::setExclusiveZone(int zone)    { m_exclusiveZone = zone; }
void ShellSurface::setKeyboardMode(KeyboardMode m) { m_keyboardMode = m; }
void ShellSurface::setScope(const QString &scope) { m_scope = scope; }

void ShellSurface::setMargins(int top, int right, int bottom, int left)
{
    m_margins = QMargins(left, top, right, bottom);
}

void ShellSurface::setSize(int w, int h)
{
    m_size = QSize(w, h);
    m_view->resize(w, h);
}

void ShellSurface::setSource(const QUrl &url)
{
    m_view->setSource(url);
}

void ShellSurface::setScreen(QScreen *screen)
{
    if (screen)
        m_view->setScreen(screen);
}

void ShellSurface::setContextProperty(const QString &name, QObject *object)
{
    m_view->rootContext()->setContextProperty(name, object);
}

void ShellSurface::setContextProperty(const QString &name, const QVariant &value)
{
    m_view->rootContext()->setContextProperty(name, value);
}

void ShellSurface::configureLayerShell()
{
    if (m_layerConfigured)
        return;

    m_view->create();

    auto *lsw = LayerShellQt::Window::get(m_view);
    if (!lsw) {
        qWarning() << "ShellSurface: LayerShellQt::Window::get() returned null for" << m_scope;
        return;
    }

    lsw->setLayer(static_cast<LayerShellQt::Window::Layer>(m_layer));
    lsw->setAnchors(m_anchors);
    lsw->setExclusiveZone(m_exclusiveZone);
    lsw->setKeyboardInteractivity(
        static_cast<LayerShellQt::Window::KeyboardInteractivity>(m_keyboardMode));

    if (!m_scope.isEmpty())
        lsw->setScope(m_scope);

    if (!m_margins.isNull())
        lsw->setMargins(m_margins);

    if (m_size.isValid())
        m_view->resize(m_size);

    m_layerConfigured = true;
}

void ShellSurface::show()
{
    configureLayerShell();
    m_view->show();
    emit visibleChanged();
}

void ShellSurface::hide()
{
    m_view->hide();
    emit visibleChanged();
}

bool ShellSurface::isVisible() const
{
    return m_view && m_view->isVisible();
}

void ShellSurface::setVisible(bool visible)
{
    if (visible) show();
    else hide();
}

static void flushWayland()
{
    auto *native = QGuiApplication::platformNativeInterface();
    if (!native) return;
    auto *display = static_cast<wl_display *>(
        native->nativeResourceForIntegration("wl_display"));
    if (display)
        wl_display_flush(display);
}

void ShellSurface::setInputRegion(const QRect &rect)
{
    if (!m_view) return;
    m_view->setMask(QRegion(rect));
    flushWayland();
}

void ShellSurface::setFullInputRegion()
{
    if (!m_view) return;
    // Set mask to FULL window size — this means entire surface receives input
    m_view->setMask(QRegion(0, 0, m_view->width(), m_view->height()));
    flushWayland();
}

void ShellSurface::clearInputRegion()
{
    if (!m_view) return;
    // IMPORTANT: QRegion() = empty region = NO input at all!
    // To CLEAR the mask (accept all input), we must set a region covering
    // the entire window, NOT an empty QRegion.
    m_view->setMask(QRegion(0, 0, m_view->width(), m_view->height()));
    flushWayland();
}
