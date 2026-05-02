#pragma once

#include <balaclava/options.h>

enum class RenderMode { tiny, big };

struct Args {
    balaclava::Options opts;
    RenderMode render_mode = RenderMode::tiny;
};

Args parse_args(int argc, char* argv[]);
