#pragma once

#include "oklch.h"

struct Gradient2D {
  OklchColor lo, hi;

  // Construct from RGB colors (for default initializers / hex input).
  static Gradient2D from_rgb(Color lo, Color hi) {
    return {OklchColor::from_rgb(lo), OklchColor::from_rgb(hi)};
  }

  Color compute(float t) const { return oklch_mix(lo, hi, t).to_rgb(); }
};
