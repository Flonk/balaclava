#pragma once

#include "../utils/color/ansi.h"
#include "../utils/color/gradient2d.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// 2 bytes per terminal cell.  Low nibble = fill (0-8), bit 4 = inverted.
struct Cell {
  uint8_t fg;
  uint8_t bg;
  bool operator==(Cell o) const { return fg == o.fg && bg == o.bg; }
  bool operator!=(Cell o) const { return !(*this == o); }
  int fg_fill() const { return fg & 0xF; }
  bool fg_inv() const { return fg & 0x10; }
  int bg_fill() const { return bg & 0xF; }
  bool bg_inv() const { return bg & 0x10; }
  bool empty() const { return fg == 0 && bg == 0; }
};

inline constexpr Cell kEmpty{0, 0};
inline constexpr Cell kSentinel{0xFF, 0xFF};

inline Cell make_cell(int fg_fill, bool fg_inv, int bg_fill, bool bg_inv) {
  return {
      static_cast<uint8_t>((std::clamp(fg_fill, 0, 8)) | (fg_inv ? 0x10 : 0)),
      static_cast<uint8_t>((std::clamp(bg_fill, 0, 8)) |
                            (bg_inv ? 0x10 : 0))};
}

struct FrameBuffer {
  int cols = 0, rows = 0;
  std::vector<Cell> cells, prev;

  // ANSI color cache — tracks what the terminal currently has set
  Color cur_fg{}, cur_bg{};
  bool cur_fg_set = false, cur_bg_set = false;

  void resize(int c, int r) {
    cols = c;
    rows = r;
    auto n = static_cast<size_t>(c * r);
    cells.assign(n, kEmpty);
    prev.assign(n, kSentinel);
  }

  Cell *line(int row) { return &cells[row * cols]; }

  // ── ANSI output ──

  void emit_fg(std::string &buf, Color c) {
    if (cur_fg_set && cur_fg == c)
      return;
    emit_color(buf, kFgPrefix, c);
    cur_fg = c;
    cur_fg_set = true;
  }

  void emit_bg(std::string &buf, Color c) {
    if (cur_bg_set && cur_bg == c)
      return;
    emit_color(buf, kBgPrefix, c);
    cur_bg = c;
    cur_bg_set = true;
  }

  void emit_ansi_cell(std::string &buf, Cell c, bool has_bg, Color bg_color,
                      Color fg_color, Color bg_sec, const char **blk) {
    const int ff = c.fg_fill(), bf = c.bg_fill();
    const bool fi = c.fg_inv(), bi = c.bg_inv();
    bool need_bg_restore = false;
    const char *ch;

    if (ff >= 8) {
      emit_fg(buf, fg_color);
      ch = blk[8];
    } else if (ff > 0 && !fi) {
      emit_fg(buf, fg_color);
      if (has_bg && bf > ff && bf >= 4) {
        emit_bg(buf, bg_sec);
        need_bg_restore = true;
      }
      ch = blk[ff];
    } else if (ff > 0 && fi) {
      emit_bg(buf, fg_color);
      need_bg_restore = true;
      if (has_bg && bf > 0)
        emit_fg(buf, bg_sec);
      else
        emit_fg(buf, bg_color);
      ch = blk[8 - ff];
    } else if (has_bg && bf >= 8) {
      emit_fg(buf, bg_sec);
      ch = blk[8];
    } else if (has_bg && bf > 0 && !bi) {
      emit_fg(buf, bg_sec);
      ch = blk[bf];
    } else if (has_bg && bf > 0 && bi) {
      emit_bg(buf, bg_sec);
      need_bg_restore = true;
      emit_fg(buf, bg_color);
      ch = blk[8 - bf];
    } else {
      ch = blk[0];
    }

    buf.append(ch);

    if (need_bg_restore)
      emit_bg(buf, bg_color);
  }

  // Exclusion rect: framebuffer skip these cells (used by MPRIS overlay).
  int excl_row = 0, excl_col = 0, excl_w = 0, excl_h = 0;

  /// Diff current cells against prev and emit minimal ANSI into buf.
  /// horizontal: controls whether the gradient t is derived from row or column.
  void flush(std::string &buf, bool full_redraw, bool horizontal, bool has_bg,
             const Padding &pad, Color bg_color, const Gradient2D &colors,
             const Gradient2D &bg_colors, const char **blk,
             bool flip_fg = false, bool flip_bg = false) {
    // Reset to known state
    buf.append("\033[0m");
    emit_color(buf, kBgPrefix, bg_color);
    cur_bg = bg_color;
    cur_bg_set = true;
    cur_fg_set = false;

    const int content_rows = rows - pad.top - pad.bottom;
    const int content_cols = cols - pad.left - pad.right;

    int cr = -1, cc = -1;

    for (int row = 0; row < rows; ++row) {
      const int base = row * cols;
      for (int col = 0; col < cols; ++col) {
        // Skip exclusion rect
        if (excl_w > 0 && row >= excl_row && row < excl_row + excl_h &&
            col >= excl_col && col < excl_col + excl_w) {
          col = excl_col + excl_w - 1; // loop will ++col
          continue;
        }

        const int idx = base + col;
        const Cell c = cells[idx];

        bool dirty = full_redraw || c != prev[idx];
        if (!dirty)
          continue;

        if (cr != row || cc != col) {
          char pos[16];
          int n =
              snprintf(pos, sizeof(pos), "\033[%d;%dH", row + 1, col + 1);
          buf.append(pos, n);
          cr = row;
          cc = col;
        }

        if (c.empty()) {
          buf.push_back(' ');
        } else {
          float t;
          if (horizontal) {
            t = static_cast<float>(col - pad.left) /
                std::max(1.f, static_cast<float>(content_cols - 1));
          } else {
            t = static_cast<float>(row - pad.top) /
                std::max(1.f, static_cast<float>(content_rows - 1));
          }
          t = std::clamp(t, 0.f, 1.f);

          Color fg_color_v = colors.compute(flip_fg ? 1.f - t : t);
          Color bg_sec = bg_colors.compute(flip_bg ? 1.f - t : t);
          emit_ansi_cell(buf, c, has_bg, bg_color, fg_color_v, bg_sec, blk);
        }

        cc++;
      }
    }

    std::swap(cells, prev);

    // Excluded cells were never rendered — mark stale so they're redrawn
    // if the exclusion rect moves next frame.
    if (excl_w > 0) {
      for (int r = excl_row; r < excl_row + excl_h && r < rows; ++r) {
        int base = r * cols;
        for (int c = excl_col; c < excl_col + excl_w && c < cols; ++c)
          prev[base + c] = kSentinel;
      }
    }
  }
};
