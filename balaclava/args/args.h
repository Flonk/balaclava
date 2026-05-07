#pragma once

#include "../render/common.h"
#include "../render/gradient2d.h"
#include <balaclava/options.h>

enum class RenderMode { oneline, fullscreen, ascii };

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
  Gradient2D colors = {{0x33, 0x33, 0x33},
                        {0x99, 0x99, 0x99},
                        {0xE7, 0x8A, 0x53},
                        {0xFB, 0xCB, 0x97}};
  Gradient2D bg_colors = {{0x18, 0x18, 0x22},
                           {0x30, 0x30, 0x44},
                           {0xA0, 0x60, 0x3A},
                           {0xC0, 0x90, 0x60}};
};

BalaclavaCliOptions parse_args(int argc, char *argv[]);
