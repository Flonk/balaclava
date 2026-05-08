#include "common.h"
#include "../args/args.h"

#include <sys/ioctl.h>
#include <unistd.h>

void get_terminal_size(int &cols, int &rows) {
  struct winsize ws{};
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 &&
      ws.ws_row > 0) {
    cols = ws.ws_col;
    rows = ws.ws_row;
  }
}

std::unique_ptr<Renderer> make_renderer(RenderMode mode) {
  switch (mode) {
  case RenderMode::fullscreen:
    return make_fullscreen_renderer();
  case RenderMode::oneline:
    return make_oneline_renderer();
  case RenderMode::ascii:
    return make_ascii_renderer();
  }
  return make_fullscreen_renderer();
}
