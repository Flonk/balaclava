#pragma once

#include "options.h"
#include "spsc_ring.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

// Forward declarations
struct pw_main_loop;
struct pw_stream;

namespace balaclava {

namespace ac {
constexpr uint32_t CHUNK_SIZE = 512;
}

class AudioCollector {
public:
  using DataCallback = std::function<void()>;

  explicit AudioCollector(const Options &opts);
  ~AudioCollector();

  AudioCollector(const AudioCollector &) = delete;
  AudioCollector &operator=(const AudioCollector &) = delete;

  void start();
  void stop();

  void setDataCallback(DataCallback cb) { m_dataCallback = std::move(cb); }

  size_t readChunk(float *buffer);

private:
  friend void on_stream_process(void *data);

  void writeAudioData(const float *data, size_t frames);
  ::pw_stream *getStream() { return m_stream; }
  void initPipeWire();
  void cleanupPipeWire();

  uint32_t m_sampleRate;
  SpscRing m_ring{1 << 12};
  std::atomic<size_t> m_data_counter{0};
  std::atomic<bool> m_running{false};
  std::string m_target;
  bool m_captureSink;
  DataCallback m_dataCallback;

  ::pw_main_loop *m_loop = nullptr;
  ::pw_stream *m_stream = nullptr;
  std::unique_ptr<std::thread> m_thread;
};

} // namespace balaclava
