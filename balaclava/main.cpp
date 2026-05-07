#include "args/args.h"
#include "render/common.h"
#include <balaclava/balaclava.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <memory>

static balaclava::Balaclava *g_bala = nullptr;
static balaclava::Balaclava *g_bala2 = nullptr;
static std::atomic<bool> g_resized{false};

static void on_signal(int) {
  if (g_bala)
    g_bala->stop();
  if (g_bala2)
    g_bala2->stop();
}

static void on_winch(int) { g_resized.store(true, std::memory_order_relaxed); }

int main(int argc, char *argv[]) {
  const auto opts = parse_args(argc, argv);
  BalaclavaCli cli{opts, RenderState::resolve(opts)};

  if (opts.render_mode == RenderMode::fullscreen) {
    screen_enter();
  }

  balaclava::Options lib_opts = opts.opts;
  lib_opts.bars = cli.rs.bars;

  balaclava::Balaclava bala(lib_opts);
  g_bala = &bala;

  std::unique_ptr<balaclava::Balaclava> bala2;
  if (opts.opts.has_secondary) {
    balaclava::Options lib_opts2 = lib_opts;
    lib_opts2.target = opts.opts.target2;
    lib_opts2.capture_sink = opts.opts.capture_sink2;
    bala2 = std::make_unique<balaclava::Balaclava>(lib_opts2);
    g_bala2 = bala2.get();
  }

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);
  std::signal(SIGWINCH, on_winch);

  bala.start();
  if (bala2)
    bala2->start();

  auto last_mpris = std::chrono::steady_clock::now() - std::chrono::seconds(10);

  while (bala.poll(cli.frame)) {
    if (!opts.beat_detection)
      cli.frame.beat = 0.0f;
    if (bala2) {
      bala2->poll(cli.frame2);
      if (!opts.beat_detection2)
        cli.frame2.beat = 0.0f;
    }

    auto now = std::chrono::steady_clock::now();
    if (now - last_mpris >= std::chrono::seconds(2)) {
      cli.np = mpris_now_playing();
      last_mpris = now;
    }

    if (opts.render_mode == RenderMode::ascii) {
      render_ascii(cli.frame.bars);
    } else if (opts.render_mode == RenderMode::oneline) {
      render_oneline(cli.frame.bars, cli.rs.bar_width, cli.rs.gap);
    } else {
      if (g_resized.exchange(false)) {
        cli.rs = RenderState::resolve(opts);
        bala.setBars(cli.rs.bars);
        if (bala2)
          bala2->setBars(cli.rs.bars);
        printf("\033[2J");
        fflush(stdout);
      }
      render_fullscreen(cli);
    }
  }

  if (opts.render_mode == RenderMode::fullscreen) {
    screen_leave();
  } else {
    printf("\n");
  }

  return 0;
}
