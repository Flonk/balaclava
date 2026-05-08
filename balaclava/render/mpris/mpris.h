#pragma once

#include <cstdint>
#include <string>

struct NowPlaying {
  std::string artist;
  std::string title;
  std::string playback_status; // "Playing"|"Paused"|"Stopped"|""
  int64_t position_us = -1;   // microseconds, -1 = unknown
  int64_t length_us = -1;     // from mpris:length, -1 = unknown
};

// Query MPRIS for currently playing/paused track.
NowPlaying mpris_now_playing();

// Transport controls (fire-and-forget DBus calls).
void mpris_play_pause();
void mpris_next();
void mpris_previous();

// Volume control. Step is 0.05 (5%), clamped to [0, 1].
void mpris_volume_up();
void mpris_volume_down();
