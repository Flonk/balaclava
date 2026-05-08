#pragma once

#include "mpris/mpris.h"

#include <balaclava/visualizereffects.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

struct Padding {
  int top, left, bottom, right;
};

struct BalaclavaCliOptions;
enum class RenderMode : int;

struct MprisPlayback {
  int64_t position_us = -1;
  int64_t length_us = -1;
  bool playing = false;
  bool paused = false;
  int64_t poll_position_us = -1;
  std::chrono::steady_clock::time_point poll_time;
};

struct BalaclavaCli {
  const BalaclavaCliOptions &opts;
  balaclava::Baclava frame;
  balaclava::Baclava frame2;
  NowPlaying np;
  MprisPlayback mpris_pb;
  std::string frame_buf;
};

class Renderer {
public:
  virtual ~Renderer() = default;
  virtual void enter(const BalaclavaCliOptions &opts, int cols, int rows) = 0;
  virtual void leave() {}
  virtual void resize(int cols, int rows) {}
  virtual void render(BalaclavaCli &cli) = 0;
  // Process raw stdin bytes. Returns number of unconsumed bytes remaining
  // (moved to start of buf). Default: discard all.
  virtual int process_stdin(char *, int) { return 0; }
  int bars() const { return bars_; }

protected:
  int cols_ = 80;
  int rows_ = 24;
  int bar_width_ = 1;
  int gap_ = 0;
  int bars_ = 40;
};

void get_terminal_size(int &cols, int &rows);

std::unique_ptr<Renderer> make_fullscreen_renderer();
std::unique_ptr<Renderer> make_oneline_renderer();
std::unique_ptr<Renderer> make_ascii_renderer();
std::unique_ptr<Renderer> make_renderer(RenderMode mode);

// Lower block elements: ▁▂▃▄▅▆▇█ (fill from bottom)
inline const char *vblocks[] = {" ",      "\u2581", "\u2582",
                                "\u2583", "\u2584", "\u2585",
                                "\u2586", "\u2587", "\u2588"};

// Left block elements: ▏▎▍▌▋▊▉█ (fill from left)
inline const char *hblocks[] = {" ",      "\u258F", "\u258E",
                                "\u258D", "\u258C", "\u258B",
                                "\u258A", "\u2589", "\u2588"};
