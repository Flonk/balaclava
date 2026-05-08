#include "args/args.h"
#include "render/common.h"
#include <balaclava/balaclava.h>

#include <atomic>
#include <chrono>
#include <clocale>
#include <csignal>
#include <cstdio>
#include <memory>
#include <unistd.h>

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
  std::setlocale(LC_CTYPE, "");
  const auto opts = parse_args(argc, argv);
  BalaclavaCli cli{opts};
  auto renderer = make_renderer(opts.render_mode);

  int cols = 80, rows = 24;
  get_terminal_size(cols, rows);
  renderer->enter(opts, cols, rows);

  balaclava::Options lib_opts = opts.opts;
  lib_opts.bars = renderer->bars();

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

  const bool mpris_enabled = opts.mpris_mode != MprisMode::off;
  auto last_mpris = std::chrono::steady_clock::now() - std::chrono::seconds(10);
  auto last_resize = std::chrono::steady_clock::time_point{};
  bool resize_pending = false;

  char stdin_buf[128];
  int stdin_len = 0;

  while (bala.poll(cli.frame)) {
    if (!opts.beat_detection)
      cli.frame.beat = 0.0f;
    if (bala2) {
      bala2->poll(cli.frame2);
      if (!opts.beat_detection2)
        cli.frame2.beat = 0.0f;
    }

    auto now = std::chrono::steady_clock::now();

    // MPRIS poll
    if (mpris_enabled && now - last_mpris >= std::chrono::seconds(2)) {
      cli.np = mpris_now_playing();
      cli.mpris_pb.poll_position_us = cli.np.position_us;
      cli.mpris_pb.poll_time = now;
      cli.mpris_pb.length_us = cli.np.length_us;
      cli.mpris_pb.playing = (cli.np.playback_status == "Playing");
      cli.mpris_pb.paused = (cli.np.playback_status == "Paused");
      last_mpris = now;
    }

    // Interpolate position
    if (cli.mpris_pb.playing && cli.mpris_pb.poll_position_us >= 0) {
      auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
                             now - cli.mpris_pb.poll_time)
                             .count();
      cli.mpris_pb.position_us = cli.mpris_pb.poll_position_us + elapsed_us;
      if (cli.mpris_pb.length_us > 0)
        cli.mpris_pb.position_us =
            std::min(cli.mpris_pb.position_us, cli.mpris_pb.length_us);
    } else {
      cli.mpris_pb.position_us = cli.mpris_pb.poll_position_us;
    }

    // Non-blocking stdin read
    int n = read(STDIN_FILENO, stdin_buf + stdin_len,
                 sizeof(stdin_buf) - static_cast<size_t>(stdin_len));
    if (n > 0) {
      stdin_len += n;
      stdin_len = renderer->process_stdin(stdin_buf, stdin_len);
    }

    if (g_resized.exchange(false)) {
      get_terminal_size(cols, rows);
      renderer->resize(cols, rows);
      resize_pending = true;
    }

    if (resize_pending &&
        now - last_resize >= std::chrono::milliseconds(100)) {
      bala.setBars(renderer->bars());
      if (bala2)
        bala2->setBars(renderer->bars());
      last_resize = now;
      resize_pending = false;
    }

    renderer->render(cli);
  }

  renderer->leave();

  return 0;
}
