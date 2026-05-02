#include "args.h"
#include <balaclava/balaclava.h>

#include <csignal>
#include <cstdio>
#include <vector>

static balaclava::Balaclava *g_bala = nullptr;

static void on_signal(int) {
  if (g_bala)
    g_bala->stop();
}

int main(int argc, char *argv[]) {
  auto args = parse_args(argc, argv);

  balaclava::Balaclava bala(args.opts);
  g_bala = &bala;

  std::signal(SIGINT, on_signal);
  std::signal(SIGTERM, on_signal);

  bala.start();

  std::vector<float> values;

  while (bala.poll(values)) {
    if (args.render_mode == RenderMode::tiny) {
      printf("\r");
      for (const float &v : values) {
        int height = static_cast<int>(v * 8.0f);
        static const char *blocks[] = {" ",      "\u2581", "\u2582",
                                       "\u2583", "\u2584", "\u2585",
                                       "\u2586", "\u2587", "\u2588"};
        printf("%s", blocks[height]);
      }
      fflush(stdout);
    } else {
      // TODO: big render mode
    }
  }

  printf("\n");
  return 0;
}
