#pragma once

#include <cstdint>

struct Color {
  uint8_t r, g, b;

  bool operator==(const Color &o) const {
    return r == o.r && g == o.g && b == o.b;
  }
  bool operator!=(const Color &o) const { return !(*this == o); }
};
