#pragma once

#include "options.h"
#include "audiocollector.h"
#include "spectrumanalyzer.h"
#include "visualizereffects.h"

#include <atomic>
#include <functional>
#include <vector>

namespace balaclava {

class Balaclava {
public:
    using FrameCallback = std::function<void(const std::vector<float>& bars)>;

    explicit Balaclava(const Options& opts);

    void start();
    void stop();

    void setFrameCallback(FrameCallback cb) { m_frameCallback = std::move(cb); }

    // Blocks until data is available, then processes and returns true.
    // Returns false if stopped.
    bool poll(std::vector<float>& out);

private:
    AudioCollector m_collector;
    SpectrumAnalyzer m_analyzer;
    VisualizerEffects m_effects;
    FrameCallback m_frameCallback;

    std::atomic<bool> m_dataReady{false};
    std::atomic<bool> m_running{false};
};

} // namespace balaclava
