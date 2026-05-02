#pragma once

#include <balaclava/options.h>

enum class RenderMode { oneline, fullscreen, ascii };

struct Args {
    balaclava::Options opts;
    RenderMode render_mode = RenderMode::fullscreen;
    int bar_width = 0;     // 0 = auto
    int gap = -1;          // -1 = auto (0 for oneline, 1 for fullscreen)
    float headroom = 1.0f;
};

Args parse_args(int argc, char* argv[]);
