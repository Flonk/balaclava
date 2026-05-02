#include <balaclava/visualizereffects.h>

#include <algorithm>
#include <cmath>
#include <vector>

namespace balaclava {

VisualizerEffects::VisualizerEffects(const Options& opts)
    : m_smoothingAlpha(static_cast<float>(opts.smoothing_alpha))
    , m_gravityDecay(static_cast<float>(opts.gravity_decay))
    , m_gravityRise(static_cast<float>(opts.gravity_rise))
    , m_gravityPower(static_cast<float>(opts.gravity_power))
    , m_noiseReduction(static_cast<float>(opts.noise_reduction))
    , m_contrast(static_cast<float>(opts.contrast))
    , m_eqBass(static_cast<float>(opts.eq_bass))
    , m_eqMid(static_cast<float>(opts.eq_mid))
    , m_eqTreble(static_cast<float>(opts.eq_treble))
    , m_monstercat(opts.monstercat)
    , m_monstercatFalloff(static_cast<float>(opts.monstercat_falloff))
    , m_noiseFloorMinDecay(static_cast<float>(opts.noise_floor_min_decay))
    , m_noiseFloorMaxDecay(static_cast<float>(opts.noise_floor_max_decay))
    , m_noiseFloorMinRise(static_cast<float>(opts.noise_floor_min_rise))
    , m_noiseFloorMaxRise(static_cast<float>(opts.noise_floor_max_rise))
    , m_noiseFloorClamp(static_cast<float>(opts.noise_floor_clamp))
{}

void VisualizerEffects::process(std::vector<float>& values) {
    if (values.empty()) {
        return;
    }

    ensureBuffers(values.size());

    if (m_eqBass != 1.0f || m_eqMid != 1.0f || m_eqTreble != 1.0f) {
        // Quadratic through (0, bass), (0.5, mid), (1.0, treble) in log-freq space
        const float a = 2.0f * m_eqBass - 4.0f * m_eqMid + 2.0f * m_eqTreble;
        const float b = -3.0f * m_eqBass + 4.0f * m_eqMid - m_eqTreble;
        const float c = m_eqBass;
        const float inv = 1.0f / std::max(1.0f, static_cast<float>(values.size()) - 1.0f);
        for (size_t i = 0; i < values.size(); ++i) {
            float t = static_cast<float>(i) * inv;
            float gain = a * t * t + b * t + c;
            values[i] = std::clamp(values[i] * std::max(0.0f, gain), 0.0f, 1.0f);
        }
    }

    applyNoiseReduction(values);

    if (m_contrast != 1.0f) {
        for (float& v : values) {
            v = std::pow(v, m_contrast);
        }
    }

    if (m_monstercat) {
        applyMonstercatFilter(values);
    }

    applyTemporalEffects(values);
}

void VisualizerEffects::ensureBuffers(int size) {
    if (static_cast<int>(m_noiseFloor.size()) != size) {
        m_noiseFloor.assign(size, 0.0f);
        m_smoothedBars.assign(size, 0.0f);
        m_peakBars.assign(size, 0.0f);
        m_displayBars.assign(size, 0.0f);
    }
}

void VisualizerEffects::applyNoiseReduction(std::vector<float>& values) {
    float intensity = std::clamp(m_noiseReduction, 0.0f, 1.0f);
    if (intensity <= 0.0f) {
        return;
    }

    const float decay = std::lerp(m_noiseFloorMinDecay, m_noiseFloorMaxDecay, intensity);
    const float rise = std::lerp(m_noiseFloorMaxRise, m_noiseFloorMinRise, intensity);

    for (size_t i = 0; i < values.size(); ++i) {
        float sample = std::clamp(values[i], 0.0f, 1.0f);
        float floor = m_noiseFloor[i];

        floor *= decay;
        if (sample < floor) {
            floor = sample;
        } else {
            floor += (sample - floor) * rise;
        }

        if (floor < m_noiseFloorClamp) {
            floor = 0.0f;
        }

        m_noiseFloor[i] = floor;

        float cleaned = sample - floor * intensity;
        values[i] = std::clamp(cleaned, 0.0f, 1.0f);
    }
}

void VisualizerEffects::applyMonstercatFilter(std::vector<float>& values) {
    if (values.empty()) {
        return;
    }

    static thread_local std::vector<float> scratch;
    scratch.resize(values.size());
    std::copy(values.begin(), values.end(), scratch.begin());

    const float inv = 1.0f / m_monstercatFalloff;
    float carry = 0.0f;
    for (size_t i = 0; i < values.size(); ++i) {
        carry = std::max(scratch[i], carry * inv);
        values[i] = carry;
    }

    carry = 0.0f;
    for (int i = static_cast<int>(values.size()) - 1; i >= 0; --i) {
        carry = std::max(scratch[i], carry * inv);
        values[i] = std::max(values[i], carry);
    }
}

void VisualizerEffects::applyTemporalEffects(std::vector<float>& values) {
    constexpr float kMinValue = 1e-4f;
    constexpr float kDecayScale = 4.0f;

    for (size_t i = 0; i < values.size(); ++i) {
        float current = std::clamp(values[i], 0.0f, 1.0f);

        float smoothed = m_smoothingAlpha * current + (1.0f - m_smoothingAlpha) * m_smoothedBars[i];
        if (smoothed < kMinValue) {
            smoothed = 0.0f;
        }

        float peak = m_peakBars[i];
        if (smoothed >= peak) {
            peak = smoothed;
        } else {
            const float diff = std::clamp(peak - smoothed, 0.0f, 1.0f);
            const float releaseFactor = 1.0f + std::pow(diff, m_gravityPower) * kDecayScale;
            const float decay = std::pow(std::clamp(m_gravityDecay, 0.0f, 0.9999f), releaseFactor);
            peak = smoothed + (peak - smoothed) * decay;
            if (peak < smoothed) {
                peak = smoothed;
            }
        }

        m_smoothedBars[i] = smoothed;
        m_peakBars[i] = peak;

        // Display rises toward peak with inertia, follows peak down immediately
        float display = m_displayBars[i];
        if (peak >= display) {
            display += (peak - display) * (1.0f - m_gravityRise);
        } else {
            display = peak;
        }
        m_displayBars[i] = display;

        values[i] = std::clamp(display, 0.0f, 1.0f);
    }
}

} // namespace balaclava
