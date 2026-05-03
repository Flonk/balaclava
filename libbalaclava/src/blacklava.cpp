#include <balaclava/blacklava.h>

#include <algorithm>
#include <cmath>

namespace balaclava {

static float lpCoeff(float freq, float sampleRate) {
  return std::exp(-2.0f * 3.14159265f * freq / sampleRate);
}

// BlackLava beat detection system
BlackLava::BlackLava(const Options &opts) {
  m_baseThreshold = 2.0f;
  m_effectiveThreshold = m_baseThreshold;
  m_decay = static_cast<float>(opts.beat_decay);
  m_gamma = static_cast<float>(opts.beat_gamma);
  m_maxBeatFloor = static_cast<float>(opts.beat_floor);

  const double fps = opts.sample_rate / static_cast<double>(opts.hop_size);
  m_avgAlpha = static_cast<float>(2.0 / (0.3 * fps + 1.0));

  // Threshold adapts over ~1s
  m_thresholdAdapt = static_cast<float>(2.0 / (1.0 * fps + 1.0));

  float sr = static_cast<float>(opts.sample_rate);

  // Band 0: sub-bass, < 50 Hz
  m_bands[0].lpCoeffHi = lpCoeff(50.0f, sr);
  m_bands[0].bandpass = false;

  // Band 1: mid-bass, 120-140 Hz (bandpass)
  m_bands[1].lpCoeffLo = lpCoeff(120.0f, sr);
  m_bands[1].lpCoeffHi = lpCoeff(140.0f, sr);
  m_bands[1].bandpass = true;
}

float BlackLava::feed(const float *samples, std::size_t count) {
  float bestDeviation = 0.0f;
  float maxAvgFlux = 0.0f;

  // First pass: compute flux for all bands
  float fluxes[NUM_BANDS];

  float crests[NUM_BANDS];

  for (int b = 0; b < NUM_BANDS; ++b) {
    auto &band = m_bands[b];
    float energy = 0.0f;
    float peak = 0.0f;

    float lpHi = band.lpStateHi;
    float lpLo = band.lpStateLo;

    if (band.bandpass) {
      for (std::size_t i = 0; i < count; ++i) {
        float s = samples[i];
        lpHi = lpHi * band.lpCoeffHi + s * (1.0f - band.lpCoeffHi);
        lpLo = lpLo * band.lpCoeffLo + s * (1.0f - band.lpCoeffLo);
        float bp = lpHi - lpLo;
        energy += bp * bp;
        float absBp = std::abs(bp);
        peak = std::max(peak, absBp);
      }
    } else {
      for (std::size_t i = 0; i < count; ++i) {
        lpHi = lpHi * band.lpCoeffHi + samples[i] * (1.0f - band.lpCoeffHi);
        energy += lpHi * lpHi;
        float absLp = std::abs(lpHi);
        peak = std::max(peak, absLp);
      }
    }

    band.lpStateHi = lpHi;
    band.lpStateLo = lpLo;
    energy /= static_cast<float>(count);

    // Crest factor: peak / RMS. Kicks ~3-5, sustained bass ~1
    float rms = std::sqrt(energy);
    crests[b] = (rms > 1e-10f) ? (peak / rms) : 1.0f;

    fluxes[b] = std::max(0.0f, energy - band.prevEnergy);
    band.prevEnergy = energy;

    band.avgFlux += (fluxes[b] - band.avgFlux) * m_avgAlpha;
    maxAvgFlux = std::max(maxAvgFlux, band.avgFlux);
  }

  // Second pass: compute ratios with loudness scaling and crest factor
  for (int b = 0; b < NUM_BANDS; ++b) {
    auto &band = m_bands[b];
    if (band.avgFlux > 1e-12f && maxAvgFlux > 1e-12f) {
      float ratio = fluxes[b] / band.avgFlux;
      float loudness = std::sqrt(band.avgFlux / maxAvgFlux);
      // Crest factor boost: normalize so ~1 for sustained, >1 for transients
      float crestBoost = std::max(1.0f, crests[b] / 1.4f);
      ratio *= BAND_WEIGHTS[b] * loudness * crestBoost;
      bestDeviation = std::max(bestDeviation, ratio);
    }
  }

  // Decay existing beat
  m_beat *= m_decay;

  // Adaptive threshold
  m_avgDeviation += (bestDeviation - m_avgDeviation) * m_thresholdAdapt;
  m_effectiveThreshold =
      std::max(m_baseThreshold, m_avgDeviation * m_baseThreshold);

  // Track RMS: fast peak, slow average
  float sumSq = 0.0f;
  for (std::size_t i = 0; i < count; ++i) {
    sumSq += samples[i] * samples[i];
  }
  float rms = std::sqrt(sumSq / static_cast<float>(count));
  m_avgRms += (rms - m_avgRms) * m_thresholdAdapt * 0.5f;
  // Peak RMS: fast rise, slow decay
  if (rms > m_peakRms) {
    m_peakRms += (rms - m_peakRms) * 0.3f;
  } else {
    m_peakRms += (rms - m_peakRms) * m_thresholdAdapt * 0.2f;
  }

  // Dynamic range: peak/avg. High = dynamic (jazz ~4+), low = compressed (DnB
  // ~1.5)
  float dynRange = (m_avgRms > 1e-8f) ? (m_peakRms / m_avgRms) : 1.0f;
  // Map: 1.0 (fully compressed) → 0, 4.0+ (very dynamic) → 1
  float clarity = std::clamp((dynRange - 1.0f) / 3.0f, 0.0f, 1.0f);
  // clarity: 0 = compressed/noisy, 1 = dynamic/clean
  float beatFloor = 0.3f + clarity * (m_maxBeatFloor - 0.3f);
  float effectiveGamma = m_gamma * (0.8f + 0.2f * clarity);

  // Don't detect beats in near-silence
  if (m_avgRms < 0.005f)
    return m_beat;

  // Trigger
  if (bestDeviation > m_effectiveThreshold) {
    float intensity = std::clamp((bestDeviation - m_effectiveThreshold) /
                                     (m_effectiveThreshold * 2.0f),
                                 0.0f, 1.0f);
    if (intensity >= beatFloor) {
      // Remap survivors: floor→0, 1→1, then gamma
      intensity = (intensity - beatFloor) / (1.0f - beatFloor);
      intensity = std::pow(intensity, effectiveGamma);
      m_beat = std::max(m_beat, intensity);
    }
  }

  return m_beat;
}

} // namespace balaclava
