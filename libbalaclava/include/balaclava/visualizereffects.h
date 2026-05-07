#pragma once

#include "options.h"
#include <vector>

namespace balaclava {

struct Baclava {
  std::vector<float> bars;
  float beat = 0.0f; // 0.0 = no beat, 1.0 = peak beat intensity
};

class VisualizerEffects {
public:
  explicit VisualizerEffects(const Options &opts);

  void process(std::vector<float> &values);

private:
  void ensureBuffers(int size);
  void applyNoiseReduction(std::vector<float> &values);
  void applyMonstercatFilter(std::vector<float> &values);
  void applyTemporalEffects(std::vector<float> &values);

  std::vector<float> m_noiseFloor;
  std::vector<float> m_smoothedBars;
  std::vector<float> m_peakBars;
  std::vector<float> m_displayBars;

  float m_smoothingAlpha;
  float m_gravityDecay;
  float m_gravityRise;
  float m_gravityPower;
  float m_noiseReduction;
  float m_contrast;
  float m_eqBass;
  float m_eqMid;
  float m_eqTreble;
  bool m_monstercat;
  float m_monstercatFalloff;
  float m_noiseFloorMinDecay;
  float m_noiseFloorMaxDecay;
  float m_noiseFloorMinRise;
  float m_noiseFloorMaxRise;
  float m_noiseFloorClamp;
};

} // namespace balaclava
