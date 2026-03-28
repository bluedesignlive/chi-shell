#include "TerminalWidget.h"
#include "session/TerminalSession.h"
#include "terminal/VtBridge.h"

#include <QPainter>
#include <QKeyEvent>

namespace chiterm {

TerminalWidget::TerminalWidget(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFocus(true);

    // Cursor blink timer
    m_cursorTimer.setInterval(530);
    connect(&m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_cursorTimer.start();

    updateFontMetrics();
}

// ─── Session binding ───────────────────────────────────────

QObject *TerminalWidget::sessionObject() const
{
    return m_session;
}

void TerminalWidget::setSessionObject(QObject *obj)
{
    auto *s = qobject_cast<TerminalSession *>(obj);
    setSession(s);
}

void TerminalWidget::setSession(TerminalSession *s)
{
    if (m_session == s)
        return;

    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);

    m_session = s;

    if (m_session) {
        connect(m_session, &TerminalSession::contentChanged,
                this, [this]() { update(); });
        recalculateGrid();
    }

    emit sessionChanged();
    update();
}

// ─── Font handling ─────────────────────────────────────────

void TerminalWidget::setFontFamily(const QString &family)
{
    if (m_fontFamily == family)
        return;
    m_fontFamily = family;
    updateFontMetrics();
    recalculateGrid();
    update();
}

void TerminalWidget::setFontSize(int size)
{
    size = qBound(6, size, 72);
    if (m_fontSize == size)
        return;
    m_fontSize = size;
    updateFontMetrics();
    recalculateGrid();
    update();
}

void TerminalWidget::setPadding(int p)
{
    if (m_padding == p)
        return;
    m_padding = p;
    recalculateGrid();
    emit paddingChanged();
    update();
}

void TerminalWidget::updateFontMetrics()
{
    m_font = QFont(m_fontFamily, m_fontSize);
    m_font.setStyleHint(QFont::Monospace);
    m_font.setFixedPitch(true);

    QFontMetricsF fm(m_font);
    m_cellWidth  = fm.horizontalAdvance(QChar('M'));
    m_cellHeight = fm.height();

    if (m_cellWidth < 1.0)  m_cellWidth = 8.0;
    if (m_cellHeight < 1.0) m_cellHeight = 16.0;

    emit fontMetricsChanged();
}

// ─── Colors ────────────────────────────────────────────────

void TerminalWidget::setBackgroundColor(const QColor &c) {
    if (m_bgColor != c) { m_bgColor = c; emit colorsChanged(); update(); }
}
void TerminalWidget::setForegroundColor(const QColor &c) {
    if (m_fgColor != c) { m_fgColor = c; emit colorsChanged(); update(); }
}
void TerminalWidget::setCursorColor(const QColor &c) {
    if (m_cursorColor != c) { m_cursorColor = c; emit colorsChanged(); update(); }
}
void TerminalWidget::setSelectionColor(const QColor &c) {
    if (m_selectionColor != c) { m_selectionColor = c; emit colorsChanged(); update(); }
}

// ─── Grid calculation ──────────────────────────────────────

void TerminalWidget::geometryChange(const QRectF &newGeometry,
                                     const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    recalculateGrid();
}

void TerminalWidget::recalculateGrid()
{
    qreal availW = width()  - (2.0 * m_padding);
    qreal availH = height() - (2.0 * m_padding);

    int newCols = qMax(1, static_cast<int>(availW / m_cellWidth));
    int newRows = qMax(1, static_cast<int>(availH / m_cellHeight));

    if (newCols != m_cols || newRows != m_rows) {
        m_cols = newCols;
        m_rows = newRows;

        if (m_session)
            m_session->resize(m_rows, m_cols);

        emit sizeChanged(m_rows, m_cols);
    }
}

// ─── Keyboard input ────────────────────────────────────────

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_session) {
        event->ignore();
        return;
    }

    m_cursorVisible = true;
    m_cursorTimer.start();

    m_session->sendKey(event->key(), event->modifiers(), event->text());
    event->accept();
}

void TerminalWidget::keyReleaseEvent(QKeyEvent *event)
{
    event->accept();
}

// ─── Paint ─────────────────────────────────────────────────

void TerminalWidget::paint(QPainter *painter)
{
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // Background
    painter->fillRect(QRectF(0, 0, width(), height()), m_bgColor);

    if (!m_session)
        return;

    VtBridge *vt = m_session->vtBridge();
    if (!vt)
        return;

    const qreal padX = m_padding;
    const qreal padY = m_padding;

    painter->setFont(m_font);

    QFontMetricsF fm(m_font);
    const qreal ascent = fm.ascent();

    int vtRows = qMin(m_rows, vt->rows());
    int vtCols = qMin(m_cols, vt->cols());

    for (int row = 0; row < vtRows; ++row) {
        qreal y = padY + row * m_cellHeight;

        for (int col = 0; col < vtCols; ++col) {
            Cell cell = vt->cellAt(row, col);

            if (cell.width == 0)
                continue;

            qreal x = padX + col * m_cellWidth;
            qreal w = m_cellWidth * cell.width;

            QColor fg = cell.fg.isValid() ? cell.fg : m_fgColor;
            QColor bg = cell.bg.isValid() ? cell.bg : QColor();

            if (cell.attrs.reverse) {
                std::swap(fg, bg);
                if (!bg.isValid()) bg = m_fgColor;
                if (!fg.isValid()) fg = m_bgColor;
            }

            if (bg.isValid() && bg != m_bgColor)
                painter->fillRect(QRectF(x, y, w, m_cellHeight), bg);

            if (cell.text != QStringLiteral(" ") && !cell.text.isEmpty()
                && !cell.attrs.invisible) {
                QFont cellFont = m_font;
                if (cell.attrs.bold)   cellFont.setBold(true);
                if (cell.attrs.italic) cellFont.setItalic(true);
                if (cellFont != m_font) painter->setFont(cellFont);

                if (cell.attrs.faint) fg.setAlphaF(0.5);

                painter->setPen(fg);
                painter->drawText(QPointF(x, y + ascent), cell.text);

                if (cellFont != m_font) painter->setFont(m_font);
            }

            if (cell.attrs.underline) {
                painter->setPen(fg);
                qreal uy = y + m_cellHeight - 1.5;
                painter->drawLine(QPointF(x, uy), QPointF(x + w, uy));
            }

            if (cell.attrs.strikethrough) {
                painter->setPen(fg);
                qreal sy = y + m_cellHeight / 2.0;
                painter->drawLine(QPointF(x, sy), QPointF(x + w, sy));
            }
        }
    }

    // Cursor
    CursorState cs = vt->cursor();
    if (cs.visible && cs.row >= 0 && cs.row < vtRows
        && cs.col >= 0 && cs.col < vtCols) {
        qreal cx = padX + cs.col * m_cellWidth;
        qreal cy = padY + cs.row * m_cellHeight;

        if (m_cursorVisible) {
            switch (cs.shape) {
            case CursorShape::Block:
                painter->fillRect(QRectF(cx, cy, m_cellWidth, m_cellHeight),
                                  m_cursorColor);
                {
                    Cell cc = vt->cellAt(cs.row, cs.col);
                    if (!cc.text.isEmpty() && cc.text != QStringLiteral(" ")) {
                        painter->setPen(m_bgColor);
                        painter->drawText(QPointF(cx, cy + ascent), cc.text);
                    }
                }
                break;
            case CursorShape::Beam:
                painter->fillRect(QRectF(cx, cy, 2.0, m_cellHeight),
                                  m_cursorColor);
                break;
            case CursorShape::Underline:
                painter->fillRect(QRectF(cx, cy + m_cellHeight - 2.0,
                                          m_cellWidth, 2.0),
                                  m_cursorColor);
                break;
            }
        }
    }
}

} // namespace chiterm
