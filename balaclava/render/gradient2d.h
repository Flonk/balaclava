#pragma once

#include "../color.h"

#include <cstdio>
#include <string>

// 2D gradient: interpolates lo→hi by t, blends toward beat colors by beat
// fmt: printf format with 3 %d args (r, g, b)
// t: vertical position 0.0 (bottom) to 1.0 (top)
// beat: beat intensity 0.0 to 1.0
struct Gradient2D {
  Color lo, hi, beat_lo, beat_hi;

  void operator()(std::string &buf, const char *fmt, float t,
                  float beat) const {
    auto mix = [](uint8_t a, uint8_t b, float t) -> uint8_t {
      return static_cast<uint8_t>(a + (b - a) * t);
    };
    uint8_t r = mix(lo.r, hi.r, t);
    uint8_t g = mix(lo.g, hi.g, t);
    uint8_t b = mix(lo.b, hi.b, t);
    if (beat > 0.0f) {
      r = mix(r, mix(beat_lo.r, beat_hi.r, t), beat);
      g = mix(g, mix(beat_lo.g, beat_hi.g, t), beat);
      b = mix(b, mix(beat_lo.b, beat_hi.b, t), beat);
    }
    char color[24];
    int n = snprintf(color, sizeof(color), fmt, r, g, b);
    buf.append(color, n);
  }
};
