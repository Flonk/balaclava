#pragma once

#include "options.h"
#include <cstddef>
#include <deque>
#include <fftw3.h>
#include <vector>

namespace balaclava {

class SpectrumAnalyzer {
public:
  explicit SpectrumAnalyzer(const Options &opts);
  ~SpectrumAnalyzer();

  SpectrumAnalyzer(const SpectrumAnalyzer &) = delete;
  SpectrumAnalyzer &operator=(const SpectrumAnalyzer &) = delete;

  bool consume(const float *samples, std::size_t count,
               std::vector<float> &outBars);
  void setBars(int bars);

private:
  void rebuildWindow();
  void rebuildBinMapping(float sampleRate, float minFreq, float maxFreq);
  bool processFrame(std::vector<float> &outBars);

  int m_bars;
  float m_sampleRate;
  float m_minFreq;
  float m_maxFreq;
  float m_dynamicFalloff;
  float m_dynamicRise;
  float m_autoGainFloor;
  const std::size_t m_frameSize;
  const std::size_t m_hopSize;

  std::deque<float> m_fifo;

  std::vector<float> m_window;
  std::vector<float> m_magnitudes;
  std::vector<float> m_binWeights;
  std::vector<float> m_normalized;

  std::vector<std::size_t> m_binStart;
  std::vector<std::size_t> m_binEnd;

  fftwf_plan m_plan = nullptr;
  float *m_fftInput = nullptr;
  fftwf_complex *m_fftOutput = nullptr;

  float m_dynamicMax = 0;
};

} // namespace balaclava
