#pragma once

#include <string>
#include <vector>

struct Terminal {
    int cols = 80;
    int rows = 24;
};

Terminal get_terminal_size();

// Returns bar count that fits terminal width for given bar_width and gap
int bars_for_terminal(int cols, int bar_width, int gap);

// Pick smallest bar_width (starting at 2) that keeps bar count <= 32
int auto_bar_width(int cols, int gap);

void render_fullscreen(const std::vector<float>& values, const Terminal& term,
                       int bar_width, int gap, float headroom, float beat, std::string& buf);

void render_oneline(const std::vector<float>& values, int bar_width, int gap);
void render_ascii(const std::vector<float>& values);

void screen_enter();
void screen_leave();
