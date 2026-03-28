#include "TerminalWidget.h"
#include "session/TerminalSession.h"
#include "terminal/VtBridge.h"
#include "Log.h"

#include <QPainter>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QClipboard>
#include <QGuiApplication>

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

    m_cursorTimer.setInterval(530);
    connect(&m_cursorTimer, &QTimer::timeout, this, [this]() {
        m_cursorVisible = !m_cursorVisible;
        update();
    });
    m_cursorTimer.start();

    m_resizeTimer.setSingleShot(true);
    m_resizeTimer.setInterval(30);
    connect(&m_resizeTimer, &QTimer::timeout, this, &TerminalWidget::applyGridToSession);

    updateFontMetrics();
    qCDebug(logRender) << "TerminalWidget created";
}

// ─── Session ───────────────────────────────────────────────

QObject *TerminalWidget::sessionObject() const { return m_session; }
void TerminalWidget::setSessionObject(QObject *obj) { setSession(qobject_cast<TerminalSession *>(obj)); }

void TerminalWidget::setSession(TerminalSession *s)
{
    if (m_session == s) return;
    if (m_session) disconnect(m_session, nullptr, this, nullptr);
    m_session = s;
    m_scrollOffset = 0;
    if (m_session) {
        connect(m_session, &TerminalSession::contentChanged, this, [this]() {
            // Auto-scroll to bottom on new output if already at bottom
            if (m_scrollOffset == 0) update();
            else update(); // still repaint
        });
        syncThemeToVt();
        recalculateGrid();
        applyGridToSession();
        qCDebug(logRender) << "Session attached — grid:" << m_cols << "x" << m_rows;
    }
    emit sessionChanged();
    update();
}

// ─── Font ──────────────────────────────────────────────────

void TerminalWidget::setFontFamily(const QString &f) {
    if (m_fontFamily == f) return;
    m_fontFamily = f;
    updateFontMetrics(); recalculateGrid(); applyGridToSession(); update();
}

void TerminalWidget::setFontSize(int s) {
    s = qBound(6, s, 72);
    if (m_fontSize == s) return;
    m_fontSize = s;
    updateFontMetrics(); recalculateGrid(); applyGridToSession(); update();
}

void TerminalWidget::setPadding(int p) {
    if (m_padding == p) return;
    m_padding = p;
    recalculateGrid(); applyGridToSession();
    emit paddingChanged(); update();
}

void TerminalWidget::updateFontMetrics() {
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

// ─── Zoom ──────────────────────────────────────────────────

void TerminalWidget::zoomIn()  { setFontSize(m_fontSize + 1); }
void TerminalWidget::zoomOut() { setFontSize(m_fontSize - 1); }
void TerminalWidget::zoomReset() { setFontSize(m_defaultFontSize); }

// ─── Theme ─────────────────────────────────────────────────

void TerminalWidget::syncThemeToVt()
{
    if (!m_session || !m_session->vtBridge()) return;
    auto *vt = m_session->vtBridge();
    vt->setDefaultColors(m_fgColor, m_bgColor);

    bool dark = m_bgColor.lightnessF() < 0.5;
    QVector<QColor> p(16);
    if (dark) {
        p[ 0] = m_bgColor.lighter(130);
        p[ 1] = QColor(0xF2, 0x8B, 0x82);  p[ 2] = QColor(0x81, 0xC9, 0x95);
        p[ 3] = QColor(0xE2, 0xC7, 0x92);  p[ 4] = QColor(0x8A, 0xB4, 0xF8);
        p[ 5] = QColor(0xD3, 0xBC, 0xFD);  p[ 6] = QColor(0x78, 0xD9, 0xD9);
        p[ 7] = m_fgColor.darker(120);
        p[ 8] = m_bgColor.lighter(200);
        p[ 9] = QColor(0xFF, 0xA0, 0x97);  p[10] = QColor(0xA5, 0xDB, 0xB0);
        p[11] = QColor(0xF0, 0xD6, 0xA0);  p[12] = QColor(0xA8, 0xC7, 0xFA);
        p[13] = QColor(0xEB, 0xDD, 0xFF);  p[14] = QColor(0x9E, 0xEB, 0xEB);
        p[15] = m_fgColor;
    } else {
        p[ 0] = m_bgColor.darker(115);
        p[ 1] = QColor(0xB3, 0x26, 0x1E);  p[ 2] = QColor(0x18, 0x6C, 0x3A);
        p[ 3] = QColor(0x7D, 0x57, 0x00);  p[ 4] = QColor(0x1B, 0x6E, 0xF3);
        p[ 5] = QColor(0x68, 0x54, 0x8E);  p[ 6] = QColor(0x00, 0x6A, 0x6A);
        p[ 7] = m_fgColor.lighter(120);
        p[ 8] = m_bgColor.darker(160);
        p[ 9] = QColor(0xDC, 0x36, 0x2E);  p[10] = QColor(0x26, 0x8E, 0x4E);
        p[11] = QColor(0xA4, 0x72, 0x00);  p[12] = QColor(0x44, 0x85, 0xF4);
        p[13] = QColor(0x9C, 0x5B, 0xB8);  p[14] = QColor(0x00, 0x8A, 0x8A);
        p[15] = m_fgColor.darker(105);
    }
    vt->setAnsiPalette(p);
}

void TerminalWidget::setBackgroundColor(const QColor &c) {
    if (m_bgColor == c) return; m_bgColor = c; syncThemeToVt(); emit colorsChanged(); update();
}
void TerminalWidget::setForegroundColor(const QColor &c) {
    if (m_fgColor == c) return; m_fgColor = c; syncThemeToVt(); emit colorsChanged(); update();
}
void TerminalWidget::setCursorColor(const QColor &c) {
    if (m_cursorColor != c) { m_cursorColor = c; emit colorsChanged(); update(); }
}
void TerminalWidget::setSelectionColor(const QColor &c) {
    if (m_selectionColor != c) { m_selectionColor = c; emit colorsChanged(); update(); }
}

// ─── Grid ──────────────────────────────────────────────────

void TerminalWidget::geometryChange(const QRectF &n, const QRectF &o) {
    QQuickPaintedItem::geometryChange(n, o);
    recalculateGrid();
}

void TerminalWidget::recalculateGrid() {
    qreal w = width(), h = height();
    if (w < 10.0 || h < 10.0) return;
    qreal availW = w - 2.0 * m_padding, availH = h - 2.0 * m_padding;
    if (availW < m_cellWidth || availH < m_cellHeight) return;
    int nc = qMax(1, static_cast<int>(availW / m_cellWidth));
    int nr = qMax(1, static_cast<int>(availH / m_cellHeight));
    if (nc != m_cols || nr != m_rows) {
        m_cols = nc; m_rows = nr;
        m_pendingCols = nc; m_pendingRows = nr;
        m_resizeTimer.start();
        emit sizeChanged(m_rows, m_cols);
        update();
    }
}

void TerminalWidget::applyGridToSession() {
    if (!m_session || m_pendingRows <= 0 || m_pendingCols <= 0) return;
    m_session->resize(m_pendingRows, m_pendingCols);
    qCDebug(logRender) << "Grid applied:" << m_pendingCols << "x" << m_pendingRows
                       << "widget:" << width() << "x" << height();
}

// ─── Clipboard ─────────────────────────────────────────────

void TerminalWidget::copyToClipboard()
{
    // TODO: selection-based copy. For now, no-op.
    qCDebug(logRender) << "Copy (selection not yet implemented)";
}

void TerminalWidget::pasteFromClipboard()
{
    if (!m_session) return;
    QClipboard *clip = QGuiApplication::clipboard();
    QString text = clip->text();
    if (!text.isEmpty()) {
        m_session->paste(text);
        qCDebug(logRender) << "Pasted" << text.size() << "chars";
    }
}

// ─── Keyboard ──────────────────────────────────────────────

void TerminalWidget::keyPressEvent(QKeyEvent *event)
{
    if (!m_session) { event->ignore(); return; }

    m_cursorVisible = true;
    m_cursorTimer.start();

    auto key = event->key();
    auto mods = event->modifiers();

    // ── Terminal chrome shortcuts (NOT sent to shell) ────────

    // Ctrl+Shift+C → Copy
    if (key == Qt::Key_C && mods == (Qt::ControlModifier | Qt::ShiftModifier)) {
        copyToClipboard();
        event->accept(); return;
    }

    // Ctrl+Shift+V → Paste
    if (key == Qt::Key_V && mods == (Qt::ControlModifier | Qt::ShiftModifier)) {
        pasteFromClipboard();
        event->accept(); return;
    }

    // Ctrl+= or Ctrl+Shift+= → Zoom in
    if ((key == Qt::Key_Equal || key == Qt::Key_Plus) && (mods & Qt::ControlModifier)) {
        zoomIn();
        event->accept(); return;
    }

    // Ctrl+- → Zoom out
    if (key == Qt::Key_Minus && (mods & Qt::ControlModifier)) {
        zoomOut();
        event->accept(); return;
    }

    // Ctrl+0 → Zoom reset
    if (key == Qt::Key_0 && (mods & Qt::ControlModifier)) {
        zoomReset();
        event->accept(); return;
    }

    // Shift+PageUp / Shift+PageDown → Scroll
    if (key == Qt::Key_PageUp && (mods & Qt::ShiftModifier)) {
        if (m_session->vtBridge()) {
            m_scrollOffset = qMin(m_scrollOffset + m_rows,
                                   m_session->vtBridge()->scrollbackCount());
            update();
        }
        event->accept(); return;
    }
    if (key == Qt::Key_PageDown && (mods & Qt::ShiftModifier)) {
        m_scrollOffset = qMax(0, m_scrollOffset - m_rows);
        update();
        event->accept(); return;
    }

    // Shift+Home → Scroll to top
    if (key == Qt::Key_Home && (mods & Qt::ShiftModifier)) {
        if (m_session->vtBridge())
            m_scrollOffset = m_session->vtBridge()->scrollbackCount();
        update();
        event->accept(); return;
    }

    // Shift+End → Scroll to bottom
    if (key == Qt::Key_End && (mods & Qt::ShiftModifier)) {
        m_scrollOffset = 0;
        update();
        event->accept(); return;
    }

    // ── Any other key resets scroll to bottom ───────────────
    if (m_scrollOffset > 0) {
        m_scrollOffset = 0;
        update();
    }

    // ── Forward to shell ────────────────────────────────────
    m_session->sendKey(key, mods, event->text());
    event->accept();
}

void TerminalWidget::keyReleaseEvent(QKeyEvent *event) { event->accept(); }

// ─── Mouse wheel → scroll ──────────────────────────────────

void TerminalWidget::wheelEvent(QWheelEvent *event)
{
    if (!m_session || !m_session->vtBridge()) {
        event->ignore();
        return;
    }

    // Ctrl+Scroll → Zoom
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) zoomIn();
        else if (event->angleDelta().y() < 0) zoomOut();
        event->accept();
        return;
    }

    // Normal scroll → scrollback
    int delta = event->angleDelta().y();
    int lines = qMax(1, qAbs(delta) / 40);  // ~3 lines per notch

    if (delta > 0) {
        // Scroll up (into scrollback)
        m_scrollOffset = qMin(m_scrollOffset + lines,
                               m_session->vtBridge()->scrollbackCount());
    } else {
        // Scroll down (toward live)
        m_scrollOffset = qMax(0, m_scrollOffset - lines);
    }

    update();
    event->accept();
}

// ─── Paint ─────────────────────────────────────────────────

void TerminalWidget::paint(QPainter *painter)
{
    const qreal w = width(), h = height();
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->fillRect(QRectF(0, 0, w, h), m_bgColor);

    if (!m_session) return;
    VtBridge *vt = m_session->vtBridge();
    if (!vt || m_rows <= 0 || m_cols <= 0) return;

    int vtRows = vt->rows();
    int vtCols = vt->cols();
    int renderRows = qMin(m_rows, vtRows);
    int renderCols = qMin(m_cols, vtCols);

    const qreal padX = m_padding;
    const qreal padY = m_padding;

    painter->setFont(m_font);
    QFontMetricsF fm(m_font);
    const qreal ascent = fm.ascent();

    // ── Render with scrollback offset ───────────────────────
    // If scrollOffset > 0, we're viewing scrollback history.
    // Top rows come from scrollback, bottom rows from live screen.
    int sbCount = vt->scrollbackCount();
    int sbVisible = qMin(m_scrollOffset, renderRows);  // scrollback rows shown
    int liveStart = 0;  // first live row to show
    int liveRows = renderRows - sbVisible;

    if (m_scrollOffset > 0) {
        // Some rows are from scrollback
        liveStart = 0;  // when scrolled, show top of live screen
        // Actually: if scrollOffset < renderRows, we show some scrollback + some live
        // scrollback rows: from (scrollOffset - sbVisible) to scrollOffset
        // live rows: from 0 to (renderRows - sbVisible)
    }

    for (int screenRow = 0; screenRow < renderRows; ++screenRow) {
        qreal y = padY + screenRow * m_cellHeight;

        // Determine if this row is from scrollback or live
        int sbRow = m_scrollOffset - screenRow - 1;  // scrollback index (0=most recent)
        bool fromScrollback = (sbRow >= 0 && sbRow < sbCount && m_scrollOffset > 0);

        for (int col = 0; col < renderCols; ++col) {
            Cell cell;
            if (fromScrollback) {
                cell = vt->scrollbackCellAt(sbRow, col);
            } else {
                int liveRow = screenRow - qMin(m_scrollOffset, renderRows);
                if (m_scrollOffset >= renderRows) liveRow = screenRow; // shouldn't happen
                else liveRow = screenRow - m_scrollOffset;
                if (liveRow < 0 || liveRow >= vtRows) continue;
                cell = vt->cellAt(liveRow, col);
            }

            if (cell.width == 0) continue;

            qreal x = padX + col * m_cellWidth;
            qreal cw = m_cellWidth * cell.width;

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

    // ── Cursor (only when at bottom / live view) ────────────
    if (m_scrollOffset == 0) {
        CursorState cs = vt->cursor();
        if (cs.visible && m_cursorVisible
            && cs.row >= 0 && cs.row < renderRows
            && cs.col >= 0 && cs.col < renderCols) {
            qreal cx = padX + cs.col * m_cellWidth;
            qreal cy = padY + cs.row * m_cellHeight;

            switch (cs.shape) {
            case CursorShape::Block:
                painter->fillRect(QRectF(cx, cy, m_cellWidth, m_cellHeight), m_cursorColor);
                {
                    Cell cc = vt->cellAt(cs.row, cs.col);
                    if (!cc.text.isEmpty() && cc.text != QStringLiteral(" ")) {
                        painter->setPen(m_bgColor);
                        painter->drawText(QPointF(cx, cy + ascent), cc.text);
                    }
                }
                break;
            case CursorShape::Beam:
                painter->fillRect(QRectF(cx, cy, 2.0, m_cellHeight), m_cursorColor);
                break;
            case CursorShape::Underline:
                painter->fillRect(QRectF(cx, cy + m_cellHeight - 2.0, m_cellWidth, 2.0),
                                  m_cursorColor);
                break;
            }
        }
    }

    // ── Scrollback indicator ────────────────────────────────
    if (m_scrollOffset > 0) {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(m_cursorColor.red(), m_cursorColor.green(),
                                  m_cursorColor.blue(), 180));
        QString indicator = QString("↑ %1 lines").arg(m_scrollOffset);
        QFont indFont = m_font;
        indFont.setPointSize(10);
        painter->setFont(indFont);
        QFontMetricsF ifm(indFont);
        qreal iw = ifm.horizontalAdvance(indicator) + 16;
        qreal ih = ifm.height() + 6;
        qreal ix = (w - iw) / 2.0;
        qreal iy = 4;

        painter->setBrush(QColor(m_cursorColor.red(), m_cursorColor.green(),
                                  m_cursorColor.blue(), 40));
        painter->drawRoundedRect(QRectF(ix, iy, iw, ih), 4, 4);
        painter->setPen(m_cursorColor);
        painter->drawText(QPointF(ix + 8, iy + ifm.ascent() + 3), indicator);
        painter->setFont(m_font);
    }
}

} // namespace chiterm
