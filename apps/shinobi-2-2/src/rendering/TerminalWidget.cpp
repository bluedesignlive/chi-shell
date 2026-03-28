#include "TerminalWidget.h"
#include "session/TerminalSession.h"
#include "terminal/VtBridge.h"
#include "Log.h"

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
    setAntialiasing(true);
    setFillColor(Qt::transparent);
    setOpaquePainting(true);

    m_cursorTimer.setInterval(530);
    connect(&m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_cursorTimer.start();

    updateFontMetrics();
    qCDebug(logRender) << "TerminalWidget created";
}

// ─── Session ───────────────────────────────────────────────

QObject *TerminalWidget::sessionObject() const { return m_session; }

void TerminalWidget::setSessionObject(QObject *obj)
{
    setSession(qobject_cast<TerminalSession *>(obj));
}

void TerminalWidget::setSession(TerminalSession *s)
{
    if (m_session == s) return;

    if (m_session)
        disconnect(m_session, nullptr, this, nullptr);

    m_session = s;

    if (m_session) {
        connect(m_session, &TerminalSession::contentChanged,
                this, [this]() { update(); });
        syncThemeToVt();
        recalculateGrid();
        qCDebug(logRender) << "Session attached — grid:"
                           << m_cols << "x" << m_rows;
    }

    emit sessionChanged();
    update();
}

// ─── Font ──────────────────────────────────────────────────

void TerminalWidget::setFontFamily(const QString &family)
{
    if (m_fontFamily == family) return;
    m_fontFamily = family;
    updateFontMetrics();
    recalculateGrid();
    update();
}

void TerminalWidget::setFontSize(int size)
{
    size = qBound(6, size, 72);
    if (m_fontSize == size) return;
    m_fontSize = size;
    updateFontMetrics();
    recalculateGrid();
    update();
}

void TerminalWidget::setPadding(int p)
{
    if (m_padding == p) return;
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

    qCDebug(logRender) << "Font:" << m_fontFamily << m_fontSize << "pt"
                       << "cell:" << m_cellWidth << "x" << m_cellHeight;
    emit fontMetricsChanged();
}

// ─── Colors + Theme sync ───────────────────────────────────

void TerminalWidget::syncThemeToVt()
{
    if (!m_session || !m_session->vtBridge())
        return;
    m_session->vtBridge()->setDefaultColors(m_fgColor, m_bgColor);
}

void TerminalWidget::setBackgroundColor(const QColor &c) {
    if (m_bgColor == c) return;
    m_bgColor = c;
    syncThemeToVt();
    emit colorsChanged();
    update();
}

void TerminalWidget::setForegroundColor(const QColor &c) {
    if (m_fgColor == c) return;
    m_fgColor = c;
    syncThemeToVt();
    emit colorsChanged();
    update();
}

void TerminalWidget::setCursorColor(const QColor &c) {
    if (m_cursorColor != c) { m_cursorColor = c; emit colorsChanged(); update(); }
}

void TerminalWidget::setSelectionColor(const QColor &c) {
    if (m_selectionColor != c) { m_selectionColor = c; emit colorsChanged(); update(); }
}

// ─── Grid ──────────────────────────────────────────────────

void TerminalWidget::geometryChange(const QRectF &newGeometry,
                                     const QRectF &oldGeometry)
{
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    recalculateGrid();
}

void TerminalWidget::recalculateGrid()
{
    qreal w = width();
    qreal h = height();

    // Ignore invalid sizes during layout transitions
    if (w < 10.0 || h < 10.0)
        return;

    qreal availW = w - (2.0 * m_padding);
    qreal availH = h - (2.0 * m_padding);

    if (availW < m_cellWidth || availH < m_cellHeight)
        return;

    int newCols = static_cast<int>(availW / m_cellWidth);
    int newRows = static_cast<int>(availH / m_cellHeight);
    newCols = qMax(1, newCols);
    newRows = qMax(1, newRows);

    if (newCols != m_cols || newRows != m_rows) {
        m_cols = newCols;
        m_rows = newRows;

        if (m_session)
            m_session->resize(m_rows, m_cols);

        qCDebug(logRender) << "Grid:" << m_cols << "x" << m_rows
                           << "widget:" << w << "x" << h;
        emit sizeChanged(m_rows, m_cols);
    }
}

// ─── Keyboard ──────────────────────────────────────────────

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_session) { event->ignore(); return; }
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
    const qreal w = width();
    const qreal h = height();

    painter->setRenderHint(QPainter::TextAntialiasing, true);

    // Fill ENTIRE widget surface with terminal background.
    // This ensures edge-to-edge coverage — no gaps.
    painter->fillRect(QRectF(0, 0, w, h), m_bgColor);

    if (!m_session) return;

    VtBridge *vt = m_session->vtBridge();
    if (!vt || m_rows <= 0 || m_cols <= 0) return;

    // Center the grid within the widget for even margins
    const qreal gridW = m_cols * m_cellWidth;
    const qreal gridH = m_rows * m_cellHeight;
    const qreal padX = qMax(m_padding * 1.0, (w - gridW) / 2.0);
    const qreal padY = qMax(m_padding * 1.0, (h - gridH) / 2.0);

    painter->setFont(m_font);

    QFontMetricsF fm(m_font);
    const qreal ascent = fm.ascent();

    int vtRows = qMin(m_rows, vt->rows());
    int vtCols = qMin(m_cols, vt->cols());

    for (int row = 0; row < vtRows; ++row) {
        qreal y = padY + row * m_cellHeight;

        for (int col = 0; col < vtCols; ++col) {
            Cell cell = vt->cellAt(row, col);
            if (cell.width == 0) continue;

            qreal x = padX + col * m_cellWidth;
            qreal cw = m_cellWidth * cell.width;

            // Resolve colors: invalid = use theme default
            QColor fg = cell.fg.isValid() ? cell.fg : m_fgColor;
            QColor bg = cell.bg.isValid() ? cell.bg : QColor();

            if (cell.attrs.reverse) {
                QColor tmp = fg;
                fg = bg.isValid() ? bg : m_bgColor;
                bg = tmp;
            }

            // Draw cell background only when non-default
            if (bg.isValid() && bg != m_bgColor)
                painter->fillRect(QRectF(x, y, cw, m_cellHeight), bg);

            // Draw text
            if (cell.text != QStringLiteral(" ") && !cell.text.isEmpty()
                && !cell.attrs.invisible) {
                QFont cellFont = m_font;
                bool fontChanged = false;
                if (cell.attrs.bold)   { cellFont.setBold(true);   fontChanged = true; }
                if (cell.attrs.italic) { cellFont.setItalic(true); fontChanged = true; }
                if (fontChanged) painter->setFont(cellFont);

                QColor drawFg = fg;
                if (cell.attrs.faint) drawFg.setAlphaF(0.5);

                painter->setPen(drawFg);
                painter->drawText(QPointF(x, y + ascent), cell.text);

                if (fontChanged) painter->setFont(m_font);
            }

            // Decorations
            if (cell.attrs.underline) {
                painter->setPen(fg);
                qreal uy = y + m_cellHeight - 1.5;
                painter->drawLine(QPointF(x, uy), QPointF(x + cw, uy));
            }
            if (cell.attrs.strikethrough) {
                painter->setPen(fg);
                qreal sy = y + m_cellHeight / 2.0;
                painter->drawLine(QPointF(x, sy), QPointF(x + cw, sy));
            }
        }
    }

    // ── Cursor ──────────────────────────────────────────────
    CursorState cs = vt->cursor();
    if (cs.visible && cs.row >= 0 && cs.row < vtRows
        && cs.col >= 0 && cs.col < vtCols && m_cursorVisible) {
        qreal cx = padX + cs.col * m_cellWidth;
        qreal cy = padY + cs.row * m_cellHeight;

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

} // namespace chiterm
