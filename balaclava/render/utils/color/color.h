#pragma once

#include <cstdint>

struct Color {
  uint8_t r, g, b;

  bool operator==(const Color &o) const {
    return r == o.r && g == o.g && b == o.b;
  }
  bool operator!=(const Color &o) const { return !(*this == o); }
};

inline uint8_t mix(uint8_t a, uint8_t b, float t) {
  return static_cast<uint8_t>(a + (b - a) * t);
}
