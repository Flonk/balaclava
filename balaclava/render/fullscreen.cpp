#include "../args/args.h"
#include "common.h"

#include <algorithm>

static constexpr const char *FG = "\033[38;2;%d;%d;%dm";
static constexpr const char *BG = "\033[48;2;%d;%d;%dm";

void render_fullscreen(BalaclavaCli &cli) {
  auto &buf = cli.frame_buf;
  const auto &opts = cli.opts;
  const auto &rs = cli.rs;
  const auto &pad = opts.pad;
  const auto &np = cli.np;

  const auto &values = cli.frame.bars;
  const float beat = cli.frame.beat;
  const auto &bg_values = cli.frame2.bars;
  const float bg_beat = cli.frame2.beat;
  const bool has_bg = !bg_values.empty();

  buf.clear();
  buf.append("\033[H");

  const int bars = static_cast<int>(values.size());
  const int bg_bar_count = static_cast<int>(bg_values.size());
  const int content_rows = rs.rows - pad.top - pad.bottom;
  const int content_cols = rs.cols - pad.left - pad.right;
  const int total_units = content_rows * 8;
  const int used = bars * rs.bar_width + std::max(0, bars - 1) * rs.gap;
  const int inner_margin = std::max(0, (content_cols - used) / 2);

  for (int row = 0; row < rs.rows; ++row) {
    if (row < pad.top || row >= rs.rows - pad.bottom) {
      buf.append(rs.cols, ' ');
    } else {
      int content_row = row - pad.top;
      int row_bottom = (content_rows - 1 - content_row) * 8;
      float row_t = static_cast<float>(content_row) /
                    std::max(1.0f, static_cast<float>(content_rows - 1));

      int left = pad.left + inner_margin;
      if (left > 0) {
        buf.append(left, ' ');
      }

      for (int bar = 0; bar < bars; ++bar) {
        float height =
            values[bar] * opts.headroom * static_cast<float>(total_units);
        int fg_units = static_cast<int>(height) - row_bottom;

        int bg_units = 0;
        if (has_bg && bar < bg_bar_count) {
          float bg_height =
              bg_values[bar] * opts.headroom * static_cast<float>(total_units);
          bg_units = static_cast<int>(bg_height) - row_bottom;
        }

        bool need_bg_reset = false;
        const char *ch;
        if (fg_units >= 8) {
          opts.colors(buf, FG, row_t, beat);
          ch = blocks[8];
        } else if (fg_units > 0) {
          opts.colors(buf, FG, row_t, beat);
          if (has_bg && bg_units > fg_units && bg_units >= 4) {
            opts.bg_colors(buf, BG, row_t, bg_beat);
            need_bg_reset = true;
          }
          ch = blocks[fg_units];
        } else if (has_bg && bg_units >= 8) {
          opts.bg_colors(buf, FG, row_t, bg_beat);
          ch = blocks[8];
        } else if (has_bg && bg_units > 0) {
          opts.bg_colors(buf, FG, row_t, bg_beat);
          ch = blocks[bg_units];
        } else {
          ch = blocks[0];
        }

        for (int w = 0; w < rs.bar_width; ++w) {
          buf.append(ch);
        }

        if (need_bg_reset) {
          buf.append("\033[49m");
        }

        if (bar < bars - 1 && rs.gap > 0) {
          buf.append(rs.gap, ' ');
        }
      }

      int remaining = rs.cols - left - used;
      if (remaining > 0) {
        buf.append("\033[0m");
        buf.append(remaining, ' ');
      }
    }

    if (row < rs.rows - 1) {
      buf.push_back('\n');
    }
  }

  // MPRIS overlay top-right, respecting padding
  if (!np.title.empty() && pad.top > 0) {
    std::string label =
        np.artist.empty() ? np.title : np.artist + " \xe2\x80\x94 " + np.title;
    int len = static_cast<int>(label.size());
    int max_len = rs.cols - pad.left - pad.right;
    if (len > max_len) {
      label = label.substr(0, max_len);
      len = max_len;
    }
    int col = rs.cols - pad.right - len + 1;
    char pos[16];
    int pn = snprintf(pos, sizeof(pos), "\033[%d;%dH", pad.top + 1, col);
    buf.append(pos, pn);
    buf.append("\033[38;2;80;80;80m");
    buf.append(label);
  }

  buf.append("\033[0m");
  fwrite(buf.data(), 1, buf.size(), stdout);
  fflush(stdout);
}
