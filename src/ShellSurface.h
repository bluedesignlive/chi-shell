#ifndef SHELLSURFACE_H
#define SHELLSURFACE_H

#include <QObject>
#include <QQuickView>
#include <QQmlEngine>
#include <QScreen>
#include <QRegion>

#include <LayerShellQt/Window>

class ShellSurface : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)

public:
    enum Layer {
        Background = LayerShellQt::Window::LayerBackground,
        Bottom     = LayerShellQt::Window::LayerBottom,
        Top        = LayerShellQt::Window::LayerTop,
        Overlay    = LayerShellQt::Window::LayerOverlay
    };
    Q_ENUM(Layer)

    enum KeyboardMode {
        KeyboardNone      = LayerShellQt::Window::KeyboardInteractivityNone,
        KeyboardExclusive = LayerShellQt::Window::KeyboardInteractivityExclusive,
        KeyboardOnDemand  = LayerShellQt::Window::KeyboardInteractivityOnDemand
    };
    Q_ENUM(KeyboardMode)

    explicit ShellSurface(QQmlEngine *engine, QObject *parent = nullptr);
    ~ShellSurface() override;

    void setLayer(Layer layer);
    void setAnchors(LayerShellQt::Window::Anchors anchors);
    Q_INVOKABLE void setExclusiveZone(int zone);
    void setKeyboardMode(KeyboardMode mode);
    void setMargins(int top, int right, int bottom, int left);
    void setSize(int width, int height);
    void setSource(const QUrl &qmlUrl);
    void setScreen(QScreen *screen);
    void setScope(const QString &scope);

    void setContextProperty(const QString &name, QObject *object);
    void setContextProperty(const QString &name, const QVariant &value);

    void show();
    void hide();
    bool isVisible() const;
    void setVisible(bool visible);

    QQuickView *view() const { return m_view; }

    // Input region management for layer-shell popups
    Q_INVOKABLE void setInputRegion(const QRect &rect);
    Q_INVOKABLE void setFullInputRegion();
    Q_INVOKABLE void clearInputRegion();

signals:
    void visibleChanged();

private:
    void configureLayerShell();

    QQuickView *m_view = nullptr;
    QQmlEngine *m_engine = nullptr;

    Layer m_layer = Top;
    LayerShellQt::Window::Anchors m_anchors;
    int m_exclusiveZone = 0;
    KeyboardMode m_keyboardMode = KeyboardNone;
    QMargins m_margins;
    QSize m_size;
    QString m_scope;
    bool m_layerConfigured = false;
};

#endif // SHELLSURFACE_H
