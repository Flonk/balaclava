#pragma once

#include <cstddef>
#include <string>

namespace balaclava {

struct Options {
    // Audio
    std::string target = "@DEFAULT_SINK@";
    bool capture_sink = true;

    // Spectrum
    int bars = 40;
    double sample_rate = 48000;
    std::size_t frame_size = 2048;
    std::size_t hop_size = 512;
    double min_frequency = 20.0;
    double max_frequency = 12000.0;
    double dynamic_falloff_ms = 1000;
    double dynamic_rise_ms = 50;
    double auto_gain_floor = 0.01;

    // Effects
    double smoothing_alpha = 0.65;
    double gravity_decay = 0.95;
    double gravity_rise = 0.0;
    double gravity_power = 1.2;
    double noise_reduction = 0.0;
    bool monstercat = true;
    double monstercat_falloff = 1.6;
    double noise_floor_min_decay = 0.90;
    double noise_floor_max_decay = 0.9995;
    double noise_floor_min_rise = 0.002;
    double noise_floor_max_rise = 0.05;
    double eq_bass = 2.7;
    double eq_mid = 1.0;
    double eq_treble = 1.4;
    double contrast = 2.0;
    double noise_floor_clamp = 1e-4;

    // Beat detection
    double beat_decay = 0.9;        // beat intensity decay per frame
    double beat_gamma = 1.6;        // gamma curve on beat intensity (>1 suppresses weak beats)
    double beat_floor = 0.6;        // max beat floor for clean signals (0-1)
};

} // namespace balaclava
