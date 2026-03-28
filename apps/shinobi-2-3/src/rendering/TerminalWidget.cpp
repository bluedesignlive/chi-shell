#include "TerminalWidget.h"
#include "session/TerminalSession.h"
#include "terminal/VtBridge.h"
#include "Log.h"

#include <QPainter>
#include <QKeyEvent>

namespace chiterm {

TerminalWidget::TerminalWidget(QQuickItem *parent)
    : QQuickPaintedItem(parent)
    , m_bgColor(0x0d, 0x0b, 0x0f)
    , m_fgColor(0xe7, 0xe0, 0xe8)
    , m_cursorColor(0xd3, 0xbc, 0xfd)
    , m_selectionColor(0x4f, 0x3d, 0x74, 100)
{
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setAcceptedMouseButtons(Qt::AllButtons);
    setFocus(true);
    setAntialiasing(true);
    setOpaquePainting(true);

    // Cursor blink
    m_cursorTimer.setInterval(530);
    connect(&m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_cursorTimer.start();

    // Resize debounce — wait 30ms after last geometry change
    // before actually resizing the PTY/VT. The widget still
    // repaints immediately at the new size using whatever grid
    // the VT currently has. This prevents flooding the PTY
    // with dozens of TIOCSWINSZ during a window drag.
    m_resizeTimer.setSingleShot(true);
    m_resizeTimer.setInterval(30);
    connect(&m_resizeTimer, &QTimer::timeout, this, &TerminalWidget::applyGridToSession);

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
    if (m_session) disconnect(m_session, nullptr, this, nullptr);

    m_session = s;

    if (m_session) {
        connect(m_session, &TerminalSession::contentChanged,
                this, [this]() { update(); });
        syncThemeToVt();
        recalculateGrid();
        applyGridToSession();
        qCDebug(logRender) << "Session attached — grid:"
                           << m_cols << "x" << m_rows;
    }

    emit sessionChanged();
    update();
}

// ─── Font ──────────────────────────────────────────────────

void TerminalWidget::setFontFamily(const QString &f)
{
    if (m_fontFamily == f) return;
    m_fontFamily = f;
    updateFontMetrics();
    recalculateGrid();
    applyGridToSession();
    update();
}

void TerminalWidget::setFontSize(int s)
{
    s = qBound(6, s, 72);
    if (m_fontSize == s) return;
    m_fontSize = s;
    updateFontMetrics();
    recalculateGrid();
    applyGridToSession();
    update();
}

void TerminalWidget::setPadding(int p)
{
    if (m_padding == p) return;
    m_padding = p;
    recalculateGrid();
    applyGridToSession();
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

// ─── Theme sync ────────────────────────────────────────────

void TerminalWidget::syncThemeToVt()
{
    if (!m_session || !m_session->vtBridge()) return;

    auto *vt = m_session->vtBridge();

    // Set default fg/bg
    vt->setDefaultColors(m_fgColor, m_bgColor);

    // Build ANSI 16-color palette from theme-aware colors.
    // Dark mode: dark variants for normal, brighter for bright.
    // Light mode: saturated for normal, even more saturated for bright.
    //
    // The palette is designed to be legible on the current m_bgColor.
    bool dark = m_bgColor.lightnessF() < 0.5;

    QVector<QColor> palette(16);
    if (dark) {
        palette[ 0] = m_bgColor.lighter(120);              // black  (slightly lighter than bg)
        palette[ 1] = QColor(0xF2, 0x8B, 0x82);            // red
        palette[ 2] = QColor(0x81, 0xC9, 0x95);            // green
        palette[ 3] = QColor(0xE2, 0xC7, 0x92);            // yellow
        palette[ 4] = QColor(0x8A, 0xB4, 0xF8);            // blue
        palette[ 5] = QColor(0xD3, 0xBC, 0xFD);            // magenta (primary)
        palette[ 6] = QColor(0x78, 0xD9, 0xD9);            // cyan
        palette[ 7] = m_fgColor.darker(120);                // white  (slightly dimmer than fg)
        palette[ 8] = m_bgColor.lighter(200);               // bright black
        palette[ 9] = QColor(0xFF, 0xA0, 0x97);            // bright red
        palette[10] = QColor(0xA5, 0xDB, 0xB0);            // bright green
        palette[11] = QColor(0xF0, 0xD6, 0xA0);            // bright yellow
        palette[12] = QColor(0xA8, 0xC7, 0xFA);            // bright blue
        palette[13] = QColor(0xEB, 0xDD, 0xFF);            // bright magenta
        palette[14] = QColor(0x9E, 0xEB, 0xEB);            // bright cyan
        palette[15] = m_fgColor;                             // bright white
    } else {
        palette[ 0] = m_bgColor.darker(110);                // black
        palette[ 1] = QColor(0xB3, 0x26, 0x1E);            // red
        palette[ 2] = QColor(0x18, 0x6C, 0x3A);            // green
        palette[ 3] = QColor(0x7D, 0x57, 0x00);            // yellow
        palette[ 4] = QColor(0x1B, 0x6E, 0xF3);            // blue
        palette[ 5] = QColor(0x68, 0x54, 0x8E);            // magenta (primary)
        palette[ 6] = QColor(0x00, 0x6A, 0x6A);            // cyan
        palette[ 7] = m_fgColor.lighter(130);                // white
        palette[ 8] = m_bgColor.darker(150);                 // bright black
        palette[ 9] = QColor(0xDC, 0x36, 0x2E);            // bright red
        palette[10] = QColor(0x26, 0x8E, 0x4E);            // bright green
        palette[11] = QColor(0xA4, 0x72, 0x00);            // bright yellow
        palette[12] = QColor(0x44, 0x85, 0xF4);            // bright blue
        palette[13] = QColor(0x9C, 0x5B, 0xB8);            // bright magenta
        palette[14] = QColor(0x00, 0x8A, 0x8A);            // bright cyan
        palette[15] = m_fgColor.darker(105);                 // bright white
    }

    vt->setAnsiPalette(palette);
}

void TerminalWidget::setBackgroundColor(const QColor &c) {
    if (m_bgColor == c) return;
    m_bgColor = c;
    syncThemeToVt();
    emit colorsChanged(); update();
}
void TerminalWidget::setForegroundColor(const QColor &c) {
    if (m_fgColor == c) return;
    m_fgColor = c;
    syncThemeToVt();
    emit colorsChanged(); update();
}
void TerminalWidget::setCursorColor(const QColor &c) {
    if (m_cursorColor != c) { m_cursorColor = c; emit colorsChanged(); update(); }
}
void TerminalWidget::setSelectionColor(const QColor &c) {
    if (m_selectionColor != c) { m_selectionColor = c; emit colorsChanged(); update(); }
}

// ─── Grid calculation + debounced resize ───────────────────

void TerminalWidget::geometryChange(const QRectF &newGeo,
                                     const QRectF &oldGeo)
{
    QQuickPaintedItem::geometryChange(newGeo, oldGeo);
    recalculateGrid();
}

void TerminalWidget::recalculateGrid()
{
    qreal w = width();
    qreal h = height();
    if (w < 10.0 || h < 10.0) return;

    qreal availW = w - (2.0 * m_padding);
    qreal availH = h - (2.0 * m_padding);
    if (availW < m_cellWidth || availH < m_cellHeight) return;

    int newCols = qMax(1, static_cast<int>(availW / m_cellWidth));
    int newRows = qMax(1, static_cast<int>(availH / m_cellHeight));

    if (newCols != m_cols || newRows != m_rows) {
        m_cols = newCols;
        m_rows = newRows;
        m_pendingCols = newCols;
        m_pendingRows = newRows;

        // Debounce the actual PTY/VT resize
        m_resizeTimer.start();

        emit sizeChanged(m_rows, m_cols);
        update();
    }
}

void TerminalWidget::applyGridToSession()
{
    if (!m_session || m_pendingRows <= 0 || m_pendingCols <= 0) return;

    m_session->resize(m_pendingRows, m_pendingCols);

    qCDebug(logRender) << "Grid applied:" << m_pendingCols << "x" << m_pendingRows
                       << "widget:" << width() << "x" << height();
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

void TerminalWidget::keyReleaseEvent(QKeyEvent *event) { event->accept(); }

// ─── Paint ─────────────────────────────────────────────────

void TerminalWidget::paint(QPainter *painter)
{
    const qreal w = width();
    const qreal h = height();

    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->fillRect(QRectF(0, 0, w, h), m_bgColor);

    if (!m_session) return;
    VtBridge *vt = m_session->vtBridge();
    if (!vt || m_rows <= 0 || m_cols <= 0) return;

    // Render rows from VT. The VT grid may be a different size
    // than our widget grid during a resize transition. Render
    // whichever is smaller — this avoids accessing out-of-bounds
    // cells and prevents content clipping.
    int vtRows = vt->rows();
    int vtCols = vt->cols();
    int renderRows = qMin(m_rows, vtRows);
    int renderCols = qMin(m_cols, vtCols);

    // Position the grid content — top-left anchored with padding
    const qreal padX = m_padding;
    const qreal padY = m_padding;

    painter->setFont(m_font);
    QFontMetricsF fm(m_font);
    const qreal ascent = fm.ascent();

    for (int row = 0; row < renderRows; ++row) {
        qreal y = padY + row * m_cellHeight;

        for (int col = 0; col < renderCols; ++col) {
            Cell cell = vt->cellAt(row, col);
            if (cell.width == 0) continue;

            qreal x = padX + col * m_cellWidth;
            qreal cw = m_cellWidth * cell.width;

            // Resolve: invalid QColor = use theme default
            QColor fg = cell.fg.isValid() ? cell.fg : m_fgColor;
            QColor bg = cell.bg.isValid() ? cell.bg : QColor();

            if (cell.attrs.reverse) {
                QColor tmp = fg;
                fg = bg.isValid() ? bg : m_bgColor;
                bg = tmp;
            }

            if (bg.isValid() && bg != m_bgColor)
                painter->fillRect(QRectF(x, y, cw, m_cellHeight), bg);

            if (!cell.text.isEmpty() && cell.text != QStringLiteral(" ")
                && !cell.attrs.invisible) {
                QFont cf = m_font;
                bool changed = false;
                if (cell.attrs.bold)   { cf.setBold(true);   changed = true; }
                if (cell.attrs.italic) { cf.setItalic(true); changed = true; }
                if (changed) painter->setFont(cf);

                QColor drawFg = fg;
                if (cell.attrs.faint) drawFg.setAlphaF(0.5);

                painter->setPen(drawFg);
                painter->drawText(QPointF(x, y + ascent), cell.text);

                if (changed) painter->setFont(m_font);
            }

            if (cell.attrs.underline) {
                painter->setPen(fg);
                painter->drawLine(QPointF(x, y + m_cellHeight - 1.5),
                                  QPointF(x + cw, y + m_cellHeight - 1.5));
            }
            if (cell.attrs.strikethrough) {
                painter->setPen(fg);
                painter->drawLine(QPointF(x, y + m_cellHeight / 2.0),
                                  QPointF(x + cw, y + m_cellHeight / 2.0));
            }
        }
    }

    // Cursor
    CursorState cs = vt->cursor();
    if (cs.visible && m_cursorVisible
        && cs.row >= 0 && cs.row < renderRows
        && cs.col >= 0 && cs.col < renderCols) {
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
