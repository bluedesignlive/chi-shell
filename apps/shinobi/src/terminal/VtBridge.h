#pragma once

#include "Cell.h"

#include <QObject>
#include <QVector>
#include <vterm.h>

namespace chiterm {

class VtBridge : public QObject {
    Q_OBJECT

public:
    explicit VtBridge(int rows, int cols, QObject *parent = nullptr);
    ~VtBridge() override;

    void feed(const QByteArray &data);
    void resize(int rows, int cols);

    // Cell access — resolves all colors to RGB via libvterm palette
    Cell cellAt(int row, int col) const;
    CursorState cursor() const;
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }

    QByteArray encodeKey(int key, Qt::KeyboardModifiers mods,
                         const QString &text);
    // Paste: write raw text through PTY
    QByteArray encodePaste(const QString &text);

    int scrollbackCount() const { return m_scrollback.size(); }
    Cell scrollbackCellAt(int scrollRow, int col) const;

    void setDefaultColors(const QColor &fg, const QColor &bg);
    void setAnsiPalette(const QVector<QColor> &palette16);

signals:
    void damageReceived(int startRow, int startCol, int endRow, int endCol);
    void cursorMoved(int row, int col);
    void titleChanged(const QString &title);
    void bellRung();
    void contentChanged();

private:
    static int onDamage(VTermRect rect, void *user);
    static int onMoveCursor(VTermPos pos, VTermPos oldpos, int visible, void *user);
    static int onSetTermProp(VTermProp prop, VTermValue *val, void *user);
    static int onBell(void *user);
    static int onSbPushLine(int cols, const VTermScreenCell *cells, void *user);
    static int onSbPopLine(int cols, VTermScreenCell *cells, void *user);

    // Resolves VTermColor to QColor using the screen's palette.
    // Returns invalid QColor only for true defaults.
    QColor resolveColor(VTermColor c) const;
    static Cell vtermCellToCell(const VTermScreenCell &vc, const VtBridge *bridge);

    VTerm       *m_vterm   = nullptr;
    VTermScreen *m_screen  = nullptr;
    int          m_rows;
    int          m_cols;

    struct ScrollbackLine { QVector<Cell> cells; };
    QVector<ScrollbackLine> m_scrollback;
    static constexpr int MAX_SCROLLBACK = 10000;
};

} // namespace chiterm
