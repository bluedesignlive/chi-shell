#pragma once

#include <QColor>
#include <QString>
#include <cstdint>

namespace chiterm {

// ─── Cell attributes packed into 16 bits ────────────────────
struct CellAttributes {
    bool bold          = false;
    bool italic        = false;
    bool underline     = false;
    bool strikethrough = false;
    bool blink         = false;
    bool reverse       = false;
    bool invisible     = false;
    bool faint         = false;

    void reset() { *this = {}; }
};

// ─── Terminal cell ──────────────────────────────────────────
struct Cell {
    QString text;                    // grapheme cluster (usually 1 char)
    QColor  fg;                      // invalid = use default
    QColor  bg;                      // invalid = use default
    CellAttributes attrs;
    int     width = 1;               // 1 = normal, 2 = wide, 0 = continuation

    bool isDefault() const {
        return text.isEmpty() && !fg.isValid() && !bg.isValid();
    }

    void reset() {
        text.clear();
        fg = QColor();
        bg = QColor();
        attrs.reset();
        width = 1;
    }
};

// ─── Cursor state ───────────────────────────────────────────
enum class CursorShape { Block, Beam, Underline };

struct CursorState {
    int row = 0;
    int col = 0;
    bool visible = true;
    CursorShape shape = CursorShape::Block;
};

} // namespace chiterm
