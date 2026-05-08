#include "../../args/args.h"
#include "../common.h"
#include "../mpris/overlay.h"
#include "framebuffer.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <termios.h>
#include <unistd.h>

static int bars_for_terminal(int cols, int bar_width, int gap) {
  if (gap == 0)
    return std::max(1, cols / bar_width);
  return std::max(1, (cols + gap) / (bar_width + gap));
}

static int auto_bar_width(int cols, int gap) {
  int w = 2;
  if (bars_for_terminal(cols, w, gap) > 24) {
    ++w;
  }
  return w;
}

// How many 1/8 units of this cell are filled by a bar of the given height.
static int cell_units(float height, int content_row, int content_rows,
                      BarAlignment alignment) {
  switch (alignment) {
  case BarAlignment::end: {
    int row_bottom = (content_rows - 1 - content_row) * 8;
    return static_cast<int>(height) - row_bottom;
  }
  case BarAlignment::start: {
    int row_top = content_row * 8;
    return static_cast<int>(height) - row_top;
  }
  case BarAlignment::middle: {
    int center = content_rows / 2;
    float half = height / 2.0f;
    if (content_row < center) {
      int row_bottom = (center - 1 - content_row) * 8;
      return static_cast<int>(half) - row_bottom;
    }
    if (content_row > center) {
      int row_top = (content_row - center - 1) * 8;
      return static_cast<int>(half) - row_top;
    }
    return std::min(8, static_cast<int>(half));
  }
  }
  return 0;
}

static int baseline_row(int content_rows, BarAlignment alignment) {
  switch (alignment) {
  case BarAlignment::end:
    return content_rows - 1;
  case BarAlignment::start:
    return 0;
  case BarAlignment::middle:
    return content_rows / 2;
  }
  return -1;
}

static bool is_inverted(int pos, int size, BarAlignment alignment,
                        bool horizontal) {
  switch (alignment) {
  case BarAlignment::end:
    return horizontal;
  case BarAlignment::start:
    return !horizontal;
  case BarAlignment::middle:
    return horizontal ? pos < size / 2 : pos > size / 2;
  default:
    return false;
  }
}

// ─── Renderer ───────────────────────────────────────────────────────────────

class FullscreenRenderer : public Renderer {
public:
  void enter(const BalaclavaCliOptions &opts, int cols, int rows) override {
    opts_ = &opts;
    mpris_overlay_ = make_mpris_overlay(opts.mpris_mode);
    if (mpris_overlay_) {
      mpris_overlay_->attach(fb_);
      mpris_overlay_->enter(opts, cols, rows);
    }
    resolve(cols, rows);
    // Disable terminal echo and canonical mode, non-blocking reads
    tcgetattr(STDIN_FILENO, &orig_termios_);
    struct termios raw = orig_termios_;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    printf("\033[?1049h");
    printf("\033[?25l");
    printf("\033[?1000h\033[?1006h"); // SGR mouse tracking
    printf("\033[48;2;%d;%d;%dm", opts.bg_color.r, opts.bg_color.g,
           opts.bg_color.b);
    printf("\033[2J");
    fflush(stdout);
  }

  void leave() override {
    printf("\033[?1000l\033[?1006l"); // disable mouse tracking
    printf("\033[?25h");
    printf("\033[?1049l");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios_);
  }

  void resize(int cols, int rows) override {
    resolve(cols, rows);
    printf("\033[48;2;%d;%d;%dm", opts_->bg_color.r, opts_->bg_color.g,
           opts_->bg_color.b);
    printf("\033[2J");
    fflush(stdout);
    fb_full_ = true;
  }

  void render(BalaclavaCli &cli) override {
    ensure_fb();
    if (horizontal_)
      fill_horizontal(cli);
    else
      fill_vertical(cli);
    flush(cli);
  }

  int process_stdin(char *buf, int len) override {
    const bool hotkeys = opts_->hotkeys;
    const bool vol_scroll = opts_->volume_scroll;
    int consumed = 0;

    for (int i = 0; i < len; ++i) {
      // Plain space → play/pause
      if (buf[i] == ' ') {
        if (hotkeys) mpris_play_pause();
        consumed = i + 1;
        continue;
      }

      if (buf[i] != '\033') {
        consumed = i + 1;
        continue;
      }

      // Need at least \033[
      if (i + 1 >= len) break;
      if (buf[i + 1] != '[') { consumed = i + 2; continue; }
      if (i + 2 >= len) break;

      // SGR mouse: \033[<btn;col;rowM/m
      if (buf[i + 2] == '<') {
        int end = -1;
        for (int j = i + 3; j < len; ++j) {
          if (buf[j] == 'M' || buf[j] == 'm') { end = j; break; }
        }
        if (end < 0) break; // incomplete
        if (buf[end] == 'M') {
          int btn = 0, mcol = 0, mrow = 0;
          if (sscanf(buf + i + 3, "%d;%d;%d", &btn, &mcol, &mrow) == 3) {
            // btn 0 = left click
            if (btn == 0 && mpris_overlay_) {
              MprisHit hit = mpris_overlay_->hit_test(mrow, mcol);
              switch (hit) {
              case MprisHit::play_pause: mpris_play_pause(); break;
              case MprisHit::prev: mpris_previous(); break;
              case MprisHit::next: mpris_next(); break;
              case MprisHit::none: break;
              }
            }
            // btn 80 = ctrl+scroll up, btn 81 = ctrl+scroll down
            if (vol_scroll) {
              if (btn == 80) mpris_volume_up();
              else if (btn == 81) mpris_volume_down();
            }
          }
        }
        consumed = end + 1;
        continue;
      }

      // CSI sequences: \033[1;5A/B/C/D (ctrl+arrows)
      if (i + 4 < len && buf[i + 2] == '1' && buf[i + 3] == ';' &&
          buf[i + 4] == '5') {
        if (i + 5 >= len) break; // incomplete
        if (hotkeys) {
          switch (buf[i + 5]) {
          case 'C': mpris_next(); break;     // Ctrl+Right
          case 'D': mpris_previous(); break; // Ctrl+Left
          case 'A': mpris_volume_up(); break;   // Ctrl+Up
          case 'B': mpris_volume_down(); break; // Ctrl+Down
          default: break;
          }
        }
        consumed = i + 6;
        i = consumed - 1;
        continue;
      }

      // Unknown CSI — find end (letter byte)
      {
        int j = i + 2;
        while (j < len && (buf[j] < 0x40 || buf[j] > 0x7E)) ++j;
        if (j >= len) break; // incomplete
        consumed = j + 1;
        i = consumed - 1;
      }
    }

    if (consumed > 0 && consumed < len) {
      std::memmove(buf, buf + consumed, len - consumed);
      return len - consumed;
    }
    if (consumed > 0) return 0;
    // Keep partial escape at tail
    for (int i = len - 1; i >= 0; --i) {
      if (buf[i] == '\033') {
        if (i > 0) std::memmove(buf, buf + i, len - i);
        return len - i;
      }
    }
    return 0;
  }

private:
  const BalaclavaCliOptions *opts_ = nullptr;
  BarAlignment alignment_ = BarAlignment::end;
  BarAlignment alignment2_ = BarAlignment::end;
  bool baseline_ = false;
  bool baseline2_ = false;
  bool horizontal_ = false;

  FrameBuffer fb_;
  bool fb_full_ = true;
  struct termios orig_termios_ {};
  std::unique_ptr<MprisOverlay> mpris_overlay_;

  // ── framebuffer management ──

  void ensure_fb() {
    if (fb_.cols != cols_ || fb_.rows != rows_) {
      fb_.resize(cols_, rows_);
      fb_full_ = true;
    }
  }

  // ── fill: populate framebuffer from bar data ──

  void fill_vertical(const BalaclavaCli &cli) {
    const auto &opts = cli.opts;
    const auto &pad = opts.pad;
    const auto &values = cli.frame.bars;
    const auto &bg_values = cli.frame2.bars;
    const bool has_bg = !bg_values.empty();
    const int bar_count =
        std::min(static_cast<int>(values.size()), bars_);
    const int bg_bar_count = static_cast<int>(bg_values.size());
    const int content_rows = std::max(0, rows_ - pad.top - pad.bottom);
    const int content_cols = std::max(0, cols_ - pad.left - pad.right);
    const int total_units = content_rows * 8;
    const int used = bar_count * bar_width_ + std::max(0, bar_count - 1) * gap_;
    const int inner_margin = std::max(0, (content_cols - used) / 2);
    const bool center_split = has_bg && alignment_ == BarAlignment::middle &&
                              alignment2_ == BarAlignment::middle;
    const int center_row = content_rows / 2;

    for (int row = 0; row < rows_; ++row) {
      Cell *line = fb_.line(row);

      if (row < pad.top || row >= rows_ - pad.bottom) {
        std::fill(line, line + cols_, kEmpty);
        continue;
      }

      int content_row = row - pad.top;
      int col = 0;
      int left = std::min(pad.left + inner_margin, cols_);
      std::fill(line, line + left, kEmpty);
      col = left;

      int fg_base = baseline_ ? baseline_row(content_rows, alignment_) : -1;
      int bg_base = baseline2_ ? baseline_row(content_rows, alignment2_) : -1;

      for (int bar = 0; bar < bar_count && col < cols_; ++bar) {
        float fg_height =
            values[bar] * opts.headroom * static_cast<float>(total_units);
        float bg_height = 0;
        if (has_bg && bar < bg_bar_count)
          bg_height =
              bg_values[bar] * opts.headroom * static_cast<float>(total_units);

        int fg_u, bg_u;
        bool fg_inv, bg_inv;

        if (center_split) {
          int fg_offset = baseline_ ? 0 : 1;
          int bg_offset = baseline2_ ? 0 : 1;
          if (content_row <= center_row - fg_offset) {
            int upper_rows = center_row + 1 - fg_offset;
            float fg_h = values[bar] * opts.headroom *
                         static_cast<float>(upper_rows * 8);
            fg_u = static_cast<int>(fg_h) -
                   (center_row - fg_offset - content_row) * 8;
            fg_inv = false;
          } else {
            fg_u = 0;
            fg_inv = false;
          }
          if (content_row >= center_row + bg_offset) {
            int lower_rows = content_rows - center_row - bg_offset;
            float bg_h = (has_bg && bar < bg_bar_count)
                             ? bg_values[bar] * opts.headroom *
                                   static_cast<float>(lower_rows * 8)
                             : 0;
            bg_u = static_cast<int>(bg_h) -
                   (content_row - center_row - bg_offset) * 8;
            bg_inv = true;
          } else {
            bg_u = 0;
            bg_inv = false;
          }
          if (baseline_ && content_row == fg_base) {
            fg_u = fg_height > 0 ? 8 : 0;
            fg_inv = false;
          }
          if (baseline2_ && content_row == bg_base + (baseline_ ? 1 : 0)) {
            bg_u = bg_height > 0 ? 8 : 0;
            bg_inv = false;
          }
        } else {
          fg_u = cell_units(fg_height, content_row, content_rows, alignment_);
          fg_inv = is_inverted(content_row, content_rows, alignment_, false);
          bg_u = cell_units(bg_height, content_row, content_rows, alignment2_);
          bg_inv = is_inverted(content_row, content_rows, alignment2_, false);
          if (baseline_ && content_row == fg_base && fg_height > 0) {
            fg_u = 8;
            fg_inv = false;
          }
          if (baseline2_ && content_row == bg_base && bg_height > 0) {
            bg_u = 8;
            bg_inv = false;
          }
        }

        Cell c = make_cell(fg_u, fg_inv, bg_u, bg_inv);
        int w = std::min(bar_width_, cols_ - col);
        std::fill(line + col, line + col + w, c);
        col += bar_width_;

        if (bar < bar_count - 1 && gap_ > 0) {
          int g = std::min(gap_, std::max(0, cols_ - col));
          std::fill(line + col, line + col + g, kEmpty);
          col += gap_;
        }
      }

      if (col < cols_)
        std::fill(line + col, line + cols_, kEmpty);
    }
  }

  void fill_horizontal(const BalaclavaCli &cli) {
    const auto &opts = cli.opts;
    const auto &pad = opts.pad;
    const auto &values = cli.frame.bars;
    const auto &bg_values = cli.frame2.bars;
    const bool has_bg = !bg_values.empty();
    const int bar_count =
        std::min(static_cast<int>(values.size()), bars_);
    const int bg_bar_count = static_cast<int>(bg_values.size());
    const int content_rows = std::max(0, rows_ - pad.top - pad.bottom);
    const int content_cols = std::max(0, cols_ - pad.left - pad.right);
    const int total_units = content_cols * 8;
    const int bar_region =
        bar_count * bar_width_ + std::max(0, bar_count - 1) * gap_;
    const int top_margin = std::max(0, (content_rows - bar_region) / 2);
    const bool center_split = has_bg && alignment_ == BarAlignment::middle &&
                              alignment2_ == BarAlignment::middle;
    const int center_col = content_cols / 2;

    for (int row = 0; row < rows_; ++row) {
      Cell *line = fb_.line(row);

      if (row < pad.top || row >= rows_ - pad.bottom) {
        std::fill(line, line + cols_, kEmpty);
        continue;
      }

      int content_row = row - pad.top;
      int region_row = content_row - top_margin;

      int bar = -1;
      if (region_row >= 0 && region_row < bar_region) {
        int stride = bar_width_ + gap_;
        if (stride > 0) {
          int slot = region_row / stride;
          int pos_in_slot = region_row % stride;
          if (slot < bar_count && pos_in_slot < bar_width_)
            bar = slot;
        }
      }

      if (bar < 0) {
        std::fill(line, line + cols_, kEmpty);
        continue;
      }

      float fg_height =
          values[bar] * opts.headroom * static_cast<float>(total_units);
      float bg_height = 0;
      if (has_bg && bar < bg_bar_count)
        bg_height =
            bg_values[bar] * opts.headroom * static_cast<float>(total_units);

      int left = std::min(pad.left, cols_);
      std::fill(line, line + left, kEmpty);

      int fg_base = baseline_ ? baseline_row(content_cols, alignment_) : -1;
      int bg_base = baseline2_ ? baseline_row(content_cols, alignment2_) : -1;

      int max_content = std::max(0, cols_ - left);
      for (int col = 0; col < content_cols && col < max_content; ++col) {
        int fg_u, bg_u;
        bool fg_inv, bg_inv;

        if (center_split) {
          int fg_offset = baseline_ ? 0 : 1;
          int bg_offset = baseline2_ ? 0 : 1;
          if (col <= center_col - fg_offset) {
            int left_cols = center_col + 1 - fg_offset;
            float fg_h = values[bar] * opts.headroom *
                         static_cast<float>(left_cols * 8);
            fg_u =
                static_cast<int>(fg_h) - (center_col - fg_offset - col) * 8;
            fg_inv = true;
          } else {
            fg_u = 0;
            fg_inv = false;
          }
          if (col >= center_col + bg_offset) {
            int right_cols = content_cols - center_col - bg_offset;
            float bg_h = (has_bg && bar < bg_bar_count)
                             ? bg_values[bar] * opts.headroom *
                                   static_cast<float>(right_cols * 8)
                             : 0;
            bg_u =
                static_cast<int>(bg_h) - (col - center_col - bg_offset) * 8;
            bg_inv = false;
          } else {
            bg_u = 0;
            bg_inv = false;
          }
          if (baseline_ && col == fg_base) {
            fg_u = fg_height > 0 ? 8 : 0;
            fg_inv = false;
          }
          if (baseline2_ && col == bg_base + (baseline_ ? 1 : 0)) {
            bg_u = bg_height > 0 ? 8 : 0;
            bg_inv = false;
          }
        } else {
          fg_u = cell_units(fg_height, col, content_cols, alignment_);
          fg_inv = is_inverted(col, content_cols, alignment_, true);
          bg_u = cell_units(bg_height, col, content_cols, alignment2_);
          bg_inv = is_inverted(col, content_cols, alignment2_, true);
          if (baseline_ && col == fg_base && fg_height > 0) {
            fg_u = 8;
            fg_inv = false;
          }
          if (baseline2_ && col == bg_base && bg_height > 0) {
            bg_u = 8;
            bg_inv = false;
          }
        }

        line[left + col] = make_cell(fg_u, fg_inv, bg_u, bg_inv);
      }

      int tail = std::min(left + content_cols, cols_);
      std::fill(line + tail, line + cols_, kEmpty);
    }
  }

  // ── flush: diff framebuffer and emit minimal ANSI ──

  void flush(BalaclavaCli &cli) {
    auto &buf = cli.frame_buf;
    const auto &opts = cli.opts;
    const float beat = cli.frame.beat;
    const bool has_bg = !cli.frame2.bars.empty();

    buf.clear();

    // Set exclusion rect so framebuffer skips MPRIS overlay area
    if (mpris_overlay_) {
      fb_.excl_row = mpris_overlay_->prev_row_;
      fb_.excl_col = mpris_overlay_->prev_col_;
      fb_.excl_w = mpris_overlay_->prev_width_;
      fb_.excl_h = mpris_overlay_->prev_height_;
    } else {
      fb_.excl_w = 0;
    }

    const char **blk = horizontal_ ? hblocks : vblocks;
    fb_.flush(buf, fb_full_, horizontal_, has_bg, opts.pad, opts.bg_color,
              opts.colors, opts.bg_colors, blk,
              alignment_ == BarAlignment::start,
              alignment2_ == BarAlignment::start);

    if (mpris_overlay_)
      mpris_overlay_->render(cli);
    buf.append("\033[0m");

    fwrite(buf.data(), 1, buf.size(), stdout);
    fflush(stdout);

    fb_full_ = false;
  }

  // ── layout resolution ──

  void resolve(int cols, int rows) {
    cols_ = cols;
    rows_ = rows;
    int content_cols = cols_ - opts_->pad.left - opts_->pad.right;
    int content_rows = rows_ - opts_->pad.top - opts_->pad.bottom;

    if (opts_->orientation == Orientation::auto_) {
      float aspect = static_cast<float>(content_cols) /
                     std::max(1.0f, static_cast<float>(content_rows));
      horizontal_ = aspect <= 1.2f;
    } else {
      horizontal_ = opts_->orientation == Orientation::horizontal;
    }

    int layout_span = horizontal_ ? content_rows : content_cols;
    gap_ = (opts_->gap >= 0) ? opts_->gap : 1;
    bar_width_ = (opts_->bar_width > 0) ? opts_->bar_width
                                        : auto_bar_width(layout_span, gap_);
    bars_ = (opts_->bars > 0)
                ? opts_->bars
                : bars_for_terminal(layout_span, bar_width_, gap_);

    if (horizontal_) {
      alignment_ = opts_->alignment == BarAlignment::auto_ ? BarAlignment::start
                                                           : opts_->alignment;
      alignment2_ = opts_->alignment2 == BarAlignment::auto_
                        ? BarAlignment::start
                        : opts_->alignment2;
    } else {
      float adj_aspect = static_cast<float>(content_cols) /
                         std::max(1.0f, static_cast<float>(bar_width_) *
                                            static_cast<float>(content_rows));
      bool tall = adj_aspect < 2.0f;

      if (opts_->alignment == BarAlignment::auto_) {
        if (!tall) {
          alignment_ = BarAlignment::end;
        } else if (opts_->opts.has_secondary &&
                   opts_->opts.capture_sink != opts_->opts.capture_sink2) {
          alignment_ = opts_->opts.capture_sink ? BarAlignment::middle
                                                : BarAlignment::end;
        } else {
          alignment_ = BarAlignment::middle;
        }
      } else {
        alignment_ = opts_->alignment;
      }

      if (opts_->alignment2 == BarAlignment::auto_) {
        if (!tall) {
          alignment2_ = BarAlignment::end;
        } else if (opts_->opts.has_secondary &&
                   opts_->opts.capture_sink != opts_->opts.capture_sink2) {
          alignment2_ = opts_->opts.capture_sink2 ? BarAlignment::middle
                                                  : BarAlignment::end;
        } else {
          alignment2_ = BarAlignment::middle;
        }
      } else {
        alignment2_ = opts_->alignment2;
      }
    }

    auto resolve_baseline = [](Tristate t, BarAlignment a) -> bool {
      if (t == Tristate::on)
        return true;
      if (t == Tristate::off)
        return false;
      return a == BarAlignment::middle;
    };
    baseline_ = resolve_baseline(opts_->baseline, alignment_);
    baseline2_ = resolve_baseline(opts_->baseline2, alignment2_);

    if (mpris_overlay_) {
      mpris_overlay_->resize(cols, rows);
      mpris_overlay_->resolve(horizontal_, alignment_, alignment2_);
    }
  }
};

std::unique_ptr<Renderer> make_fullscreen_renderer() {
  return std::make_unique<FullscreenRenderer>();
}
