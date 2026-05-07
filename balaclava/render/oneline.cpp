#include "common.h"

#include <algorithm>

void render_oneline(const std::vector<float> &values, int bar_width, int gap) {
  printf("\r");
  for (int i = 0; i < static_cast<int>(values.size()); ++i) {
    int height = static_cast<int>(values[i] * 8.0f);
    const char *ch = blocks[std::clamp(height, 0, 8)];
    for (int w = 0; w < bar_width; ++w) {
      printf("%s", ch);
    }
    if (i < static_cast<int>(values.size()) - 1 && gap > 0) {
      printf("%*s", gap, "");
    }
  }
  fflush(stdout);
}
