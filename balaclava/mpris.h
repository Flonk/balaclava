#pragma once

#include <string>

struct NowPlaying {
  std::string artist;
  std::string title;
};

// Query MPRIS for currently playing track.
// Returns empty strings if nothing is playing or DBus unavailable.
NowPlaying mpris_now_playing();
