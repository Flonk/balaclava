#include <balaclava/spectrumanalyzer.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <numbers>
#include <stdexcept>

namespace balaclava {
namespace {
constexpr float kEpsilon = 1e-9f;
constexpr float kEaseTransitionStart = 0.85f;
constexpr float kEaseMinRange = 1e-6f;
constexpr float kBaseEasePower = 3.0f;
constexpr float kMinEasePower = 2.2f;
constexpr float kMaxEasePower = 8.0f;
constexpr float kDynamicRangeTarget = 0.35f;
constexpr float kDynamicRangeScale = 8.0f;

float applyInOutEase(float normalized, float power) {
  const float x = std::clamp(normalized, 0.0f, 1.0f);
  if (x <= kEaseTransitionStart) {
    return std::pow(x, power);
  }

  const float transitionRange =
      std::max(1.0f - kEaseTransitionStart, kEaseMinRange);
  const float t =
      std::clamp((x - kEaseTransitionStart) / transitionRange, 0.0f, 1.0f);
  const float baseAtTransition = std::pow(kEaseTransitionStart, power);
  const float easeOut = 1.0f - std::pow(1.0f - t, power);
  return baseAtTransition + (1.0f - baseAtTransition) * easeOut;
}
} // namespace

static float halflifeToFactor(double halflife_ms, double sample_rate,
                              std::size_t hop_size) {
  const double fps = sample_rate / static_cast<double>(hop_size);
  const double frames = halflife_ms / 1000.0 * fps;
  if (frames <= 0.0)
    return 0.0f;
  return static_cast<float>(std::pow(0.5, 1.0 / frames));
}

SpectrumAnalyzer::SpectrumAnalyzer(const Options &opts)
    : m_bars(std::max(1, opts.bars)),
      m_sampleRate(static_cast<float>(opts.sample_rate)),
      m_minFreq(static_cast<float>(opts.min_frequency)),
      m_maxFreq(static_cast<float>(opts.max_frequency)),
      m_dynamicFalloff(halflifeToFactor(opts.dynamic_falloff_ms,
                                        opts.sample_rate, opts.hop_size)),
      m_dynamicRise(1.0f - halflifeToFactor(opts.dynamic_rise_ms,
                                            opts.sample_rate, opts.hop_size)),
      m_autoGainFloor(static_cast<float>(opts.auto_gain_floor)),
      m_frameSize(opts.frame_size), m_hopSize(opts.hop_size), m_fifo(),
      m_window(opts.frame_size), m_magnitudes(opts.frame_size / 2 + 1),
      m_binWeights(opts.frame_size / 2 + 1, 1.0f) {
  m_fftInput = static_cast<float *>(fftwf_malloc(sizeof(float) * m_frameSize));
  m_fftOutput = static_cast<fftwf_complex *>(
      fftwf_malloc(sizeof(fftwf_complex) * (m_frameSize / 2 + 1)));

  if (!m_fftInput || !m_fftOutput) {
    throw std::bad_alloc();
  }

  m_plan = fftwf_plan_dft_r2c_1d(static_cast<int>(m_frameSize), m_fftInput,
                                 m_fftOutput, FFTW_MEASURE);
  if (!m_plan) {
    throw std::runtime_error("Failed to create FFTW plan");
  }

  rebuildWindow();
  rebuildBinMapping(m_sampleRate, m_minFreq, m_maxFreq);
}

void SpectrumAnalyzer::setBars(int bars) {
  bars = std::max(1, bars);
  if (bars == m_bars)
    return;
  m_bars = bars;
  rebuildBinMapping(m_sampleRate, m_minFreq, m_maxFreq);
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
  if (m_plan) {
    fftwf_destroy_plan(m_plan);
  }
  if (m_fftInput) {
    fftwf_free(m_fftInput);
  }
  if (m_fftOutput) {
    fftwf_free(m_fftOutput);
  }
}

bool SpectrumAnalyzer::consume(const float *samples, std::size_t count,
                               std::vector<float> &outBars) {
  if (!samples || count == 0) {
    return false;
  }

  m_fifo.insert(m_fifo.end(), samples, samples + count);

  bool updated = false;
  while (m_fifo.size() - m_fifoOffset >= m_frameSize) {
    if (processFrame(outBars)) {
      updated = true;
    }
    m_fifoOffset += m_hopSize;
  }

  // Compact: shift remaining data to front
  if (m_fifoOffset > 0) {
    std::size_t remaining = m_fifo.size() - m_fifoOffset;
    if (remaining > 0) {
      std::memmove(m_fifo.data(), m_fifo.data() + m_fifoOffset,
                   remaining * sizeof(float));
    }
    m_fifo.resize(remaining);
    m_fifoOffset = 0;
  }

  return updated;
}

void SpectrumAnalyzer::rebuildWindow() {
  constexpr float pi = std::numbers::pi_v<float>;
  for (std::size_t i = 0; i < m_frameSize; ++i) {
    float x = static_cast<float>(i) / static_cast<float>(m_frameSize - 1);
    m_window[i] = 0.5f - 0.5f * std::cos(2.0f * pi * x);
  }
}

void SpectrumAnalyzer::rebuildBinMapping(float sampleRate, float minFreq,
                                         float maxFreq) {
  const std::size_t binCount = m_frameSize / 2 + 1;
  m_binStart.resize(m_bars);
  m_binEnd.resize(m_bars);
  m_binWeights.resize(binCount);

  const float nyquist = sampleRate / 2.0f;
  const float lower = std::clamp(minFreq, 1.0f, nyquist);
  const float upper = std::clamp(maxFreq, lower, nyquist);
  const float ratio = std::max(1.0f, upper / lower);

  const float logRange = (ratio > 1.0f) ? std::log(ratio) : 1.0f;

  for (std::size_t bin = 0; bin < binCount; ++bin) {
    if (bin == 0) {
      m_binWeights[bin] = 0.0f;
      continue;
    }

    const float frequency = (static_cast<float>(bin) * sampleRate) /
                            static_cast<float>(m_frameSize);
    const float clamped = std::clamp(frequency, lower, upper);

    float normalized = 0.0f;
    if (ratio > 1.0f) {
      normalized = std::log(clamped / lower) / logRange;
    }
    normalized = std::clamp(normalized, 0.0f, 1.0f);

    const float freqRatio = std::max(clamped / lower, 1.0f);
    const float boosted = std::pow(freqRatio, 0.40f);
    const float tilt = 0.1f + 0.9f * normalized;
    m_binWeights[bin] = std::clamp(boosted * tilt, 0.3f, 12.0f);
  }

  for (int i = 0; i < m_bars; ++i) {
    const float startFrac = static_cast<float>(i) / m_bars;
    const float endFrac = static_cast<float>(i + 1) / m_bars;

    const float fStart = lower * std::pow(ratio, startFrac);
    const float fEnd = lower * std::pow(ratio, endFrac);

    std::size_t binStart =
        static_cast<std::size_t>(std::floor(fStart * m_frameSize / sampleRate));
    std::size_t binEnd =
        static_cast<std::size_t>(std::ceil(fEnd * m_frameSize / sampleRate));

    binStart = std::clamp<std::size_t>(binStart, 0, binCount - 1);
    binEnd = std::clamp<std::size_t>(binEnd, binStart + 1, binCount);

    m_binStart[i] = binStart;
    m_binEnd[i] = binEnd;
  }
}

bool SpectrumAnalyzer::processFrame(std::vector<float> &outBars) {
  if (!m_plan || m_fifo.size() - m_fifoOffset < m_frameSize) {
    return false;
  }

  const float *src = m_fifo.data() + m_fifoOffset;
  for (std::size_t i = 0; i < m_frameSize; ++i) {
    m_fftInput[i] = src[i] * m_window[i];
  }

  fftwf_execute(m_plan);

  const std::size_t binCount = m_frameSize / 2 + 1;
  for (std::size_t i = 0; i < binCount; ++i) {
    const float real = m_fftOutput[i][0];
    const float imag = m_fftOutput[i][1];
    const float magnitude = std::sqrt(real * real + imag * imag) + kEpsilon;
    m_magnitudes[i] = magnitude * m_binWeights[i];
  }

  if (static_cast<int>(outBars.size()) != m_bars) {
    outBars.resize(m_bars);
  }

  float peak = kEpsilon;
  for (int bar = 0; bar < m_bars; ++bar) {
    const std::size_t start = m_binStart[bar];
    const std::size_t end = m_binEnd[bar];

    float value = 0.0f;
    for (std::size_t bin = start; bin < end; ++bin) {
      value = std::max(value, m_magnitudes[bin]);
    }

    outBars[bar] = value;
    peak = std::max(peak, value);
  }

  if (peak > kEpsilon) {
    const float invPeak = 1.0f / peak;
    m_normalized.resize(outBars.size());
    float sumNormalized = 0.0f;
    int activeCount = 0;

    for (size_t i = 0; i < outBars.size(); ++i) {
      const float n = std::clamp(outBars[i] * invPeak, 0.0f, 1.0f);
      m_normalized[i] = n;
      if (n > 0.0f) {
        sumNormalized += n;
        ++activeCount;
      }
    }

    const float meanNormalized =
        activeCount > 0 ? (sumNormalized / static_cast<float>(activeCount))
                        : 0.0f;
    const float dynamicRange = std::clamp(1.0f - meanNormalized, 0.0f, 1.0f);
    const float powerAdjustment =
        (kDynamicRangeTarget - dynamicRange) * kDynamicRangeScale;
    const float easePower = std::clamp(kBaseEasePower + powerAdjustment,
                                       kMinEasePower, kMaxEasePower);

    for (size_t i = 0; i < outBars.size(); ++i) {
      outBars[i] = applyInOutEase(m_normalized[i], easePower) * peak;
    }
  }

  peak = std::max(peak, m_autoGainFloor);

  if (peak > m_dynamicMax) {
    m_dynamicMax += (peak - m_dynamicMax) * m_dynamicRise;
  } else {
    m_dynamicMax =
        m_dynamicMax * m_dynamicFalloff + peak * (1.0f - m_dynamicFalloff);
  }
  m_dynamicMax = std::max(std::max(m_dynamicMax, kEpsilon), m_autoGainFloor);

  const float inv = 1.0f / m_dynamicMax;
  for (float &v : outBars) {
    v = std::clamp(v * inv, 0.0f, 1.0f);
  }

  return true;
}

} // namespace balaclava
