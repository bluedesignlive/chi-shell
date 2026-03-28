#pragma once

#include <QQuickPaintedItem>
#include <QFont>
#include <QFontMetricsF>
#include <QTimer>

namespace chiterm {

class TerminalSession;

class TerminalWidget : public QQuickPaintedItem {
    Q_OBJECT

    // Use QObject* for the session property so QML can pass it freely
    Q_PROPERTY(QObject* session READ sessionObject WRITE setSessionObject NOTIFY sessionChanged)
    Q_PROPERTY(int termRows READ termRows NOTIFY sizeChanged)
    Q_PROPERTY(int termCols READ termCols NOTIFY sizeChanged)
    Q_PROPERTY(qreal cellWidth READ cellWidth NOTIFY fontMetricsChanged)
    Q_PROPERTY(qreal cellHeight READ cellHeight NOTIFY fontMetricsChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontMetricsChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontMetricsChanged)
    Q_PROPERTY(int padding READ padding WRITE setPadding NOTIFY paddingChanged)

    // Theme colors (set from QML)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor cursorColor READ cursorColor WRITE setCursorColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY colorsChanged)

public:
    explicit TerminalWidget(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

    // ── Properties ──────────────────────────────────────────
    TerminalSession *session() const { return m_session; }
    void setSession(TerminalSession *s);

    QObject *sessionObject() const;
    void setSessionObject(QObject *obj);

    int termRows() const { return m_rows; }
    int termCols() const { return m_cols; }
    qreal cellWidth() const { return m_cellWidth; }
    qreal cellHeight() const { return m_cellHeight; }

    QString fontFamily() const { return m_fontFamily; }
    void setFontFamily(const QString &family);
    int fontSize() const { return m_fontSize; }
    void setFontSize(int size);
    int padding() const { return m_padding; }
    void setPadding(int p);

    QColor backgroundColor() const { return m_bgColor; }
    void setBackgroundColor(const QColor &c);
    QColor foregroundColor() const { return m_fgColor; }
    void setForegroundColor(const QColor &c);
    QColor cursorColor() const { return m_cursorColor; }
    void setCursorColor(const QColor &c);
    QColor selectionColor() const { return m_selectionColor; }
    void setSelectionColor(const QColor &c);

signals:
    void sessionChanged();
    void sizeChanged(int rows, int cols);
    void fontMetricsChanged();
    void paddingChanged();
    void colorsChanged();

protected:
    void geometryChange(const QRectF &newGeometry,
                        const QRectF &oldGeometry) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void recalculateGrid();
    void updateFontMetrics();

    TerminalSession *m_session = nullptr;

    // Font
    QString m_fontFamily = QStringLiteral("Monospace");
    int     m_fontSize   = 13;
    QFont   m_font;
    qreal   m_cellWidth  = 8.0;
    qreal   m_cellHeight = 16.0;
    int     m_padding    = 4;

    // Grid dimensions
    int m_rows = 24;
    int m_cols = 80;

    // Colors
    QColor m_bgColor        = QColor(0x1d, 0x1b, 0x20);
    QColor m_fgColor        = QColor(0xe6, 0xe1, 0xe5);
    QColor m_cursorColor    = QColor(0x67, 0x3a, 0xb7);
    QColor m_selectionColor = QColor(0x67, 0x3a, 0xb7, 80);

    // Cursor blink
    QTimer  m_cursorTimer;
    bool    m_cursorVisible = true;
};

} // namespace chiterm
