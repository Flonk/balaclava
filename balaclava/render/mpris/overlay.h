#pragma once

#include "../../args/args.h"
#include "mpris.h"
#include "../common.h"
#include "../utils/color/ansi.h"
#include "../utils/color/color.h"
#include "../fullscreen/framebuffer.h"

#include "../utils/string.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>

enum class MprisHit { none, play_pause, prev, next };

class MprisOverlay : public Renderer {
public:
  void enter(const BalaclavaCliOptions &opts, int cols, int rows) override {
    opts_ = &opts;
    cols_ = cols;
    rows_ = rows;
  }

  void resize(int cols, int rows) override {
    cols_ = cols;
    rows_ = rows;
    invalidate();
  }

  // Called by parent renderer after its own layout resolve.
  void resolve(bool horizontal, BarAlignment alignment,
               BarAlignment alignment2) {
    position_ = opts_->mpris_position;
    if (position_ == MprisPosition::auto_) {
      if (horizontal) {
        position_ = MprisPosition::bottom;
      } else if ((alignment == BarAlignment::start &&
                  alignment2 == BarAlignment::end) ||
                 (alignment == BarAlignment::end &&
                  alignment2 == BarAlignment::start)) {
        position_ = MprisPosition::center;
      } else if (alignment == BarAlignment::start ||
                 alignment2 == BarAlignment::start) {
        position_ = MprisPosition::bottomright;
      } else {
        position_ = MprisPosition::topright;
      }
    }
  }

  void attach(FrameBuffer &fb) { fb_ = &fb; }

  void invalidate() {
    box_drawn_ = false;
    prev_artist_.clear();
    prev_title_.clear();
    prev_status_.clear();
    cached_label_.clear();
    cached_label_visible_ = 0;
  }

  MprisHit hit_test(int term_row, int term_col) const {
    auto check = [&](const HitRegion &r) {
      return r.row > 0 && term_row == r.row && term_col >= r.col_start &&
             term_col <= r.col_end;
    };
    if (check(hit_play_pause_)) return MprisHit::play_pause;
    if (check(hit_prev_)) return MprisHit::prev;
    if (check(hit_next_)) return MprisHit::next;
    return MprisHit::none;
  }

  // Exclusion rect for framebuffer
  int prev_width_ = 0, prev_height_ = 0;
  int prev_row_ = 0, prev_col_ = 0;

protected:
  const BalaclavaCliOptions *opts_ = nullptr;
  FrameBuffer *fb_ = nullptr;
  MprisPosition position_ = MprisPosition::auto_;

  // Dirty tracking
  std::string prev_artist_;
  std::string prev_title_;
  std::string prev_status_;
  bool prev_playing_ = false;
  int prev_ww_ = 0;
  MprisPosition prev_position_ = MprisPosition::auto_;
  bool box_drawn_ = false;

  // Cached label
  std::string cached_label_;
  int cached_label_visible_ = 0;

  // Hit regions (1-based terminal coordinates)
  struct HitRegion {
    int row = 0, col_start = 0, col_end = 0;
  };
  HitRegion hit_play_pause_{}, hit_prev_{}, hit_next_{};

  // ── helpers ──

  static Color resolve_bg(const BalaclavaCliOptions &opts) {
    return opts.mpris_bg_auto ? opts.bg_color : opts.mpris_bg_color;
  }

  static Color resolve_ui(const BalaclavaCliOptions &opts) {
    return opts.mpris_ui_color_auto ? opts.mpris_bar_color : opts.mpris_ui_color;
  }

  static Color beat_pulse(Color base, Color target, float beat) {
    if (beat > 0.f) {
      base.r = mix(base.r, target.r, beat);
      base.g = mix(base.g, target.g, beat);
      base.b = mix(base.b, target.b, beat);
    }
    return base;
  }

  bool track_changed(const NowPlaying &np) const {
    return np.artist != prev_artist_ || np.title != prev_title_;
  }

  bool geometry_changed(int ww, MprisPosition pos) const {
    return ww != prev_ww_ || pos != prev_position_;
  }

  void update_track_cache(const NowPlaying &np, int max_cols) {
    prev_artist_ = np.artist;
    prev_title_ = np.title;

    cached_label_.clear();
    if (!np.title.empty()) {
      if (np.artist.empty()) {
        cached_label_ = np.title;
      } else {
        cached_label_ =
            "\033[1m" + np.artist + "\033[22m \xe2\x80\x94 " + np.title;
      }
    }
    int visible = str_width(cached_label_);
    if (visible > max_cols && max_cols >= 2) {
      int cut = str_truncate(cached_label_.data(),
                             static_cast<int>(cached_label_.size()),
                             max_cols - 1);
      cached_label_.resize(cut);
      cached_label_.append("\xe2\x80\xa6"); // …
      visible = str_width(cached_label_);
    } else if (visible > max_cols) {
      int cut = str_truncate(cached_label_.data(),
                             static_cast<int>(cached_label_.size()),
                             max_cols);
      cached_label_.resize(cut);
      visible = str_width(cached_label_);
    }
    cached_label_visible_ = visible;
  }

  static int resolve_width(const BalaclavaCliOptions &opts, int cols) {
    int content_cols = cols - opts.pad.left - opts.pad.right;
    if (opts.mpris_width > 0)
      return std::min(opts.mpris_width, content_cols);
    return std::min(46, content_cols * 2 / 3);
  }

  static void resolve_position(MprisPosition pos, int cols, int rows,
                                const Padding &pad, int w, int h, int &row,
                                int &col) {
    int content_cols = cols - pad.left - pad.right;
    switch (pos) {
    case MprisPosition::topleft:
      row = pad.top; col = pad.left; break;
    case MprisPosition::top:
      row = pad.top; col = pad.left + (content_cols - w) / 2; break;
    case MprisPosition::topright:
      row = pad.top; col = cols - pad.right - w; break;
    case MprisPosition::right:
      row = (rows - h) / 2; col = cols - pad.right - w; break;
    case MprisPosition::bottomright:
      row = rows - pad.bottom - h; col = cols - pad.right - w; break;
    case MprisPosition::bottom:
      row = rows - pad.bottom - h; col = pad.left + (content_cols - w) / 2; break;
    case MprisPosition::bottomleft:
      row = rows - pad.bottom - h; col = pad.left; break;
    case MprisPosition::left:
      row = (rows - h) / 2; col = pad.left; break;
    case MprisPosition::center:
      row = (rows - h) / 2; col = pad.left + (content_cols - w) / 2; break;
    default:
      row = pad.top; col = pad.left; break;
    }
  }

  void emit_cursor(std::string &buf, int row, int col) {
    char pos[16];
    int n = snprintf(pos, sizeof(pos), "\033[%d;%dH", row + 1, col + 1);
    buf.append(pos, n);
  }

  void clear_prev(std::string &buf, Color bg) {
    if (prev_width_ <= 0) return;
    fb_->emit_bg(buf, bg);
    for (int r = 0; r < prev_height_; ++r) {
      emit_cursor(buf, prev_row_ + r, prev_col_);
      buf.append(prev_width_, ' ');
    }
    prev_width_ = 0;
    prev_height_ = 0;
    box_drawn_ = false;
    hit_play_pause_ = {};
    hit_prev_ = {};
    hit_next_ = {};
  }

  void save_prev(int row, int col, int w, int h) {
    prev_row_ = row;
    prev_col_ = col;
    prev_width_ = w;
    prev_height_ = h;
  }
};

std::unique_ptr<MprisOverlay> make_mpris_overlay(MprisMode mode);
