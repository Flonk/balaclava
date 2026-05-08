#include "../../args/args.h"
#include "../common.h"
#include "../utils/color/ansi.h"

#include <algorithm>

class OnelineRenderer : public Renderer {
public:
  void enter(const BalaclavaCliOptions &opts, int, int) override {
    gap_ = (opts.gap >= 0) ? opts.gap : 0;
    bar_width_ = (opts.bar_width > 0) ? opts.bar_width : 1;
    bars_ = (opts.bars > 0) ? opts.bars : 12;
    printf("\033[?25l");
    fflush(stdout);
  }

  void leave() override {
    printf("\033[?25h");
    printf("\n");
    fflush(stdout);
  }

  void render(BalaclavaCli &cli) override {
    auto &buf = cli.frame_buf;
    buf.clear();
    buf.append("\r");

    print_bars(cli.frame.bars, cli.frame.beat, cli.opts.colors,
               cli.opts.beat_color, bar_width_, gap_, buf);

    if (!cli.frame2.bars.empty()) {
      buf.append("\033[0m ");
      print_bars(cli.frame2.bars, cli.frame2.beat, cli.opts.bg_colors,
                 cli.opts.beat_color, bar_width_, gap_, buf);
    }

    buf.append("\033[0m");
    fwrite(buf.data(), 1, buf.size(), stdout);
    fflush(stdout);
  }

private:
  static void print_bars(const std::vector<float> &values, float beat,
                          const Gradient2D &grad, Color beat_color,
                          int bar_width, int gap, std::string &buf) {
    int count = static_cast<int>(values.size());
    for (int i = 0; i < count; ++i) {
      int height = static_cast<int>(values[i] * 8.0f);
      Color c = grad.compute(values[i]);
      if (beat > 0.f) {
        c.r = mix(c.r, beat_color.r, beat);
        c.g = mix(c.g, beat_color.g, beat);
        c.b = mix(c.b, beat_color.b, beat);
      }
      emit_color(buf, kFgPrefix, c);
      const char *ch = vblocks[std::clamp(height, 0, 8)];
      for (int w = 0; w < bar_width; ++w) {
        buf.append(ch);
      }
      if (i < count - 1 && gap > 0) {
        buf.append(gap, ' ');
      }
    }
  }
};

std::unique_ptr<Renderer> make_oneline_renderer() {
  return std::make_unique<OnelineRenderer>();
}
