#pragma once

#include <balaclava/options.h>
#include <cstdint>

enum class RenderMode { oneline, fullscreen, ascii };

struct Color {
  uint8_t r, g, b;
};

struct Args {
  balaclava::Options opts;
  RenderMode render_mode = RenderMode::fullscreen;
  int bar_width = 0; // 0 = auto
  int gap = -1;      // -1 = auto (0 for oneline, 1 for fullscreen)
  float headroom = 1.0f;
  int pad_top = 1;
  int pad_left = 1;
  int pad_bottom = 1;
  int pad_right = 1;

  // 2D gradient: bottom/top × no-beat/beat
  Color color_lo = {0x33, 0x33, 0x33};
  Color color_hi = {0x99, 0x99, 0x99};
  Color color_beat_lo = {0xE7, 0x8A, 0x53};
  Color color_beat_hi = {0xFB, 0xCB, 0x97};
};

Args parse_args(int argc, char *argv[]);
