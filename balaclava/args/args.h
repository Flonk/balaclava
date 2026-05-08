#pragma once

#include "../render/common.h"
#include "../render/utils/color/gradient2d.h"
#include "../render/utils/color/oklch.h"
#include <balaclava/options.h>

enum class RenderMode { oneline, fullscreen, ascii };
enum class BarAlignment { auto_, end, start, middle };
enum class Orientation { auto_, vertical, horizontal };
enum class Tristate { auto_, on, off };
enum class MprisPosition {
  auto_, topleft, top, topright, right, bottomright, bottom, bottomleft, left,
  center
};
enum class MprisMode { off, text, full };
enum class MprisOverflow { auto_, ellipsis, marquee, pingpong };
enum class MprisTextAlign { auto_, left, center, right };

struct BalaclavaCliOptions {
  balaclava::Options opts;
  RenderMode render_mode = RenderMode::fullscreen;
  int bar_width = 0;  // 0 = auto
  int gap = -1;       // -1 = auto
  int bars = 0;       // 0 = auto
  float headroom = 1.0f;
  Padding pad = {1, 1, 1, 1};
  bool beat_detection = true;
  bool beat_detection2 = false;
  Orientation orientation = Orientation::auto_;
  BarAlignment alignment = BarAlignment::auto_;
  BarAlignment alignment2 = BarAlignment::auto_;
  Tristate baseline = Tristate::auto_;
  Tristate baseline2 = Tristate::auto_;
  Color bg_color = OklchColor{0.f, 0.f, 0.f}.to_rgb();
  MprisMode mpris_mode = MprisMode::text;
  MprisPosition mpris_position = MprisPosition::auto_;
  Color mpris_color = OklchColor{0.5f, 0.f, 0.f}.to_rgb();
  bool mpris_ui_color_auto = true;
  Color mpris_ui_color = {};
  bool mpris_bg_auto = true;         // true = use bg_color
  Color mpris_bg_color = OklchColor{0.f, 0.f, 0.f}.to_rgb();
  Color mpris_bar_color = OklchColor{0.4f, 0.f, 0.f}.to_rgb();
  int mpris_width = 0; // 0 = auto
  MprisOverflow mpris_overflow = MprisOverflow::auto_;
  MprisTextAlign mpris_text_align = MprisTextAlign::auto_;
  int mpris_pad_h = 0;
  int mpris_pad_v = 0;
  bool hotkeys = true;
  bool volume_scroll = true;
  Color beat_color = OklchColor{0.88f, 0.1f, 70.f}.to_rgb();
  Gradient2D colors = {{0.3f, 0.f, 0.f}, {0.7f, 0.f, 0.f}};
  Gradient2D bg_colors = {{0.28f, 0.03f, 280.f}, {0.15f, 0.02f, 280.f}};
};

BalaclavaCliOptions parse_args(int argc, char *argv[]);
