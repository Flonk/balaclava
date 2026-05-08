#pragma once

#include "../render/common.h"
#include "../render/gradient2d.h"
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
  Color bg_color = {0, 0, 0};
  MprisMode mpris_mode = MprisMode::text;
  MprisPosition mpris_position = MprisPosition::auto_;
  Color mpris_color = {0x66, 0x66, 0x66};
  bool mpris_ui_color_auto = true;
  Color mpris_ui_color = {};
  bool mpris_bg_auto = true;         // true = use bg_color
  Color mpris_bg_color = {0, 0, 0};  // only used when mpris_bg_auto is false
  Color mpris_bar_color = {0x50, 0x50, 0x50};
  int mpris_width = 0; // 0 = auto
  MprisOverflow mpris_overflow = MprisOverflow::auto_;
  MprisTextAlign mpris_text_align = MprisTextAlign::auto_;
  int mpris_pad_h = 0;
  int mpris_pad_v = 0;
  bool hotkeys = true;
  bool volume_scroll = true;
  Color beat_color = {0xFB, 0xCB, 0x97};
  Gradient2D colors = {{0x33, 0x33, 0x33}, {0x99, 0x99, 0x99}};
  Gradient2D bg_colors = {{0x18, 0x18, 0x22}, {0x30, 0x30, 0x44}};
};

BalaclavaCliOptions parse_args(int argc, char *argv[]);
