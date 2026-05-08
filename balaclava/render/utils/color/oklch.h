#pragma once

#include "color.h"

#include <algorithm>
#include <cmath>

struct OklchColor {
  float L, C, h; // L: 0-1, C: 0-~0.4, h: 0-360

  static OklchColor from_rgb(Color c) {
    // sRGB -> linear
    auto to_lin = [](uint8_t v) {
      float f = v / 255.f;
      return f <= 0.04045f ? f / 12.92f
                           : std::pow((f + 0.055f) / 1.055f, 2.4f);
    };
    float r = to_lin(c.r), g = to_lin(c.g), b = to_lin(c.b);

    // Linear RGB -> LMS (via OKLab matrix)
    float l = 0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b;
    float m = 0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b;
    float s = 0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b;

    // Cube root
    float l_ = std::cbrt(l), m_ = std::cbrt(m), s_ = std::cbrt(s);

    // LMS -> OKLab
    float L_ = 0.2104542553f * l_ + 0.7936177850f * m_ - 0.0040720468f * s_;
    float a_ = 1.9779984951f * l_ - 2.4285922050f * m_ + 0.4505937099f * s_;
    float b_ = 0.0259040371f * l_ + 0.7827717662f * m_ - 0.8086757660f * s_;

    // OKLab -> OKLCH
    float C_ = std::sqrt(a_ * a_ + b_ * b_);
    float h_ = std::atan2(b_, a_) * (180.f / 3.14159265358979f);
    if (h_ < 0.f)
      h_ += 360.f;

    return {L_, C_, h_};
  }

  Color to_rgb() const {
    // OKLCH -> OKLab
    float hr = h * (3.14159265358979f / 180.f);
    float a_ = C * std::cos(hr);
    float b_ = C * std::sin(hr);

    // OKLab -> LMS (cube roots)
    float l_ = L + 0.3963377774f * a_ + 0.2158037573f * b_;
    float m_ = L - 0.1055613458f * a_ - 0.0638541728f * b_;
    float s_ = L - 0.0894841775f * a_ - 1.2914855480f * b_;

    // Cube
    float l = l_ * l_ * l_, m = m_ * m_ * m_, s = s_ * s_ * s_;

    // LMS -> linear RGB
    float r = +4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s;
    float g = -1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s;
    float b = -0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s;

    // linear -> sRGB
    auto to_srgb = [](float v) -> uint8_t {
      v = std::clamp(v, 0.f, 1.f);
      float s = v <= 0.0031308f ? v * 12.92f
                                : 1.055f * std::pow(v, 1.f / 2.4f) - 0.055f;
      return static_cast<uint8_t>(std::round(s * 255.f));
    };

    return {to_srgb(r), to_srgb(g), to_srgb(b)};
  }
};

// Interpolate in OKLCH space with shortest-arc hue.
inline OklchColor oklch_mix(OklchColor a, OklchColor b, float t) {
  float L = a.L + (b.L - a.L) * t;
  float C = a.C + (b.C - a.C) * t;

  // Shortest-arc hue interpolation
  float dh = b.h - a.h;
  if (dh > 180.f)
    dh -= 360.f;
  else if (dh < -180.f)
    dh += 360.f;
  float h = a.h + dh * t;
  if (h < 0.f)
    h += 360.f;
  else if (h >= 360.f)
    h -= 360.f;

  // If either endpoint is achromatic, use the other's hue
  if (a.C < 1e-6f)
    h = b.h;
  else if (b.C < 1e-6f)
    h = a.h;

  return {L, C, h};
}
