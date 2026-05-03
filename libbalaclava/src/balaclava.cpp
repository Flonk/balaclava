#include <balaclava/balaclava.h>

namespace balaclava {

Balaclava::Balaclava(const Options& opts)
    : m_collector(opts)
    , m_beatDetector(opts)
    , m_analyzer(opts)
    , m_effects(opts)
{
    m_collector.setDataCallback([this]() {
        m_dataReady.store(true, std::memory_order_release);
        m_dataReady.notify_one();
    });
}

void Balaclava::start() {
    m_running = true;
    m_collector.start();
}

void Balaclava::setBars(int bars) {
    m_analyzer.setBars(bars);
}

void Balaclava::stop() {
    m_running = false;
    m_dataReady.store(true, std::memory_order_release);
    m_dataReady.notify_one();
    m_collector.stop();
}

bool Balaclava::poll(Baclava& out) {
    float chunk[ac::CHUNK_SIZE];

    for (;;) {
        m_dataReady.wait(false, std::memory_order_acquire);
        m_dataReady.store(false, std::memory_order_relaxed);

        if (!m_running) return false;

        m_collector.readChunk(chunk);
        m_beatDetector.feed(chunk, ac::CHUNK_SIZE);

        if (m_analyzer.consume(chunk, ac::CHUNK_SIZE, out.bars)) {
            break;
        }
    }

    m_effects.process(out.bars);
    out.beat = m_beatDetector.beat();

    if (m_frameCallback) {
        m_frameCallback(out);
    }

    return true;
}

} // namespace balaclava
