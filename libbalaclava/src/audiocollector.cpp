#include <balaclava/audiocollector.h>
#include <pipewire/pipewire.h>
#include <spa/param/audio/format-utils.h>
#include <spa/param/props.h>
#include <spa/param/latency-utils.h>
#include <spa/param/buffers.h>
#include <cstring>
#include <cstdio>
#include <vector>

namespace balaclava {

// PipeWire callbacks
static void on_stream_param_changed(void *data, uint32_t id, const struct spa_pod *param);
void on_stream_process(void *data);

static const struct pw_stream_events stream_events = {
    .version = PW_VERSION_STREAM_EVENTS,
    .param_changed = on_stream_param_changed,
    .process = on_stream_process,
};

AudioCollector::AudioCollector(const Options& opts)
    : m_sampleRate(static_cast<uint32_t>(opts.sample_rate))
    , m_target(opts.target), m_captureSink(opts.capture_sink) {
    pw_init(nullptr, nullptr);
}

AudioCollector::~AudioCollector() {
    stop();
    cleanupPipeWire();
    pw_deinit();
}

void AudioCollector::start() {
    if (m_running) return;
    m_running = true;
    initPipeWire();
}

void AudioCollector::stop() {
    if (!m_running) return;
    m_running = false;
    cleanupPipeWire();
}

void AudioCollector::initPipeWire() {
    m_loop = pw_main_loop_new(nullptr);
    if (!m_loop) {
        fprintf(stderr, "AudioCollector: Failed to create PipeWire main loop\n");
        return;
    }

    std::string latency = "128/" + std::to_string(m_sampleRate);
    std::string rate = std::to_string(m_sampleRate) + "/1";

    pw_properties* props = pw_properties_new(
        PW_KEY_MEDIA_TYPE, "Audio",
        PW_KEY_MEDIA_CATEGORY, "Capture",
        PW_KEY_NODE_LATENCY, latency.c_str(),
        PW_KEY_NODE_RATE, rate.c_str(),
        nullptr
    );
    pw_properties_set(props, PW_KEY_TARGET_OBJECT, m_target.c_str());
    if (m_captureSink) {
        pw_properties_set(props, PW_KEY_MEDIA_ROLE, "Music");
        pw_properties_set(props, PW_KEY_STREAM_CAPTURE_SINK, "true");
    } else {
        pw_properties_set(props, PW_KEY_MEDIA_ROLE, "Communication");
    }

    m_stream = pw_stream_new_simple(
        pw_main_loop_get_loop(m_loop),
        "balaclava",
        props,
        &stream_events,
        this);

    if (!m_stream) {
        fprintf(stderr, "AudioCollector: Failed to create PipeWire stream\n");
        return;
    }

    uint8_t paramBuf[1024];
    spa_pod_builder b = SPA_POD_BUILDER_INIT(paramBuf, sizeof(paramBuf));

    spa_audio_info_raw ai{};
    ai.format   = SPA_AUDIO_FORMAT_F32_LE;
    ai.channels = 1;
    ai.rate     = m_sampleRate;

    const uint32_t bytesPerFrame = sizeof(float);
    const uint32_t minFrames     = 64;
    const uint32_t defFrames     = 128;
    const uint32_t maxFrames     = 1024;

    const struct spa_pod* params[3];
    uint32_t n_params = 0;

    params[n_params++] = spa_format_audio_raw_build(&b, SPA_PARAM_EnumFormat, &ai);

    params[n_params++] = static_cast<const struct spa_pod*>(
        spa_pod_builder_add_object(
            &b, SPA_TYPE_OBJECT_ParamBuffers, SPA_PARAM_Buffers,
            SPA_PARAM_BUFFERS_buffers, SPA_POD_CHOICE_RANGE_Int(3, 2, 8),
            SPA_PARAM_BUFFERS_blocks,  SPA_POD_Int(1),
            SPA_PARAM_BUFFERS_size,    SPA_POD_CHOICE_RANGE_Int(defFrames * bytesPerFrame,
                                                                 minFrames * bytesPerFrame,
                                                                 maxFrames * bytesPerFrame),
            SPA_PARAM_BUFFERS_stride,  SPA_POD_Int(bytesPerFrame)
        )
    );

    spa_latency_info lat = SPA_LATENCY_INFO(
        SPA_DIRECTION_INPUT,
        .min_quantum = 2,
        .max_quantum = 4
    );
    params[n_params++] = spa_latency_build(&b, SPA_PARAM_Latency, &lat);

    const pw_stream_flags flags =
        (pw_stream_flags)(
            PW_STREAM_FLAG_AUTOCONNECT |
            PW_STREAM_FLAG_MAP_BUFFERS |
            PW_STREAM_FLAG_RT_PROCESS
        );

    if (pw_stream_connect(m_stream,
                          PW_DIRECTION_INPUT,
                          PW_ID_ANY,
                          flags,
                          params, n_params) < 0) {
        fprintf(stderr, "AudioCollector: Failed to connect PipeWire stream\n");
        return;
    }

    m_thread = std::make_unique<std::thread>([this]() {
        pw_main_loop_run(m_loop);
    });
}

void AudioCollector::cleanupPipeWire() {
    if (m_loop) {
        pw_main_loop_quit(m_loop);
    }

    if (m_thread && m_thread->joinable()) {
        m_thread->join();
        m_thread.reset();
    }

    if (m_stream) {
        pw_stream_destroy(m_stream);
        m_stream = nullptr;
    }

    if (m_loop) {
        pw_main_loop_destroy(m_loop);
        m_loop = nullptr;
    }
}

size_t AudioCollector::readChunk(float* buffer) {
    size_t available = m_ring.available();
    const size_t max_latency_samples = m_sampleRate / 20;

    if (available > max_latency_samples) {
        size_t skip_amount = available - (m_sampleRate / 40);
        size_t skipped = m_ring.skip(skip_amount);
        m_data_counter.fetch_sub(skipped, std::memory_order_relaxed);
    }

    size_t got = m_ring.read(buffer, ac::CHUNK_SIZE);

    if (got > 0) {
        m_data_counter.fetch_sub(got, std::memory_order_relaxed);
    }

    if (got < ac::CHUNK_SIZE) {
        std::memset(buffer + got, 0, (ac::CHUNK_SIZE - got) * sizeof(float));
    }

    return ac::CHUNK_SIZE;
}

void AudioCollector::writeAudioData(const float* data, size_t frames) {
    size_t written = m_ring.write(data, frames);

    size_t new_count = m_data_counter.fetch_add(written, std::memory_order_relaxed) + written;

    if ((new_count / ac::CHUNK_SIZE) > ((new_count - written) / ac::CHUNK_SIZE)) {
        if (m_dataCallback) {
            m_dataCallback();
        }
    }
}

// PipeWire callback implementations
static void on_stream_param_changed(void *data, uint32_t id, const struct spa_pod *param) {
    (void)data;
    (void)id;
    if (!param) return;
}

void on_stream_process(void *data) {
    auto* collector = static_cast<AudioCollector*>(data);

    pw_buffer* pwb = pw_stream_dequeue_buffer(collector->getStream());
    if (!pwb) return;

    spa_buffer* b = pwb->buffer;
    if (!b || b->n_datas == 0) {
        pw_stream_queue_buffer(collector->getStream(), pwb);
        return;
    }

    spa_data& d = b->datas[0];
    if (!d.data || !d.chunk) {
        pw_stream_queue_buffer(collector->getStream(), pwb);
        return;
    }

    const uint32_t offs   = d.chunk->offset;
    const uint32_t nbytes = d.chunk->size;
    const uint32_t stride = d.chunk->stride ? d.chunk->stride : static_cast<uint32_t>(sizeof(float));

    if (nbytes >= stride) {
        const uint8_t* base = static_cast<const uint8_t*>(d.data) + offs;
        const size_t frames = nbytes / stride;

        if (stride == sizeof(float)) {
            const float* samples = reinterpret_cast<const float*>(base);
            collector->writeAudioData(samples, frames);
        } else {
            static thread_local std::vector<float> tmp;
            tmp.resize(frames);
            for (size_t i = 0; i < frames; ++i) {
                tmp[i] = *reinterpret_cast<const float*>(base + i * stride);
            }
            collector->writeAudioData(tmp.data(), frames);
        }
    }

    pw_stream_queue_buffer(collector->getStream(), pwb);
}

} // namespace balaclava
