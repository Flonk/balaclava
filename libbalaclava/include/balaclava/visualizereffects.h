#pragma once

#include <vector>
#include "options.h"

namespace balaclava {

class VisualizerEffects {
public:
    explicit VisualizerEffects(const Options& opts);

    void process(std::vector<float>& values);

private:
    void ensureBuffers(int size);
    void applyNoiseReduction(std::vector<float>& values);
    void applyMonstercatFilter(std::vector<float>& values);
    void applyTemporalEffects(std::vector<float>& values);

    std::vector<float> m_noiseFloor;
    std::vector<float> m_smoothedBars;
    std::vector<float> m_peakBars;

    float m_smoothingAlpha;
    float m_gravityDecay;
    float m_noiseReduction;
    bool m_monstercat;
    float m_noiseFloorMinDecay;
    float m_noiseFloorMaxDecay;
    float m_noiseFloorMinRise;
    float m_noiseFloorMaxRise;
    float m_noiseFloorClamp;
};

} // namespace balaclava
