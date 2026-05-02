#include "render.h"

#include <cstdio>
#include <algorithm>
#include <sys/ioctl.h>
#include <unistd.h>

static const char* blocks[] = {
    " ", "\u2581", "\u2582", "\u2583", "\u2584", "\u2585", "\u2586", "\u2587", "\u2588"
};

Terminal get_terminal_size() {
    struct winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0 && ws.ws_row > 0) {
        return {ws.ws_col, ws.ws_row};
    }
    return {};
}

int bars_for_terminal(int cols, int bar_width, int gap) {
    if (gap == 0) return std::max(1, cols / bar_width);
    return std::max(1, (cols + gap) / (bar_width + gap));
}

int auto_bar_width(int cols, int gap) {
    int w = 2;
    while (bars_for_terminal(cols, w, gap) > 32) {
        ++w;
    }
    return w;
}

void render_fullscreen(const std::vector<float>& values, const Terminal& term,
                       int bar_width, int gap, float headroom, std::string& buf) {
    buf.clear();
    buf.append("\033[H");

    const int bars = static_cast<int>(values.size());
    const int rows = term.rows;
    const int total_units = rows * 8;
    const int used = bars * bar_width + std::max(0, bars - 1) * gap;
    const int margin = (term.cols - used) / 2;

    for (int row = 0; row < rows; ++row) {
        int row_bottom = (rows - 1 - row) * 8;

        if (margin > 0) {
            buf.append(margin, ' ');
        }

        for (int bar = 0; bar < bars; ++bar) {
            float height = values[bar] * headroom * static_cast<float>(total_units);
            int units_in_row = static_cast<int>(height) - row_bottom;

            const char* ch;
            if (units_in_row >= 8) {
                ch = blocks[8];
            } else if (units_in_row > 0) {
                ch = blocks[units_in_row];
            } else {
                ch = blocks[0];
            }

            for (int w = 0; w < bar_width; ++w) {
                buf.append(ch);
            }

            if (bar < bars - 1 && gap > 0) {
                buf.append(gap, ' ');
            }
        }

        int pad = term.cols - margin - used;
        if (pad > 0) {
            buf.append(pad, ' ');
        }

        if (row < rows - 1) {
            buf.push_back('\n');
        }
    }

    fwrite(buf.data(), 1, buf.size(), stdout);
    fflush(stdout);
}

void render_oneline(const std::vector<float>& values, int bar_width, int gap) {
    printf("\r");
    for (int i = 0; i < static_cast<int>(values.size()); ++i) {
        int height = static_cast<int>(values[i] * 8.0f);
        const char* ch = blocks[std::clamp(height, 0, 8)];
        for (int w = 0; w < bar_width; ++w) {
            printf("%s", ch);
        }
        if (i < static_cast<int>(values.size()) - 1 && gap > 0) {
            printf("%*s", gap, "");
        }
    }
    fflush(stdout);
}

void screen_enter() {
    printf("\033[?1049h");
    printf("\033[?25l");
    printf("\033[2J");
    fflush(stdout);
}

void screen_leave() {
    printf("\033[?25h");
    printf("\033[?1049l");
    fflush(stdout);
}
