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
    double max_frequency = 20000.0;
    double dynamic_falloff = 0.5;
    double dynamic_rise = 0.0005;
    double auto_gain_floor = 0.01;

    // Effects
    double smoothing_alpha = 0.7;
    double gravity_decay = 0.9;
    double noise_reduction = 0.5;
    bool monstercat = true;
    double noise_floor_min_decay = 0.90;
    double noise_floor_max_decay = 0.9995;
    double noise_floor_min_rise = 0.002;
    double noise_floor_max_rise = 0.05;
    double noise_floor_clamp = 1e-4;
};

} // namespace balaclava
