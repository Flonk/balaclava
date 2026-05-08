#pragma once

#include "color.h"

#include <string>

inline uint8_t mix(uint8_t a, uint8_t b, float t) {
  return static_cast<uint8_t>(a + (b - a) * t);
}

// Fast uint8_t to decimal string, returns number of chars written.
inline int u8_to_dec(char *dst, uint8_t v) {
  if (v >= 100) {
    dst[0] = '0' + v / 100;
    dst[1] = '0' + (v / 10) % 10;
    dst[2] = '0' + v % 10;
    return 3;
  }
  if (v >= 10) {
    dst[0] = '0' + v / 10;
    dst[1] = '0' + v % 10;
    return 2;
  }
  dst[0] = '0' + v;
  return 1;
}

inline constexpr const char *kFgPrefix = "\033[38;2;";
inline constexpr const char *kBgPrefix = "\033[48;2;";

// Emit an ANSI color escape (FG or BG).
// prefix: kFgPrefix or kBgPrefix (7 chars)
inline void emit_color(std::string &buf, const char *prefix, Color c) {
  char tmp[24]; // "\033[38;2;255;255;255m" = 20 chars max
  int n = 0;
  for (int i = 0; i < 7; ++i)
    tmp[n++] = prefix[i];
  n += u8_to_dec(tmp + n, c.r);
  tmp[n++] = ';';
  n += u8_to_dec(tmp + n, c.g);
  tmp[n++] = ';';
  n += u8_to_dec(tmp + n, c.b);
  tmp[n++] = 'm';
  buf.append(tmp, n);
}

// 2D gradient: interpolates lo->hi by t, blends toward beat colors by beat
// t: position 0.0 to 1.0
// beat: beat intensity 0.0 to 1.0
struct Gradient2D {
  Color lo, hi;

  Color compute(float t) const {
    return {mix(lo.r, hi.r, t), mix(lo.g, hi.g, t), mix(lo.b, hi.b, t)};
  }
};
