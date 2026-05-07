#pragma once

#include "../mpris.h"
#include "gradient2d.h"

#include <balaclava/visualizereffects.h>
#include <string>
#include <vector>

struct Padding {
  int top, left, bottom, right;
};

struct BalaclavaCliOptions;

// Dynamic render state, resolved from BalaclavaCliOptions + terminal size
struct RenderState {
  int cols = 80;
  int rows = 24;
  int bar_width = 1;
  int gap = 0;
  int bars = 40;

  static RenderState resolve(const BalaclavaCliOptions &opts);
};

int bars_for_terminal(int cols, int bar_width, int gap);
int auto_bar_width(int cols, int gap);

struct BalaclavaCli {
  const BalaclavaCliOptions &opts;
  RenderState rs;
  balaclava::Baclava frame;
  balaclava::Baclava frame2;
  NowPlaying np;
  std::string frame_buf;
};

void render_fullscreen(BalaclavaCli &cli);

void render_oneline(const std::vector<float> &values, int bar_width, int gap);
void render_ascii(const std::vector<float> &values);

void screen_enter();
void screen_leave();

inline const char *blocks[] = {" ",      "\u2581", "\u2582", "\u2583", "\u2584",
                               "\u2585", "\u2586", "\u2587", "\u2588"};
