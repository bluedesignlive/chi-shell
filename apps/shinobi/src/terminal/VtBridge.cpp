#include "VtBridge.h"
#include "Log.h"
#include <cstring>

namespace chiterm {

VtBridge::VtBridge(int rows, int cols, QObject *parent)
    : QObject(parent), m_rows(rows), m_cols(cols)
{
    m_vterm = vterm_new(rows, cols);
    vterm_set_utf8(m_vterm, 1);
    m_screen = vterm_obtain_screen(m_vterm);

    static VTermScreenCallbacks cbs = {};
    cbs.damage      = &VtBridge::onDamage;
    cbs.movecursor  = &VtBridge::onMoveCursor;
    cbs.settermprop = &VtBridge::onSetTermProp;
    cbs.bell        = &VtBridge::onBell;
    cbs.sb_pushline = &VtBridge::onSbPushLine;
    cbs.sb_popline  = &VtBridge::onSbPopLine;

    vterm_screen_set_callbacks(m_screen, &cbs, this);
    vterm_screen_set_damage_merge(m_screen, VTERM_DAMAGE_SCROLL);
    vterm_screen_enable_altscreen(m_screen, 1);
    vterm_screen_reset(m_screen, 1);

    qCInfo(logVt) << "VtBridge created —" << cols << "x" << rows;
}

VtBridge::~VtBridge()
{
    if (m_vterm) { vterm_free(m_vterm); m_vterm = nullptr; }
}

void VtBridge::feed(const QByteArray &data)
{
    if (!m_vterm) return;
    vterm_input_write(m_vterm, data.constData(), static_cast<size_t>(data.size()));
    emit contentChanged();
}

void VtBridge::resize(int rows, int cols)
{
    if (!m_vterm || (rows == m_rows && cols == m_cols)) return;
    qCDebug(logVt) << "VT resize:" << m_cols << "x" << m_rows << "→" << cols << "x" << rows;
    m_rows = rows;
    m_cols = cols;
    vterm_set_size(m_vterm, rows, cols);
    emit contentChanged();
}

// ─── Color resolution ──────────────────────────────────────
// This is the KEY fix: we ask libvterm to resolve indexed and
// default colors through its internal palette into RGB values.
// This means our setAnsiPalette() and setDefaultColors() calls
// are respected when reading cells back.

QColor VtBridge::resolveColor(VTermColor c) const
{
    // Check default BEFORE resolving — default means "use theme"
    if (VTERM_COLOR_IS_DEFAULT_FG(&c) || VTERM_COLOR_IS_DEFAULT_BG(&c))
        return QColor();

    // Ask libvterm to resolve indexed → RGB through palette
    vterm_screen_convert_color_to_rgb(m_screen, &c);

    return QColor(c.rgb.red, c.rgb.green, c.rgb.blue);
}

void VtBridge::setDefaultColors(const QColor &fg, const QColor &bg)
{
    if (!m_vterm) return;
    VTermColor vfg, vbg;
    vterm_color_rgb(&vfg, fg.red(), fg.green(), fg.blue());
    vterm_color_rgb(&vbg, bg.red(), bg.green(), bg.blue());
    VTermState *state = vterm_obtain_state(m_vterm);
    vterm_state_set_default_colors(state, &vfg, &vbg);
    qCDebug(logVt) << "Default colors → fg:" << fg.name() << "bg:" << bg.name();
    emit contentChanged();
}

void VtBridge::setAnsiPalette(const QVector<QColor> &palette16)
{
    if (!m_vterm || palette16.size() < 16) return;
    VTermState *state = vterm_obtain_state(m_vterm);
    for (int i = 0; i < 16; ++i) {
        VTermColor vc;
        vterm_color_rgb(&vc, palette16[i].red(), palette16[i].green(), palette16[i].blue());
        vterm_state_set_palette_color(state, i, &vc);
    }
    qCDebug(logVt) << "ANSI 16-color palette updated";
    emit contentChanged();
}

// ─── Cell access ───────────────────────────────────────────

Cell VtBridge::cellAt(int row, int col) const
{
    if (!m_screen || row < 0 || row >= m_rows || col < 0 || col >= m_cols) {
        Cell e; e.text = QStringLiteral(" "); return e;
    }
    VTermPos pos = { row, col };
    VTermScreenCell vc;
    memset(&vc, 0, sizeof(vc));
    vterm_screen_get_cell(m_screen, pos, &vc);
    return vtermCellToCell(vc, this);
}

CursorState VtBridge::cursor() const
{
    CursorState cs;
    if (!m_screen) return cs;
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
        Cell e; e.text = QStringLiteral(" "); return e;
    }
    int idx = m_scrollback.size() - 1 - scrollRow;
    const auto &line = m_scrollback[idx];
    if (col < 0 || col >= line.cells.size()) {
        Cell e; e.text = QStringLiteral(" "); return e;
    }
    return line.cells[col];
}

// ─── Input encoding ────────────────────────────────────────

QByteArray VtBridge::encodeKey(int key, Qt::KeyboardModifiers mods,
                                const QString &text)
{
    if (!m_vterm) return {};

    VTermModifier mod = VTERM_MOD_NONE;
    if (mods & Qt::ShiftModifier)   mod = static_cast<VTermModifier>(mod | VTERM_MOD_SHIFT);
    if (mods & Qt::AltModifier)     mod = static_cast<VTermModifier>(mod | VTERM_MOD_ALT);
    if (mods & Qt::ControlModifier) mod = static_cast<VTermModifier>(mod | VTERM_MOD_CTRL);

    VTermKey vkey = VTERM_KEY_NONE;
    switch (key) {
    case Qt::Key_Return: case Qt::Key_Enter: vkey = VTERM_KEY_ENTER;     break;
    case Qt::Key_Tab:       vkey = VTERM_KEY_TAB;       break;
    case Qt::Key_Backspace: vkey = VTERM_KEY_BACKSPACE; break;
    case Qt::Key_Escape:    vkey = VTERM_KEY_ESCAPE;    break;
    case Qt::Key_Up:        vkey = VTERM_KEY_UP;        break;
    case Qt::Key_Down:      vkey = VTERM_KEY_DOWN;      break;
    case Qt::Key_Left:      vkey = VTERM_KEY_LEFT;      break;
    case Qt::Key_Right:     vkey = VTERM_KEY_RIGHT;     break;
    case Qt::Key_Insert:    vkey = VTERM_KEY_INS;       break;
    case Qt::Key_Delete:    vkey = VTERM_KEY_DEL;       break;
    case Qt::Key_Home:      vkey = VTERM_KEY_HOME;      break;
    case Qt::Key_End:       vkey = VTERM_KEY_END;       break;
    case Qt::Key_PageUp:    vkey = VTERM_KEY_PAGEUP;    break;
    case Qt::Key_PageDown:  vkey = VTERM_KEY_PAGEDOWN;  break;
    default:
        if (key >= Qt::Key_F1 && key <= Qt::Key_F12)
            vkey = static_cast<VTermKey>(VTERM_KEY_FUNCTION_0 + (key - Qt::Key_F1 + 1));
        break;
    }

    if (vkey != VTERM_KEY_NONE) {
        vterm_keyboard_key(m_vterm, vkey, mod);
    } else if (!text.isEmpty()) {
        for (const QChar &ch : text)
            vterm_keyboard_unichar(m_vterm, ch.unicode(), mod);
    }

    char outbuf[4096];
    size_t outlen = vterm_output_read(m_vterm, outbuf, sizeof(outbuf));
    return outlen > 0 ? QByteArray(outbuf, static_cast<int>(outlen)) : QByteArray();
}

QByteArray VtBridge::encodePaste(const QString &text)
{
    if (!m_vterm || text.isEmpty()) return {};

    // Bracketed paste: wrap in \e[200~ ... \e[201~
    vterm_keyboard_start_paste(m_vterm);
    for (const QChar &ch : text)
        vterm_keyboard_unichar(m_vterm, ch.unicode(), VTERM_MOD_NONE);
    vterm_keyboard_end_paste(m_vterm);

    char outbuf[65536];
    size_t outlen = vterm_output_read(m_vterm, outbuf, sizeof(outbuf));
    return outlen > 0 ? QByteArray(outbuf, static_cast<int>(outlen)) : QByteArray();
}

// ─── Callbacks ─────────────────────────────────────────────

int VtBridge::onDamage(VTermRect rect, void *user) {
    emit static_cast<VtBridge*>(user)->damageReceived(
        rect.start_row, rect.start_col, rect.end_row, rect.end_col);
    return 0;
}
int VtBridge::onMoveCursor(VTermPos pos, VTermPos, int, void *user) {
    emit static_cast<VtBridge*>(user)->cursorMoved(pos.row, pos.col);
    return 0;
}
int VtBridge::onSetTermProp(VTermProp prop, VTermValue *val, void *user) {
    if (prop == VTERM_PROP_TITLE) {
        emit static_cast<VtBridge*>(user)->titleChanged(
            QString::fromUtf8(val->string.str, val->string.len));
    }
    return 0;
}
int VtBridge::onBell(void *user) {
    emit static_cast<VtBridge*>(user)->bellRung();
    return 0;
}
int VtBridge::onSbPushLine(int cols, const VTermScreenCell *cells, void *user) {
    auto *self = static_cast<VtBridge*>(user);
    ScrollbackLine line;
    line.cells.resize(cols);
    for (int i = 0; i < cols; ++i)
        line.cells[i] = vtermCellToCell(cells[i], self);
    self->m_scrollback.append(std::move(line));
    while (self->m_scrollback.size() > MAX_SCROLLBACK)
        self->m_scrollback.removeFirst();
    return 0;
}
int VtBridge::onSbPopLine(int cols, VTermScreenCell *cells, void *user) {
    auto *self = static_cast<VtBridge*>(user);
    if (self->m_scrollback.isEmpty()) return 0;
    const auto &line = self->m_scrollback.last();
    int count = qMin(cols, line.cells.size());
    for (int i = 0; i < count; ++i) {
        memset(&cells[i], 0, sizeof(VTermScreenCell));
        cells[i].chars[0] = line.cells[i].text.isEmpty() ? ' '
                            : line.cells[i].text.at(0).unicode();
        cells[i].width = line.cells[i].width;
    }
    for (int i = count; i < cols; ++i) {
        memset(&cells[i], 0, sizeof(VTermScreenCell));
        cells[i].chars[0] = ' ';
        cells[i].width = 1;
    }
    self->m_scrollback.removeLast();
    return 1;
}

// ─── Cell conversion ───────────────────────────────────────

Cell VtBridge::vtermCellToCell(const VTermScreenCell &vc, const VtBridge *bridge)
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

    // Use the bridge's resolveColor which calls
    // vterm_screen_convert_color_to_rgb — this resolves indexed
    // colors through the palette we set, so ANSI colors work
    cell.fg = bridge->resolveColor(vc.fg);
    cell.bg = bridge->resolveColor(vc.bg);

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
