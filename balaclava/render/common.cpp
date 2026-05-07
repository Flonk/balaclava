#include "../args/args.h"
#include "common.h"

#include <algorithm>
#include <sys/ioctl.h>
#include <unistd.h>

static void get_terminal_size(int &cols, int &rows) {
  struct winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 &&
      ws.ws_row > 0) {
    cols = ws.ws_col;
    rows = ws.ws_row;
  }
}

int bars_for_terminal(int cols, int bar_width, int gap) {
  if (gap == 0)
    return std::max(1, cols / bar_width);
  return std::max(1, (cols + gap) / (bar_width + gap));
}

int auto_bar_width(int cols, int gap) {
  int w = 2;
  while (bars_for_terminal(cols, w, gap) > 32) {
    ++w;
  }
  return w;
}

RenderState RenderState::resolve(const BalaclavaCliOptions &opts) {
  RenderState rs;
  get_terminal_size(rs.cols, rs.rows);

  int content_cols = rs.cols - opts.pad.left - opts.pad.right;

  rs.gap = (opts.gap >= 0)
               ? opts.gap
               : (opts.render_mode == RenderMode::fullscreen) ? 1 : 0;

  rs.bar_width = (opts.bar_width > 0)
                     ? opts.bar_width
                     : (opts.render_mode == RenderMode::fullscreen)
                           ? auto_bar_width(content_cols, rs.gap)
                           : 1;

  if (opts.bars > 0) {
    rs.bars = opts.bars;
  } else if (opts.render_mode == RenderMode::ascii) {
    rs.bars = 32;
  } else {
    int cols =
        (opts.render_mode == RenderMode::fullscreen) ? content_cols : rs.cols;
    rs.bars = bars_for_terminal(cols, rs.bar_width, rs.gap);
  }

  return rs;
}

void screen_enter() {
  printf("\033[?1049h");
  printf("\033[?25l");
  printf("\033[2J");
  fflush(stdout);
}

void screen_leave() {
  printf("\033[?25h");
  printf("\033[?1049l");
  fflush(stdout);
}
