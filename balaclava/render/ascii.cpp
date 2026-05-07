#include "common.h"

#include <algorithm>

void render_ascii(const std::vector<float> &values) {
  static const char levels[] = "0123456789abcdefghijklmnopqrstuv";
  for (const float &v : values) {
    int idx = static_cast<int>(v * 31.0f);
    putchar(levels[std::clamp(idx, 0, 31)]);
  }
  putchar('\n');
  fflush(stdout);
}
