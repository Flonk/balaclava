#pragma once

#include "overlay.h"

class MprisFullOverlay final : public MprisOverlay {
public:
  void render(BalaclavaCli &cli) override {
    auto &buf = cli.frame_buf;
    const auto &opts = *opts_;
    const auto &np = cli.np;
    const auto &pb = cli.mpris_pb;
    float beat = cli.frame.beat;

    Color bg = resolve_bg(opts);
    int ww = resolve_width(opts, cols_);
    bool has_text = !np.title.empty() || !np.artist.empty();
    bool has_transport = pb.length_us > 0;

    if (!has_text && !has_transport) {
      clear_prev(buf, bg);
      save_prev(0, 0, 0, 0);
      invalidate();
      return;
    }

    int ph = opts.mpris_pad_h, pv = opts.mpris_pad_v;
    int content_lines = (has_text ? 1 : 0) + (has_transport ? 1 : 0);
    int widget_h = content_lines + 2 * pv;
    int row, col;
    resolve_position(position_, cols_, rows_, opts.pad, ww, widget_h, row, col);

    bool geom_dirty = geometry_changed(ww, position_) ||
                      row != prev_row_ || col != prev_col_ ||
                      ww != prev_width_ || widget_h != prev_height_;
    bool text_dirty = track_changed(np);
    bool status_dirty = np.playback_status != prev_status_ ||
                        pb.playing != prev_playing_;

    if (geom_dirty && prev_width_ > 0)
      clear_prev(buf, bg);

    int inner_w = ww - 2 * (1 + ph);
    if (inner_w < 1) inner_w = 1;
    int inner_col = col + 1 + ph;

    if (!box_drawn_ || geom_dirty) {
      fb_->emit_bg(buf, bg);
      for (int r = 0; r < widget_h; ++r) {
        emit_cursor(buf, row + r, col);
        buf.append(ww, ' ');
      }
      box_drawn_ = true;
      text_dirty = true;
      status_dirty = true;
    }

    if (text_dirty)
      update_track_cache(np, inner_w);

    Align align = align_for(position_, opts.mpris_text_align);
    int cur_line = pv;

    if (has_text && text_dirty) {
      fb_->emit_bg(buf, bg);
      emit_cursor(buf, row + cur_line, inner_col);
      buf.append(inner_w, ' ');

      int text_col = inner_col;
      if (align == Align::center)
        text_col += (inner_w - cached_label_visible_) / 2;
      else if (align == Align::right)
        text_col += inner_w - cached_label_visible_;
      emit_cursor(buf, row + cur_line, text_col);
      fb_->emit_fg(buf, opts.mpris_color);
      fb_->emit_bg(buf, bg);
      buf.append(cached_label_);
    }
    if (has_text) ++cur_line;

    static constexpr int kControlWidth = 9;
    if (has_transport) {
      int transport_row = row + cur_line;
      int bar_width = inner_w - kControlWidth;
      if (bar_width < 1) bar_width = 1;

      bool btn_right = buttons_right(position_, opts.mpris_text_align);

      fb_->emit_bg(buf, bg);

      if (btn_right) {
        emit_cursor(buf, transport_row, inner_col);
        Color filled =
            beat_pulse(opts.mpris_bar_color, opts.beat_color, beat);
        emit_progress_bar(buf, filled, opts.mpris_bar_color, pb, bar_width);
        if (status_dirty) {
          int btn_col = inner_col + bar_width;
          emit_buttons(buf, opts, pb, transport_row, btn_col);
        }
      } else {
        if (status_dirty) {
          emit_cursor(buf, transport_row, inner_col);
          emit_buttons(buf, opts, pb, transport_row, inner_col);
        }
        emit_cursor(buf, transport_row, inner_col + kControlWidth);
        Color filled =
            beat_pulse(opts.mpris_bar_color, opts.beat_color, beat);
        emit_progress_bar(buf, filled, opts.mpris_bar_color, pb, bar_width);
      }
    } else {
      hit_play_pause_ = {};
      hit_prev_ = {};
      hit_next_ = {};
    }

    save_prev(row, col, ww, widget_h);
    prev_ww_ = ww;
    prev_position_ = position_;
    prev_status_ = np.playback_status;
    prev_playing_ = pb.playing;
  }

private:
  enum class Align { left, center, right };

  static Align align_for(MprisPosition pos, MprisTextAlign override_) {
    switch (override_) {
    case MprisTextAlign::left: return Align::left;
    case MprisTextAlign::center: return Align::center;
    case MprisTextAlign::right: return Align::right;
    default: break;
    }
    switch (pos) {
    case MprisPosition::topleft:
    case MprisPosition::bottomleft:
    case MprisPosition::left:
      return Align::left;
    case MprisPosition::topright:
    case MprisPosition::right:
    case MprisPosition::bottomright:
      return Align::right;
    default:
      return Align::center;
    }
  }

  static bool buttons_right(MprisPosition pos, MprisTextAlign override_) {
    return align_for(pos, override_) == Align::left;
  }

  void emit_progress_bar(std::string &buf, Color filled_color,
                         Color unfilled_color, const MprisPlayback &pb,
                         int width) {
    float progress = 0.f;
    if (pb.length_us > 0 && pb.position_us >= 0)
      progress = std::clamp(static_cast<float>(pb.position_us) /
                                static_cast<float>(pb.length_us),
                            0.f, 1.f);

    float filled_f = progress * static_cast<float>(width);
    int filled_full = static_cast<int>(filled_f);
    int partial =
        static_cast<int>((filled_f - static_cast<float>(filled_full)) * 8.f);

    bool in_filled = true;
    fb_->emit_fg(buf, filled_color);
    for (int i = 0; i < width; ++i) {
      if (i < filled_full) {
        buf.append("\xe2\x96\x88"); // █
      } else if (i == filled_full && partial > 0) {
        buf.append(hblocks[partial]);
      } else {
        if (in_filled) {
          fb_->emit_fg(buf, unfilled_color);
          in_filled = false;
        }
        buf.append("\xe2\x94\x80"); // ─
      }
    }
  }

  void emit_buttons(std::string &buf, const BalaclavaCliOptions &opts,
                    const MprisPlayback &pb, int transport_row, int btn_col) {
    fb_->emit_fg(buf, resolve_ui(opts));

    buf.append("\xe2\x8f\xae ");
    hit_prev_ = {transport_row + 1, btn_col + 1, btn_col + 2};

    int pp_col = btn_col + 2;
    const char *icon = pb.playing ? "\xe2\x8f\xb8" : "\xe2\x96\xb6";
    buf.push_back(' ');
    buf.append(icon);
    buf.push_back(' ');
    hit_play_pause_ = {transport_row + 1, pp_col + 1, pp_col + 3};

    int ncol = pp_col + 3;
    buf.push_back(' ');
    buf.append("\xe2\x8f\xad");
    buf.append("  ");
    hit_next_ = {transport_row + 1, ncol + 1, ncol + 4};
  }
};
