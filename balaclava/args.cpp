#include "args.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void usage() {
    fprintf(stderr,
        "Usage: balaclava [OPTIONS] [TARGET]\n"
        "\n"
        "TARGET is a PipeWire sink/source name (default: @DEFAULT_SINK@)\n"
        "\n"
        "Options:\n"
        "  --source [TARGET]       Capture from a source instead of a sink\n"
        "  --render tiny|big       Render mode (default: tiny)\n"
        "  --bars N                Number of bars (default: 40)\n"
        "  --sample-rate N         Sample rate in Hz (default: 48000)\n"
        "  --frame-size N          FFT frame size (default: 2048)\n"
        "  --hop-size N            FFT hop size (default: 512)\n"
        "  --min-freq N            Min frequency in Hz (default: 20)\n"
        "  --max-freq N            Max frequency in Hz (default: 20000)\n"
        "  --dynamic-falloff N     Dynamic range falloff 0-1 (default: 0.5)\n"
        "  --dynamic-rise N        Dynamic range rise 0-1 (default: 0.0005)\n"
        "  --auto-gain-floor N     Auto gain floor (default: 0.01)\n"
        "  --smoothing N           Smoothing alpha 0-1 (default: 0.7)\n"
        "  --gravity N             Gravity decay 0-1 (default: 0.9)\n"
        "  --noise-reduction N     Noise reduction 0-1 (default: 0.5)\n"
        "  --no-monstercat         Disable monstercat smoothing\n"
        "  -h, --help              Show this help\n"
    );
}

Args parse_args(int argc, char* argv[]) {
    Args args;

    for (int i = 1; i < argc; ++i) {
        auto next = [&]() -> const char* {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for %s\n", argv[i]);
                std::exit(1);
            }
            return argv[++i];
        };

        if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
            usage();
            std::exit(0);
        } else if (std::strcmp(argv[i], "--source") == 0) {
            args.opts.capture_sink = false;
            if (i + 1 < argc && argv[i + 1][0] != '-') args.opts.target = argv[++i];
            else args.opts.target = "@DEFAULT_SOURCE@";
        } else if (std::strcmp(argv[i], "--render") == 0) {
            const char* v = next();
            if (std::strcmp(v, "tiny") == 0) args.render_mode = RenderMode::tiny;
            else if (std::strcmp(v, "big") == 0) args.render_mode = RenderMode::big;
            else { fprintf(stderr, "Unknown render mode: %s\n", v); std::exit(1); }
        } else if (std::strcmp(argv[i], "--bars") == 0) {
            args.opts.bars = std::atoi(next());
        } else if (std::strcmp(argv[i], "--sample-rate") == 0) {
            args.opts.sample_rate = std::atof(next());
        } else if (std::strcmp(argv[i], "--frame-size") == 0) {
            args.opts.frame_size = std::atol(next());
        } else if (std::strcmp(argv[i], "--hop-size") == 0) {
            args.opts.hop_size = std::atol(next());
        } else if (std::strcmp(argv[i], "--min-freq") == 0) {
            args.opts.min_frequency = std::atof(next());
        } else if (std::strcmp(argv[i], "--max-freq") == 0) {
            args.opts.max_frequency = std::atof(next());
        } else if (std::strcmp(argv[i], "--dynamic-falloff") == 0) {
            args.opts.dynamic_falloff = std::atof(next());
        } else if (std::strcmp(argv[i], "--dynamic-rise") == 0) {
            args.opts.dynamic_rise = std::atof(next());
        } else if (std::strcmp(argv[i], "--auto-gain-floor") == 0) {
            args.opts.auto_gain_floor = std::atof(next());
        } else if (std::strcmp(argv[i], "--smoothing") == 0) {
            args.opts.smoothing_alpha = std::atof(next());
        } else if (std::strcmp(argv[i], "--gravity") == 0) {
            args.opts.gravity_decay = std::atof(next());
        } else if (std::strcmp(argv[i], "--noise-reduction") == 0) {
            args.opts.noise_reduction = std::atof(next());
        } else if (std::strcmp(argv[i], "--no-monstercat") == 0) {
            args.opts.monstercat = false;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            std::exit(1);
        } else {
            args.opts.target = argv[i];
        }
    }

    return args;
}
