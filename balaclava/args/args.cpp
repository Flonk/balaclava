#include "args.h"
#include "../render/utils/color/oklch.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static Color parse_hex(const char *s) {
  ++s; // skip '#'
  unsigned int v = static_cast<unsigned int>(strtoul(s, nullptr, 16));
  return {static_cast<uint8_t>(v >> 16), static_cast<uint8_t>(v >> 8),
          static_cast<uint8_t>(v)};
}

// Parse "L C H" (space-separated floats) as OKLCH -> RGB.
static Color parse_oklch(const char *s) {
  char *end;
  float L = std::strtof(s, &end);
  float C = std::strtof(end, &end);
  float H = std::strtof(end, &end);
  return OklchColor{L, C, H}.to_rgb();
}

// Parse color: #rrggbb = hex, anything else = OKLCH "L C H".
static Color parse_color(const char *s) {
  if (s[0] == '#')
    return parse_hex(s);
  return parse_oklch(s);
}

// Parse color directly to OklchColor (for gradient endpoints).
static OklchColor parse_color_oklch(const char *s) {
  if (s[0] == '#')
    return OklchColor::from_rgb(parse_hex(s));
  char *end;
  float L = std::strtof(s, &end);
  float C = std::strtof(end, &end);
  float H = std::strtof(end, &end);
  return {L, C, H};
}

static void usage() {
  fprintf(stderr,
#include "usage.inc"
  );
}

BalaclavaCliOptions parse_args(int argc, char *argv[]) {
  BalaclavaCliOptions args;
  bool has_sink = false, has_source = false;
  bool has_sink2 = false, has_source2 = false;

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
    } else if (std::strcmp(argv[i], "--sink") == 0) {
      if (has_source) {
        fprintf(stderr, "--sink and --source are mutually exclusive\n");
        std::exit(1);
      }
      has_sink = true;
      args.opts.capture_sink = true;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        args.opts.target = argv[++i];
      else
        args.opts.target = "@DEFAULT_SINK@";
    } else if (std::strcmp(argv[i], "--source") == 0) {
      if (has_sink) {
        fprintf(stderr, "--sink and --source are mutually exclusive\n");
        std::exit(1);
      }
      has_source = true;
      args.opts.capture_sink = false;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        args.opts.target = argv[++i];
      else
        args.opts.target = "@DEFAULT_SOURCE@";
    } else if (std::strcmp(argv[i], "--sink2") == 0) {
      if (has_source2) {
        fprintf(stderr, "--sink2 and --source2 are mutually exclusive\n");
        std::exit(1);
      }
      has_sink2 = true;
      args.opts.has_secondary = true;
      args.opts.capture_sink2 = true;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        args.opts.target2 = argv[++i];
      else
        args.opts.target2 = "@DEFAULT_SINK@";
    } else if (std::strcmp(argv[i], "--source2") == 0) {
      if (has_sink2) {
        fprintf(stderr, "--sink2 and --source2 are mutually exclusive\n");
        std::exit(1);
      }
      has_source2 = true;
      args.opts.has_secondary = true;
      args.opts.capture_sink2 = false;
      if (i + 1 < argc && argv[i + 1][0] != '-')
        args.opts.target2 = argv[++i];
      else
        args.opts.target2 = "@DEFAULT_SOURCE@";
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
      args.bars = std::atoi(next());
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
    } else if (std::strcmp(argv[i], "--beat-detection") == 0) {
      const char *v = next();
      if (std::strcmp(v, "on") == 0)
        args.beat_detection = true;
      else if (std::strcmp(v, "off") == 0)
        args.beat_detection = false;
      else {
        fprintf(stderr, "--beat-detection expects on|off\n");
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--beat-detection2") == 0) {
      const char *v = next();
      if (std::strcmp(v, "on") == 0)
        args.beat_detection2 = true;
      else if (std::strcmp(v, "off") == 0)
        args.beat_detection2 = false;
      else {
        fprintf(stderr, "--beat-detection2 expects on|off\n");
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--beat-decay") == 0) {
      args.opts.beat_decay = std::atof(next());
    } else if (std::strcmp(argv[i], "--beat-gamma") == 0) {
      args.opts.beat_gamma = std::atof(next());
    } else if (std::strcmp(argv[i], "--beat-floor") == 0) {
      args.opts.beat_floor = std::atof(next());
    } else if (std::strcmp(argv[i], "--orientation") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0)
        args.orientation = Orientation::auto_;
      else if (std::strcmp(v, "vertical") == 0)
        args.orientation = Orientation::vertical;
      else if (std::strcmp(v, "horizontal") == 0)
        args.orientation = Orientation::horizontal;
      else {
        fprintf(stderr, "Unknown orientation: %s\n", v);
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--alignment") == 0 ||
               std::strcmp(argv[i], "--alignment2") == 0) {
      bool is2 = argv[i][11] == '2';
      const char *v = next();
      BarAlignment a;
      if (std::strcmp(v, "auto") == 0)
        a = BarAlignment::auto_;
      else if (std::strcmp(v, "end") == 0)
        a = BarAlignment::end;
      else if (std::strcmp(v, "start") == 0)
        a = BarAlignment::start;
      else if (std::strcmp(v, "middle") == 0)
        a = BarAlignment::middle;
      else {
        fprintf(stderr, "Unknown alignment: %s\n", v);
        std::exit(1);
      }
      if (is2)
        args.alignment2 = a;
      else
        args.alignment = a;
    } else if (std::strcmp(argv[i], "--baseline") == 0 ||
               std::strcmp(argv[i], "--baseline2") == 0) {
      bool is2 = argv[i][10] == '2';
      const char *v = next();
      Tristate t;
      if (std::strcmp(v, "on") == 0)
        t = Tristate::on;
      else if (std::strcmp(v, "off") == 0)
        t = Tristate::off;
      else if (std::strcmp(v, "auto") == 0)
        t = Tristate::auto_;
      else {
        fprintf(stderr, "--baseline expects on|off|auto\n");
        std::exit(1);
      }
      if (is2)
        args.baseline2 = t;
      else
        args.baseline = t;
    } else if (std::strcmp(argv[i], "--pad-top") == 0) {
      args.pad.top = std::atoi(next());
    } else if (std::strcmp(argv[i], "--pad-left") == 0) {
      args.pad.left = std::atoi(next());
    } else if (std::strcmp(argv[i], "--pad-bottom") == 0) {
      args.pad.bottom = std::atoi(next());
    } else if (std::strcmp(argv[i], "--pad-right") == 0) {
      args.pad.right = std::atoi(next());
    } else if (std::strcmp(argv[i], "--bg-color") == 0) {
      args.bg_color = parse_color(next());
    } else if (std::strcmp(argv[i], "--color-lo") == 0) {
      args.colors.lo = parse_color_oklch(next());
    } else if (std::strcmp(argv[i], "--color-hi") == 0) {
      args.colors.hi = parse_color_oklch(next());
    } else if (std::strcmp(argv[i], "--beat-color") == 0) {
      args.beat_color = parse_color(next());
    } else if (std::strcmp(argv[i], "--mpris-mode") == 0) {
      const char *v = next();
      if (std::strcmp(v, "off") == 0)
        args.mpris_mode = MprisMode::off;
      else if (std::strcmp(v, "text") == 0)
        args.mpris_mode = MprisMode::text;
      else if (std::strcmp(v, "full") == 0)
        args.mpris_mode = MprisMode::full;
      else {
        fprintf(stderr, "Unknown mpris mode: %s\n", v);
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--mpris-bg-color") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0) {
        args.mpris_bg_auto = true;
      } else {
        args.mpris_bg_auto = false;
        args.mpris_bg_color = parse_color(v);
      }
    } else if (std::strcmp(argv[i], "--mpris-bar-color") == 0) {
      args.mpris_bar_color = parse_color(next());
    } else if (std::strcmp(argv[i], "--mpris-width") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0)
        args.mpris_width = 0;
      else
        args.mpris_width = std::atoi(v);
    } else if (std::strcmp(argv[i], "--mpris-overflow") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0)
        args.mpris_overflow = MprisOverflow::auto_;
      else if (std::strcmp(v, "ellipsis") == 0)
        args.mpris_overflow = MprisOverflow::ellipsis;
      else if (std::strcmp(v, "marquee") == 0)
        args.mpris_overflow = MprisOverflow::marquee;
      else if (std::strcmp(v, "pingpong") == 0)
        args.mpris_overflow = MprisOverflow::pingpong;
      else {
        fprintf(stderr, "Unknown mpris overflow: %s\n", v);
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--mpris-text-align") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0)
        args.mpris_text_align = MprisTextAlign::auto_;
      else if (std::strcmp(v, "left") == 0)
        args.mpris_text_align = MprisTextAlign::left;
      else if (std::strcmp(v, "center") == 0)
        args.mpris_text_align = MprisTextAlign::center;
      else if (std::strcmp(v, "right") == 0)
        args.mpris_text_align = MprisTextAlign::right;
      else {
        fprintf(stderr, "Unknown mpris text align: %s\n", v);
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--mpris-padding-horizontal") == 0) {
      args.mpris_pad_h = std::atoi(next());
    } else if (std::strcmp(argv[i], "--mpris-padding-vertical") == 0) {
      args.mpris_pad_v = std::atoi(next());
    } else if (std::strcmp(argv[i], "--mpris-ui-color") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0) {
        args.mpris_ui_color_auto = true;
      } else {
        args.mpris_ui_color_auto = false;
        args.mpris_ui_color = parse_color(v);
      }
    } else if (std::strcmp(argv[i], "--mpris-color") == 0) {
      args.mpris_color = parse_color(next());
    } else if (std::strcmp(argv[i], "--mpris-position") == 0) {
      const char *v = next();
      if (std::strcmp(v, "auto") == 0)
        args.mpris_position = MprisPosition::auto_;
      else if (std::strcmp(v, "topleft") == 0)
        args.mpris_position = MprisPosition::topleft;
      else if (std::strcmp(v, "top") == 0)
        args.mpris_position = MprisPosition::top;
      else if (std::strcmp(v, "topright") == 0)
        args.mpris_position = MprisPosition::topright;
      else if (std::strcmp(v, "right") == 0)
        args.mpris_position = MprisPosition::right;
      else if (std::strcmp(v, "bottomright") == 0)
        args.mpris_position = MprisPosition::bottomright;
      else if (std::strcmp(v, "bottom") == 0)
        args.mpris_position = MprisPosition::bottom;
      else if (std::strcmp(v, "bottomleft") == 0)
        args.mpris_position = MprisPosition::bottomleft;
      else if (std::strcmp(v, "left") == 0)
        args.mpris_position = MprisPosition::left;
      else if (std::strcmp(v, "center") == 0)
        args.mpris_position = MprisPosition::center;
      else {
        fprintf(stderr, "Unknown mpris position: %s\n", v);
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--hotkeys") == 0) {
      const char *v = next();
      if (std::strcmp(v, "on") == 0)
        args.hotkeys = true;
      else if (std::strcmp(v, "off") == 0)
        args.hotkeys = false;
      else {
        fprintf(stderr, "--hotkeys expects on|off\n");
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--volume-scroll") == 0) {
      const char *v = next();
      if (std::strcmp(v, "on") == 0)
        args.volume_scroll = true;
      else if (std::strcmp(v, "off") == 0)
        args.volume_scroll = false;
      else {
        fprintf(stderr, "--volume-scroll expects on|off\n");
        std::exit(1);
      }
    } else if (std::strcmp(argv[i], "--color-lo2") == 0) {
      args.bg_colors.lo = parse_color_oklch(next());
    } else if (std::strcmp(argv[i], "--color-hi2") == 0) {
      args.bg_colors.hi = parse_color_oklch(next());
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      std::exit(1);
    }
  }

  return args;
}
