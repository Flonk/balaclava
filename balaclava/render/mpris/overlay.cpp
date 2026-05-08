#include "overlay.h"
#include "full.h"
#include "text.h"

std::unique_ptr<MprisOverlay> make_mpris_overlay(MprisMode mode) {
  switch (mode) {
  case MprisMode::text: return std::make_unique<MprisTextOverlay>();
  case MprisMode::full: return std::make_unique<MprisFullOverlay>();
  case MprisMode::off: return nullptr;
  }
  return nullptr;
}
