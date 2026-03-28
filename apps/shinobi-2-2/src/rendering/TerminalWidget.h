#pragma once

#include <QQuickPaintedItem>
#include <QFont>
#include <QFontMetricsF>
#include <QTimer>

namespace chiterm {

class TerminalSession;

class TerminalWidget : public QQuickPaintedItem {
    Q_OBJECT

    Q_PROPERTY(QObject* session READ sessionObject WRITE setSessionObject NOTIFY sessionChanged)
    Q_PROPERTY(int termRows READ termRows NOTIFY sizeChanged)
    Q_PROPERTY(int termCols READ termCols NOTIFY sizeChanged)
    Q_PROPERTY(qreal cellWidth READ cellWidth NOTIFY fontMetricsChanged)
    Q_PROPERTY(qreal cellHeight READ cellHeight NOTIFY fontMetricsChanged)
    Q_PROPERTY(QString fontFamily READ fontFamily WRITE setFontFamily NOTIFY fontMetricsChanged)
    Q_PROPERTY(int fontSize READ fontSize WRITE setFontSize NOTIFY fontMetricsChanged)
    Q_PROPERTY(int padding READ padding WRITE setPadding NOTIFY paddingChanged)

    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor foregroundColor READ foregroundColor WRITE setForegroundColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor cursorColor READ cursorColor WRITE setCursorColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor selectionColor READ selectionColor WRITE setSelectionColor NOTIFY colorsChanged)

public:
    explicit TerminalWidget(QQuickItem *parent = nullptr);

    void paint(QPainter *painter) override;

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
    void syncThemeToVt();

    TerminalSession *m_session = nullptr;

    QString m_fontFamily = QStringLiteral("Monospace");
    int     m_fontSize   = 13;
    QFont   m_font;
    qreal   m_cellWidth  = 8.0;
    qreal   m_cellHeight = 16.0;
    int     m_padding    = 8;

    int m_rows = 0;
    int m_cols = 0;

    QColor m_bgColor        = QColor(0x0d, 0x0b, 0x0f);
    QColor m_fgColor        = QColor(0xe7, 0xe0, 0xe8);
    QColor m_cursorColor    = QColor(0xd3, 0xbc, 0xfd);
    QColor m_selectionColor = QColor(0x4f, 0x3d, 0x74, 100);

    QTimer m_cursorTimer;
    bool   m_cursorVisible = true;
};

} // namespace chiterm
