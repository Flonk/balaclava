#include "args.h"
#include "mpris.h"
#include "render.h"
#include <balaclava/balaclava.h>

#include <atomic>
#include <chrono>
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
  Padding pad = {args.pad_top, args.pad_left, args.pad_bottom, args.pad_right};

  // Content area after padding
  auto content_cols = [&]() { return term.cols - pad.left - pad.right; };

  // Resolve auto gap: 0 for oneline, 1 for fullscreen
  int gap = args.gap;
  if (gap < 0) {
    gap = (args.render_mode == RenderMode::fullscreen) ? 1 : 0;
  }

  // Resolve auto bar width
  int bar_width = args.bar_width;
  if (bar_width == 0) {
    bar_width = (args.render_mode == RenderMode::fullscreen) ? auto_bar_width(content_cols(), gap) : 1;
  }

  // Auto bar count from terminal width unless explicitly set via --bars
  bool bars_auto = (args.opts.bars == 40); // default means auto
  if (bars_auto) {
    if (args.render_mode == RenderMode::ascii) {
      args.opts.bars = 32;
    } else {
      int cols = (args.render_mode == RenderMode::fullscreen) ? content_cols() : term.cols;
      args.opts.bars = bars_for_terminal(cols, bar_width, gap);
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

  NowPlaying np;
  auto last_mpris = std::chrono::steady_clock::now() - std::chrono::seconds(10);

  while (bala.poll(frame)) {
    // Poll MPRIS every 2 seconds
    auto now = std::chrono::steady_clock::now();
    if (now - last_mpris >= std::chrono::seconds(2)) {
      np = mpris_now_playing();
      last_mpris = now;
    }
    if (args.render_mode == RenderMode::ascii) {
      render_ascii(frame.bars);
    } else if (args.render_mode == RenderMode::oneline) {
      render_oneline(frame.bars, bar_width, gap);
    } else {
      if (g_resized.exchange(false)) {
        term = get_terminal_size();
        if (bars_auto) {
          bar_width = auto_bar_width(content_cols(), gap);
          int new_bars = bars_for_terminal(content_cols(), bar_width, gap);
          bala.setBars(new_bars);
        }
        printf("\033[2J");
        fflush(stdout);
      }
      render_fullscreen(frame.bars, term, bar_width, gap, args.headroom, frame.beat, grad, np, pad, frame_buf);
    }
  }

  if (args.render_mode == RenderMode::fullscreen) {
    screen_leave();
  } else {
    printf("\n");
  }

  return 0;
}
