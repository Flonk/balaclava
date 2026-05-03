#include "args.h"
#include "render.h"
#include <balaclava/balaclava.h>

#include <atomic>
#include <csignal>
#include <cstdio>
#include <string>
#include <vector>

static balaclava::Balaclava *g_bala = nullptr;
static std::atomic<bool> g_resized{false};

static void on_signal(int) {
  if (g_bala)
    g_bala->stop();
}

static void on_winch(int) {
  g_resized.store(true, std::memory_order_relaxed);
}

int main(int argc, char *argv[]) {
  auto args = parse_args(argc, argv);

  Terminal term = get_terminal_size();

  // Resolve auto gap: 0 for oneline, 1 for fullscreen
  int gap = args.gap;
  if (gap < 0) {
    gap = (args.render_mode == RenderMode::fullscreen) ? 1 : 0;
  }

  // Resolve auto bar width
  int bar_width = args.bar_width;
  if (bar_width == 0) {
    bar_width = (args.render_mode == RenderMode::fullscreen) ? auto_bar_width(term.cols, gap) : 1;
  }

  // Auto bar count from terminal width unless explicitly set via --bars
  bool bars_auto = (args.opts.bars == 40); // default means auto
  if (bars_auto) {
    if (args.render_mode == RenderMode::ascii) {
      args.opts.bars = 32;
    } else {
      args.opts.bars = bars_for_terminal(term.cols, bar_width, gap);
    }
  }

  if (args.render_mode == RenderMode::fullscreen) {
    screen_enter();
  }

  balaclava::Balaclava bala(args.opts);
  g_bala = &bala;

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);
  std::signal(SIGWINCH, on_winch);

  bala.start();

  balaclava::Baclava frame;
  std::string frame_buf;
  Gradient grad = {args.color_lo, args.color_hi, args.color_beat_lo, args.color_beat_hi};

  while (bala.poll(frame)) {
    if (args.render_mode == RenderMode::ascii) {
      render_ascii(frame.bars);
    } else if (args.render_mode == RenderMode::oneline) {
      render_oneline(frame.bars, bar_width, gap);
    } else {
      if (g_resized.exchange(false)) {
        term = get_terminal_size();
        if (bars_auto) {
          bar_width = auto_bar_width(term.cols, gap);
          int new_bars = bars_for_terminal(term.cols, bar_width, gap);
          bala.setBars(new_bars);
        }
        printf("\033[2J");
        fflush(stdout);
      }
      render_fullscreen(frame.bars, term, bar_width, gap, args.headroom, frame.beat, grad, frame_buf);
    }
  }

  if (args.render_mode == RenderMode::fullscreen) {
    screen_leave();
  } else {
    printf("\n");
  }

  return 0;
}
