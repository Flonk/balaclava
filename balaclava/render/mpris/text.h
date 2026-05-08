#pragma once

#include "overlay.h"

class MprisTextOverlay final : public MprisOverlay {
public:
  void render(BalaclavaCli &cli) override {
    auto &buf = cli.frame_buf;
    const auto &opts = *opts_;
    const auto &np = cli.np;
    float beat = cli.frame.beat;

    Color bg = resolve_bg(opts);
    int ph = opts.mpris_pad_h, pv = opts.mpris_pad_v;
    int ww = resolve_width(opts, cols_);
    int content_w = ww - 2 * ph;
    if (content_w < 1) content_w = 1;

    bool geom_dirty = geometry_changed(ww, position_);
    bool text_dirty = track_changed(np);

    if (text_dirty || geom_dirty)
      update_track_cache(np, content_w);

    int visible = cached_label_visible_;
    int outer_w = visible + 2 * ph;
    int outer_h = 1 + 2 * pv;

    if (prev_width_ > 0 && (outer_w != prev_width_ || outer_h != prev_height_ ||
                             visible == 0 || geom_dirty))
      clear_prev(buf, bg);

    if (visible == 0) {
      save_prev(0, 0, 0, 0);
      prev_ww_ = ww;
      prev_position_ = position_;
      return;
    }

    int row, col;
    resolve_position(position_, cols_, rows_, opts.pad, outer_w, outer_h, row, col);

    if (!box_drawn_ || text_dirty || geom_dirty) {
      if (ph > 0 || pv > 0) {
        fb_->emit_bg(buf, bg);
        for (int r = 0; r < outer_h; ++r) {
          emit_cursor(buf, row + r, col);
          buf.append(outer_w, ' ');
        }
      }
      box_drawn_ = true;
    }

    emit_cursor(buf, row + pv, col + ph);
    fb_->emit_fg(buf, beat_pulse(opts.mpris_color, opts.beat_color, beat));
    fb_->emit_bg(buf, bg);
    buf.append(cached_label_);

    save_prev(row, col, outer_w, outer_h);
    prev_ww_ = ww;
    prev_position_ = position_;
  }
};
