#include "../../args/args.h"
#include "../common.h"

#include <algorithm>

static const char levels[] = "0123456789abcdefghijklmnopqrstuv";

class AsciiRenderer : public Renderer {
public:
  void enter(const BalaclavaCliOptions &opts, int, int) override {
    gap_ = (opts.gap >= 0) ? opts.gap : 0;
    bar_width_ = (opts.bar_width > 0) ? opts.bar_width : 1;
    bars_ = (opts.bars > 0) ? opts.bars : 32;
  }

  void leave() override {
    printf("\n");
    fflush(stdout);
  }

  void render(BalaclavaCli &cli) override {
    print_bars(cli.frame.bars);

    putchar(' ');
    int beat_idx = static_cast<int>(cli.frame.beat * 31.0f);
    putchar(levels[std::clamp(beat_idx, 0, 31)]);

    if (!cli.frame2.bars.empty()) {
      putchar(' ');
      print_bars(cli.frame2.bars);
      putchar(' ');
      int beat2_idx = static_cast<int>(cli.frame2.beat * 31.0f);
      putchar(levels[std::clamp(beat2_idx, 0, 31)]);
    }

    putchar('\n');
    fflush(stdout);
  }

private:
  static void print_bars(const std::vector<float> &bars) {
    for (const float &v : bars) {
      int idx = static_cast<int>(v * 31.0f);
      putchar(levels[std::clamp(idx, 0, 31)]);
    }
  }
};

std::unique_ptr<Renderer> make_ascii_renderer() {
  return std::make_unique<AsciiRenderer>();
}
