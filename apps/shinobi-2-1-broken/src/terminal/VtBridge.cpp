#include "VtBridge.h"
#include "Log.h"

#include <QGuiApplication>
#include <cstring>

namespace chiterm {

VtBridge::VtBridge(int rows, int cols, QObject *parent)
    : QObject(parent)
    , m_rows(rows)
    , m_cols(cols)
{
    m_vterm = vterm_new(rows, cols);
    vterm_set_utf8(m_vterm, 1);

    m_screen = vterm_obtain_screen(m_vterm);

    static VTermScreenCallbacks callbacks = {};
    callbacks.damage      = &VtBridge::onDamage;
    callbacks.movecursor  = &VtBridge::onMoveCursor;
    callbacks.settermprop = &VtBridge::onSetTermProp;
    callbacks.bell        = &VtBridge::onBell;
    callbacks.sb_pushline = &VtBridge::onSbPushLine;
    callbacks.sb_popline  = &VtBridge::onSbPopLine;

    vterm_screen_set_callbacks(m_screen, &callbacks, this);
    vterm_screen_set_damage_merge(m_screen, VTERM_DAMAGE_SCROLL);
    vterm_screen_enable_altscreen(m_screen, 1);
    vterm_screen_reset(m_screen, 1);

    qCInfo(logVt) << "VtBridge created — libvterm" << cols << "x" << rows
                  << "scrollback max:" << MAX_SCROLLBACK;
}

VtBridge::~VtBridge()
{
    if (m_vterm) {
        vterm_free(m_vterm);
        m_vterm = nullptr;
    }
    qCDebug(logVt) << "VtBridge destroyed";
}

void VtBridge::feed(const QByteArray &data)
{
    if (!m_vterm)
        return;
    vterm_input_write(m_vterm, data.constData(),
                      static_cast<size_t>(data.size()));
    emit contentChanged();
}

void VtBridge::resize(int rows, int cols)
{
    if (!m_vterm || (rows == m_rows && cols == m_cols))
        return;

    qCDebug(logVt) << "VT resize:" << m_cols << "x" << m_rows
                   << "→" << cols << "x" << rows;
    m_rows = rows;
    m_cols = cols;
    vterm_set_size(m_vterm, rows, cols);
    emit contentChanged();
}

Cell VtBridge::cellAt(int row, int col) const
{
    if (!m_screen || row < 0 || row >= m_rows || col < 0 || col >= m_cols) {
        Cell empty;
        empty.text = QStringLiteral(" ");
        return empty;
    }

    VTermPos pos;
    pos.row = row;
    pos.col = col;

    VTermScreenCell vc;
    memset(&vc, 0, sizeof(vc));
    vterm_screen_get_cell(m_screen, pos, &vc);

    return vtermCellToCell(vc);
}

CursorState VtBridge::cursor() const
{
    CursorState cs;
    if (!m_screen)
        return cs;

    VTermPos pos;
    vterm_state_get_cursorpos(vterm_obtain_state(m_vterm), &pos);
    cs.row = pos.row;
    cs.col = pos.col;
    cs.visible = true;
    return cs;
}

Cell VtBridge::scrollbackCellAt(int scrollRow, int col) const
{
    if (scrollRow < 0 || scrollRow >= m_scrollback.size()) {
        Cell empty;
        empty.text = QStringLiteral(" ");
        return empty;
    }

    int idx = m_scrollback.size() - 1 - scrollRow;
    const auto &line = m_scrollback[idx];

    if (col < 0 || col >= line.cells.size()) {
        Cell empty;
        empty.text = QStringLiteral(" ");
        return empty;
    }
    return line.cells[col];
}

QByteArray VtBridge::encodeKey(int key, Qt::KeyboardModifiers mods,
                                const QString &text)
{
    if (!m_vterm)
        return {};

    VTermModifier mod = VTERM_MOD_NONE;
    if (mods & Qt::ShiftModifier)   mod = static_cast<VTermModifier>(mod | VTERM_MOD_SHIFT);
    if (mods & Qt::AltModifier)     mod = static_cast<VTermModifier>(mod | VTERM_MOD_ALT);
    if (mods & Qt::ControlModifier) mod = static_cast<VTermModifier>(mod | VTERM_MOD_CTRL);

    VTermKey vkey = VTERM_KEY_NONE;
    switch (key) {
    case Qt::Key_Return: case Qt::Key_Enter: vkey = VTERM_KEY_ENTER;     break;
    case Qt::Key_Tab:                        vkey = VTERM_KEY_TAB;       break;
    case Qt::Key_Backspace:                  vkey = VTERM_KEY_BACKSPACE; break;
    case Qt::Key_Escape:                     vkey = VTERM_KEY_ESCAPE;    break;
    case Qt::Key_Up:                         vkey = VTERM_KEY_UP;        break;
    case Qt::Key_Down:                       vkey = VTERM_KEY_DOWN;      break;
    case Qt::Key_Left:                       vkey = VTERM_KEY_LEFT;      break;
    case Qt::Key_Right:                      vkey = VTERM_KEY_RIGHT;     break;
    case Qt::Key_Insert:                     vkey = VTERM_KEY_INS;       break;
    case Qt::Key_Delete:                     vkey = VTERM_KEY_DEL;       break;
    case Qt::Key_Home:                       vkey = VTERM_KEY_HOME;      break;
    case Qt::Key_End:                        vkey = VTERM_KEY_END;       break;
    case Qt::Key_PageUp:                     vkey = VTERM_KEY_PAGEUP;    break;
    case Qt::Key_PageDown:                   vkey = VTERM_KEY_PAGEDOWN;  break;
    case Qt::Key_F1:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 1);  break;
    case Qt::Key_F2:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 2);  break;
    case Qt::Key_F3:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 3);  break;
    case Qt::Key_F4:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 4);  break;
    case Qt::Key_F5:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 5);  break;
    case Qt::Key_F6:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 6);  break;
    case Qt::Key_F7:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 7);  break;
    case Qt::Key_F8:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 8);  break;
    case Qt::Key_F9:  vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 9);  break;
    case Qt::Key_F10: vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 10); break;
    case Qt::Key_F11: vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 11); break;
    case Qt::Key_F12: vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + 12); break;
    default: break;
    }

    if (vkey != VTERM_KEY_NONE) {
        vterm_keyboard_key(m_vterm, vkey, mod);
    } else if (!text.isEmpty()) {
        for (const QChar &ch : text) {
            uint32_t codepoint = ch.unicode();
            vterm_keyboard_unichar(m_vterm, codepoint, mod);
        }
    }

    char outbuf[4096];
    size_t outlen = vterm_output_read(m_vterm, outbuf, sizeof(outbuf));

    if (outlen > 0)
        return QByteArray(outbuf, static_cast<int>(outlen));
    return {};
}

// ─── libvterm callbacks ────────────────────────────────────

int VtBridge::onDamage(VTermRect rect, void *user)
{
    auto *self = static_cast<VtBridge *>(user);
    emit self->damageReceived(rect.start_row, rect.start_col,
                              rect.end_row, rect.end_col);
    return 0;
}

int VtBridge::onMoveCursor(VTermPos pos, VTermPos, int, void *user)
{
    auto *self = static_cast<VtBridge *>(user);
    emit self->cursorMoved(pos.row, pos.col);
    return 0;
}

int VtBridge::onSetTermProp(VTermProp prop, VTermValue *val, void *user)
{
    auto *self = static_cast<VtBridge *>(user);
    switch (prop) {
    case VTERM_PROP_TITLE: {
        QString title = QString::fromUtf8(val->string.str, val->string.len);
        qCDebug(logVt) << "Title changed:" << title;
        emit self->titleChanged(title);
        break;
    }
    default:
        break;
    }
    return 0;
}

int VtBridge::onBell(void *user)
{
    auto *self = static_cast<VtBridge *>(user);
    qCDebug(logVt) << "Bell";
    emit self->bellRung();
    return 0;
}

int VtBridge::onSbPushLine(int cols, const VTermScreenCell *cells, void *user)
{
    auto *self = static_cast<VtBridge *>(user);

    ScrollbackLine line;
    line.cells.resize(cols);
    for (int i = 0; i < cols; ++i)
        line.cells[i] = vtermCellToCell(cells[i]);

    self->m_scrollback.append(std::move(line));

    while (self->m_scrollback.size() > MAX_SCROLLBACK)
        self->m_scrollback.removeFirst();

    return 0;
}

int VtBridge::onSbPopLine(int cols, VTermScreenCell *cells, void *user)
{
    auto *self = static_cast<VtBridge *>(user);

    if (self->m_scrollback.isEmpty())
        return 0;

    const auto &line = self->m_scrollback.last();
    int count = qMin(cols, line.cells.size());

    for (int i = 0; i < count; ++i) {
        memset(&cells[i], 0, sizeof(VTermScreenCell));
        const Cell &c = line.cells[i];
        if (!c.text.isEmpty())
            cells[i].chars[0] = c.text.at(0).unicode();
        else
            cells[i].chars[0] = ' ';
        cells[i].width = c.width;
    }

    for (int i = count; i < cols; ++i) {
        memset(&cells[i], 0, sizeof(VTermScreenCell));
        cells[i].chars[0] = ' ';
        cells[i].width = 1;
    }

    self->m_scrollback.removeLast();
    return 1;
}

// ─── Helpers ───────────────────────────────────────────────

QColor VtBridge::vtermColorToQColor(VTermColor c)
{
    if (VTERM_COLOR_IS_RGB(&c))
        return QColor(c.rgb.red, c.rgb.green, c.rgb.blue);

    if (VTERM_COLOR_IS_INDEXED(&c)) {
        static const QColor ansi16[] = {
            QColor(0x1d, 0x1b, 0x20), QColor(0xb3, 0x26, 0x1e),
            QColor(0x38, 0x6a, 0x20), QColor(0x7d, 0x57, 0x00),
            QColor(0x00, 0x61, 0xa4), QColor(0x7b, 0x43, 0x97),
            QColor(0x00, 0x6a, 0x6a), QColor(0xca, 0xc4, 0xd0),
            QColor(0x48, 0x45, 0x4e), QColor(0xdc, 0x36, 0x2e),
            QColor(0x4c, 0x8c, 0x2b), QColor(0xa4, 0x72, 0x00),
            QColor(0x00, 0x7d, 0xd4), QColor(0x9c, 0x5b, 0xb8),
            QColor(0x00, 0x8a, 0x8a), QColor(0xe6, 0xe1, 0xe5),
        };

        int idx = c.indexed.idx;
        if (idx < 16) return ansi16[idx];

        if (idx < 232) {
            idx -= 16;
            int r = (idx / 36) * 51;
            int g = ((idx / 6) % 6) * 51;
            int b = (idx % 6) * 51;
            return QColor(r, g, b);
        }

        int gray = 8 + (idx - 232) * 10;
        return QColor(gray, gray, gray);
    }

    return QColor();
}

Cell VtBridge::vtermCellToCell(const VTermScreenCell &vc)
{
    Cell cell;

    if (vc.chars[0] != 0) {
        QString str;
        for (int i = 0; vc.chars[i] != 0 && i < VTERM_MAX_CHARS_PER_CELL; ++i) {
            char32_t cp = static_cast<char32_t>(vc.chars[i]);
            str += QString::fromUcs4(&cp, 1);
        }
        cell.text = str;
    } else {
        cell.text = QStringLiteral(" ");
    }

    cell.fg = vtermColorToQColor(vc.fg);
    cell.bg = vtermColorToQColor(vc.bg);

    cell.attrs.bold          = vc.attrs.bold;
    cell.attrs.italic        = vc.attrs.italic;
    cell.attrs.underline     = vc.attrs.underline != 0;
    cell.attrs.strikethrough = vc.attrs.strike;
    cell.attrs.blink         = vc.attrs.blink;
    cell.attrs.reverse       = vc.attrs.reverse;
    cell.attrs.invisible     = vc.attrs.conceal;
    cell.attrs.faint         = false;

    cell.width = vc.width;
    return cell;
}

} // namespace chiterm
