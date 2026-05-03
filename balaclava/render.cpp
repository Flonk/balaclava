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

static uint8_t mix(uint8_t a, uint8_t b, float t) {
    return static_cast<uint8_t>(a + (b - a) * t);
}

static void append_color(std::string& buf, float t, float beat, const Gradient& g) {
    // Bilinear: interpolate bottom→top, then blend toward beat colors
    uint8_t r0 = mix(g.lo.r, g.hi.r, t);
    uint8_t g0 = mix(g.lo.g, g.hi.g, t);
    uint8_t b0 = mix(g.lo.b, g.hi.b, t);

    if (beat > 0.0f) {
        uint8_t r1 = mix(g.beat_lo.r, g.beat_hi.r, t);
        uint8_t g1 = mix(g.beat_lo.g, g.beat_hi.g, t);
        uint8_t b1 = mix(g.beat_lo.b, g.beat_hi.b, t);
        r0 = mix(r0, r1, beat);
        g0 = mix(g0, g1, beat);
        b0 = mix(b0, b1, beat);
    }

    char color[24];
    int n = snprintf(color, sizeof(color), "\033[38;2;%d;%d;%dm", r0, g0, b0);
    buf.append(color, n);
}

void render_fullscreen(const std::vector<float>& values, const Terminal& term,
                       int bar_width, int gap, float headroom, float beat,
                       const Gradient& grad, const NowPlaying& np,
                       const Padding& pad, std::string& buf) {
    buf.clear();
    buf.append("\033[H");

    const int bars = static_cast<int>(values.size());
    const int content_rows = term.rows - pad.top - pad.bottom;
    const int content_cols = term.cols - pad.left - pad.right;
    const int total_units = content_rows * 8;
    const int used = bars * bar_width + std::max(0, bars - 1) * gap;
    const int inner_margin = std::max(0, (content_cols - used) / 2);

    for (int row = 0; row < term.rows; ++row) {
        if (row < pad.top || row >= term.rows - pad.bottom) {
            buf.append(term.cols, ' ');
        } else {
            int content_row = row - pad.top;
            int row_bottom = (content_rows - 1 - content_row) * 8;
            float row_t = static_cast<float>(content_row) / std::max(1.0f, static_cast<float>(content_rows - 1));

            int left = pad.left + inner_margin;
            if (left > 0) {
                buf.append(left, ' ');
            }

            append_color(buf, row_t, beat, grad);

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

            int remaining = term.cols - left - used;
            if (remaining > 0) {
                buf.append("\033[0m");
                buf.append(remaining, ' ');
            }
        }

        if (row < term.rows - 1) {
            buf.push_back('\n');
        }
    }

    // MPRIS overlay top-right, respecting padding
    if (!np.title.empty() && pad.top > 0) {
        std::string label = np.artist.empty() ? np.title : np.artist + " \xe2\x80\x94 " + np.title;
        int len = static_cast<int>(label.size());
        int max_len = term.cols - pad.left - pad.right;
        if (len > max_len) {
            label = label.substr(0, max_len);
            len = max_len;
        }
        int col = term.cols - pad.right - len + 1;
        char pos[16];
        int pn = snprintf(pos, sizeof(pos), "\033[%d;%dH", pad.top + 1, col);
        buf.append(pos, pn);
        buf.append("\033[38;2;80;80;80m");
        buf.append(label);
    }

    buf.append("\033[0m");
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

void render_ascii(const std::vector<float>& values) {
    // 32 levels: 0-9 a-v
    static const char levels[] = "0123456789abcdefghijklmnopqrstuv";
    for (const float& v : values) {
        int idx = static_cast<int>(v * 31.0f);
        putchar(levels[std::clamp(idx, 0, 31)]);
    }
    putchar('\n');
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
