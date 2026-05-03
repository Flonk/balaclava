#pragma once

#include "options.h"
#include <array>
#include <cstddef>

namespace balaclava {

// BlackLava beat detection system
class BlackLava {
public:
    explicit BlackLava(const Options& opts);

    // Feed raw audio samples, returns current beat intensity (0-1)
    float feed(const float* samples, std::size_t count);
    float beat() const { return m_beat; }

private:
    static constexpr int NUM_BANDS = 2;
    static constexpr float BAND_WEIGHTS[NUM_BANDS] = {2.5f, 1.0f};

    struct Band {
        // For sub-bass: single LP at cutoff
        // For mid-bass: bandpass via two LPs (hi - lo)
        float lpCoeffLo = 0.0f;  // high-pass edge (0 for low-pass only)
        float lpCoeffHi = 0.0f;  // low-pass edge
        float lpStateLo = 0.0f;
        float lpStateHi = 0.0f;
        float prevEnergy = 0.0f;
        float avgFlux = 0.0f;
        bool bandpass = false;
    };

    std::array<Band, NUM_BANDS> m_bands;

    float m_beat = 0.0f;

    // Parameters
    float m_baseThreshold;
    float m_effectiveThreshold;
    float m_decay;
    float m_gamma;
    float m_avgAlpha;

    // Adaptive threshold
    float m_avgDeviation = 0.0f;
    float m_thresholdAdapt;

    float m_maxBeatFloor;
    float m_avgRms = 0.0f;
    float m_peakRms = 0.0f;
};

} // namespace balaclava
