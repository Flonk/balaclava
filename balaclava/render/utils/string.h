#pragma once

#include <string>
#include <wchar.h>

// Measure visible terminal column width of a string, skipping ANSI escapes.
inline int str_width(const char *s, int len) {
  int w = 0;
  for (int i = 0; i < len;) {
    if (s[i] == '\033' && i + 1 < len && s[i + 1] == '[') {
      i += 2;
      while (i < len && s[i] != 'm') ++i;
      if (i < len) ++i;
      continue;
    }
    auto c = static_cast<unsigned char>(s[i]);
    wchar_t cp = 0;
    int bytes = 1;
    if (c < 0x80) { cp = c; }
    else if (c < 0xE0) { cp = c & 0x1F; bytes = 2; }
    else if (c < 0xF0) { cp = c & 0x0F; bytes = 3; }
    else { cp = c & 0x07; bytes = 4; }
    for (int j = 1; j < bytes && i + j < len; ++j)
      cp = (cp << 6) | (static_cast<unsigned char>(s[i + j]) & 0x3F);
    i += bytes;
    int cw = wcwidth(cp);
    if (cw > 0) w += cw;
  }
  return w;
}

inline int str_width(const std::string &s) {
  return str_width(s.data(), static_cast<int>(s.size()));
}

// Truncate a string to fit within max_cols terminal columns.
inline int str_truncate(const char *s, int len, int max_cols) {
  int w = 0;
  int last_cut = 0;
  for (int i = 0; i < len;) {
    if (s[i] == '\033' && i + 1 < len && s[i + 1] == '[') {
      i += 2;
      while (i < len && s[i] != 'm') ++i;
      if (i < len) ++i;
      last_cut = i;
      continue;
    }
    auto c = static_cast<unsigned char>(s[i]);
    int bytes = 1;
    wchar_t cp = c;
    if (c >= 0xF0) { bytes = 4; cp = c & 0x07; }
    else if (c >= 0xE0) { bytes = 3; cp = c & 0x0F; }
    else if (c >= 0xC0) { bytes = 2; cp = c & 0x1F; }
    for (int j = 1; j < bytes && i + j < len; ++j)
      cp = (cp << 6) | (static_cast<unsigned char>(s[i + j]) & 0x3F);
    int cw = wcwidth(cp);
    if (cw < 0) cw = 0;
    if (w + cw > max_cols) break;
    w += cw;
    i += bytes;
    last_cut = i;
  }
  return last_cut;
}
