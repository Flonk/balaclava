#include "args.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static Color parse_hex(const char *s) {
  // Skip optional '#'
  if (s[0] == '#')
    ++s;
  unsigned int v = static_cast<unsigned int>(strtoul(s, nullptr, 16));
  return {static_cast<uint8_t>(v >> 16), static_cast<uint8_t>(v >> 8),
          static_cast<uint8_t>(v)};
}

static void usage() {
  fprintf(
      stderr,
      "Usage: balaclava [OPTIONS] [TARGET]\n"
      "\n"
      "TARGET is a PipeWire sink/source name (default: @DEFAULT_SINK@)\n"
      "\n"
      "Options:\n"
      "  --source [TARGET]       Capture from a source instead of a sink\n"
      "  --render oneline|fullscreen|ascii\n"
      "                          Render mode (default: fullscreen)\n"
      "  --bars N                Number of bars (default: auto)\n"
      "  --bar-width N           Bar width in chars (default: auto)\n"
      "  --gap N                 Gap between bars in chars (default: auto)\n"
      "  --headroom N            Max bar height 0-1 (default: 0.8)\n"
      "  --sample-rate N         Sample rate in Hz (default: 48000)\n"
      "  --frame-size N          FFT frame size (default: 2048)\n"
      "  --hop-size N            FFT hop size (default: 512)\n"
      "  --min-freq N            Min frequency in Hz (default: 20)\n"
      "  --max-freq N            Max frequency in Hz (default: 20000)\n"
      "  --dynamic-falloff N     Peak hold half-life in ms (default: 1000)\n"
      "  --dynamic-rise N        Rise half-life in ms (default: 50)\n"
      "  --auto-gain-floor N     Auto gain floor (default: 0.01)\n"
      "  --smoothing N           Smoothing alpha 0-1 (default: 0.65)\n"
      "  --gravity N             Gravity decay 0-1 (default: 0.93)\n"
      "  --gravity-rise N        Rise inertia 0-1 (default: 0.0)\n"
      "  --gravity-power N       Gravity easing exponent (default: 6.0)\n"
      "  --noise-reduction N     Noise reduction 0-1 (default: 1.0)\n"
      "  --eq-bass N             Bass gain at min freq (default: 3.0)\n"
      "  --eq-mid N              Mid gain at center (default: 0.8)\n"
      "  --eq-treble N           Treble gain at max freq (default: 1.8)\n"
      "  --contrast N            Gamma curve exponent (default: 1.5)\n"
      "  --monstercat-falloff N  Monstercat falloff factor (default: 1.5)\n"
      "  --no-monstercat         Disable monstercat smoothing\n"
      "  --beat-decay N          Beat intensity decay 0-1 (default: 0.9)\n"
      "  --beat-gamma N          Beat intensity gamma (default: 1.6)\n"
      "  --beat-floor N          Max beat floor for clean signals (default: "
      "0.6)\n"
      "  --pad-top N             Top padding in rows (default: 1)\n"
      "  --pad-left N            Left padding in cols (default: 1)\n"
      "  --pad-bottom N          Bottom padding in rows (default: 1)\n"
      "  --pad-right N           Right padding in cols (default: 1)\n"
      "  --color-lo HEX          Bottom color (default: 333333)\n"
      "  --color-hi HEX          Top color (default: 999999)\n"
      "  --color-beat-lo HEX     Bottom color on beat (default: E78A53)\n"
      "  --color-beat-hi HEX     Top color on beat (default: FBCB97)\n"
      "  -h, --help              Show this help\n");
}

Args parse_args(int argc, char *argv[]) {
  Args args;
  for (int i = 1; i < argc; ++i) {
    auto next = [&]() -> const char * {
      if (i + 1 >= argc) {
        fprintf(stderr, "Missing value for %s\n", argv[i]);
        std::exit(1);
      }
      return argv[++i];
    };

    if (std::strcmp(argv[i], "-h") == 0 ||
        std::strcmp(argv[i], "--help") == 0) {
      usage();
      std::exit(0);
    } else if (std::strcmp(argv[i], "--source") == 0) {
      args.opts.capture_sink = false;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        args.opts.target = argv[++i];
      else
        args.opts.target = "@DEFAULT_SOURCE@";
    } else if (std::strcmp(argv[i], "--render") == 0) {
      const char *v = next();
      if (std::strcmp(v, "oneline") == 0)
        args.render_mode = RenderMode::oneline;
      else if (std::strcmp(v, "fullscreen") == 0)
        args.render_mode = RenderMode::fullscreen;
      else if (std::strcmp(v, "ascii") == 0)
        args.render_mode = RenderMode::ascii;
      else {
        fprintf(stderr, "Unknown render mode: %s\n", v);
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--bars") == 0) {
      args.opts.bars = std::atoi(next());
    } else if (std::strcmp(argv[i], "--bar-width") == 0) {
      args.bar_width = std::atoi(next());
    } else if (std::strcmp(argv[i], "--gap") == 0) {
      args.gap = std::atoi(next());
    } else if (std::strcmp(argv[i], "--headroom") == 0) {
      args.headroom = static_cast<float>(std::atof(next()));
    } else if (std::strcmp(argv[i], "--sample-rate") == 0) {
      args.opts.sample_rate = std::atof(next());
    } else if (std::strcmp(argv[i], "--frame-size") == 0) {
      args.opts.frame_size = std::atol(next());
    } else if (std::strcmp(argv[i], "--hop-size") == 0) {
      args.opts.hop_size = std::atol(next());
    } else if (std::strcmp(argv[i], "--min-freq") == 0) {
      args.opts.min_frequency = std::atof(next());
    } else if (std::strcmp(argv[i], "--max-freq") == 0) {
      args.opts.max_frequency = std::atof(next());
    } else if (std::strcmp(argv[i], "--dynamic-falloff") == 0) {
      args.opts.dynamic_falloff_ms = std::atof(next());
    } else if (std::strcmp(argv[i], "--dynamic-rise") == 0) {
      args.opts.dynamic_rise_ms = std::atof(next());
    } else if (std::strcmp(argv[i], "--auto-gain-floor") == 0) {
      args.opts.auto_gain_floor = std::atof(next());
    } else if (std::strcmp(argv[i], "--smoothing") == 0) {
      args.opts.smoothing_alpha = std::atof(next());
    } else if (std::strcmp(argv[i], "--gravity") == 0) {
      args.opts.gravity_decay = std::atof(next());
    } else if (std::strcmp(argv[i], "--gravity-rise") == 0) {
      args.opts.gravity_rise = std::atof(next());
    } else if (std::strcmp(argv[i], "--gravity-power") == 0) {
      args.opts.gravity_power = std::atof(next());
    } else if (std::strcmp(argv[i], "--noise-reduction") == 0) {
      args.opts.noise_reduction = std::atof(next());
    } else if (std::strcmp(argv[i], "--eq-bass") == 0) {
      args.opts.eq_bass = std::atof(next());
    } else if (std::strcmp(argv[i], "--eq-mid") == 0) {
      args.opts.eq_mid = std::atof(next());
    } else if (std::strcmp(argv[i], "--eq-treble") == 0) {
      args.opts.eq_treble = std::atof(next());
    } else if (std::strcmp(argv[i], "--contrast") == 0) {
      args.opts.contrast = std::atof(next());
    } else if (std::strcmp(argv[i], "--monstercat-falloff") == 0) {
      args.opts.monstercat_falloff = std::atof(next());
    } else if (std::strcmp(argv[i], "--no-monstercat") == 0) {
      args.opts.monstercat = false;
    } else if (std::strcmp(argv[i], "--beat-decay") == 0) {
      args.opts.beat_decay = std::atof(next());
    } else if (std::strcmp(argv[i], "--beat-gamma") == 0) {
      args.opts.beat_gamma = std::atof(next());
    } else if (std::strcmp(argv[i], "--beat-floor") == 0) {
      args.opts.beat_floor = std::atof(next());
    } else if (std::strcmp(argv[i], "--pad-top") == 0) {
      args.pad_top = std::atoi(next());
    } else if (std::strcmp(argv[i], "--pad-left") == 0) {
      args.pad_left = std::atoi(next());
    } else if (std::strcmp(argv[i], "--pad-bottom") == 0) {
      args.pad_bottom = std::atoi(next());
    } else if (std::strcmp(argv[i], "--pad-right") == 0) {
      args.pad_right = std::atoi(next());
    } else if (std::strcmp(argv[i], "--color-lo") == 0) {
      args.color_lo = parse_hex(next());
    } else if (std::strcmp(argv[i], "--color-hi") == 0) {
      args.color_hi = parse_hex(next());
    } else if (std::strcmp(argv[i], "--color-beat-lo") == 0) {
      args.color_beat_lo = parse_hex(next());
    } else if (std::strcmp(argv[i], "--color-beat-hi") == 0) {
      args.color_beat_hi = parse_hex(next());
    } else if (argv[i][0] == '-') {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      std::exit(1);
    } else {
      args.opts.target = argv[i];
    }
  }

  return args;
}
