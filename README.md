# Balaclava

![balaclava](./assets/balaclava.jpg)

Terminal audio visualizer backed by PipeWire.

Balaclava is still in alpha, so expect breaking changes!

https://github.com/user-attachments/assets/e9560725-a24f-416a-92ba-013f06c85d7e

## Features

- Real-time FFT-based spectrum visualization via PipeWire
- Media Controls via MPRIS, with keyboard shortcuts
- Beat detection with configurable pulse effects
- Many effects
- Tons of customizations!

## Installation

### NixOS (flake)

Add balaclava as a flake input:

```nix
# flake.nix
inputs.balaclava = {
  url = "github:Flonk/balaclava";
  inputs.nixpkgs.follows = "nixpkgs";
};
```

Then add the package to your home-manager or system config:

```nix
home.packages = [ inputs.balaclava.packages.${pkgs.system}.default ];
```

### Build from source

Requires CMake, pkg-config, PipeWire, dbus, and FFTW3 (single precision).

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/balaclava/balaclava
```

With nix (no need to install dependencies):

```sh
nix build  # result in ./result/bin/balaclava
# or for development:
nix develop  # drops into a shell with all deps
just build
```

## Usage

```sh
balaclava [OPTIONS]
```

```sh
# Visualize default audio output
balaclava

# Visualize a specific sink
balaclava --sink easyeffects_sink

# Capture from microphone
balaclava --source

# One-line mode
balaclava --render oneline

# Pipe-friendly ASCII output (one char per bar, one line per frame)
balaclava --render ascii

# Dual input: primary sink with secondary microphone behind it
balaclava --sink --source2
```

### Colors

Colors accept OKLCH (`"L C H"`) or hex (`"#rrggbb"`). OKLCH values are lightness (0-1), chroma (0-0.4), and hue (0-360). Gradients interpolate in OKLCH for perceptually smooth transitions.

```sh
# OKLCH: bright green gradient
balaclava --color-lo "0.4 0.15 145" --color-hi "0.75 0.15 145"

# Hex: same thing
balaclava --color-lo "#166534" --color-hi "#4ade80"
```

### Themes

```sh
# Nord
balaclava --bg-color "#2E3440" \
  --color-lo "#5E81AC" --color-hi "#88C0D0" \
  --beat-color "#EBCB8B" \
  --color-lo2 "#3B4252" --color-hi2 "#2E3440" \
  --mpris-color "#D8DEE9" --mpris-bar-color "#4C566A"

# Catppuccin Mocha
balaclava --bg-color "#1E1E2E" \
  --color-lo "#CBA6F7" --color-hi "#F5C2E7" \
  --beat-color "#FAB387" \
  --color-lo2 "#45475A" --color-hi2 "#313244" \
  --mpris-color "#CDD6F4" --mpris-bar-color "#585B70"

# Dracula
balaclava --bg-color "#282A36" \
  --color-lo "#BD93F9" --color-hi "#FF79C6" \
  --beat-color "#50FA7B" \
  --color-lo2 "#44475A" --color-hi2 "#282A36" \
  --mpris-color "#F8F8F2" --mpris-bar-color "#6272A4"

# Gruvbox
balaclava --bg-color "#282828" \
  --color-lo "#D65D0E" --color-hi "#D79921" \
  --beat-color "#CC241D" \
  --color-lo2 "#3C3836" --color-hi2 "#282828" \
  --mpris-color "#EBDBB2" --mpris-bar-color "#504945"

# Tokyo Night
balaclava --bg-color "#1A1B26" \
  --color-lo "#7AA2F7" --color-hi "#7DCFFF" \
  --beat-color "#9ECE6A" \
  --color-lo2 "#24283B" --color-hi2 "#1A1B26" \
  --mpris-color "#C0CAF5" --mpris-bar-color "#414868"
```

### Hotkeys

| Key         | Action         |
| ----------- | -------------- |
| Space       | Play/pause     |
| Ctrl+Right  | Next track     |
| Ctrl+Left   | Previous track |
| Ctrl+Up     | Volume up      |
| Ctrl+Down   | Volume down    |
| Ctrl+Scroll | Volume up/down |

Disable with `--hotkeys off` or `--volume-scroll off`.

### MPRIS widget

The now-playing widget shows track info, playback controls, and a progress bar. It auto-positions based on bar layout.

```sh
# Full widget with progress bar and buttons
balaclava --mpris-mode full

# Text-only (default)
balaclava --mpris-mode text

# Disable
balaclava --mpris-mode off

# Custom position and colors
balaclava --mpris-mode full --mpris-position bottomright \
  --mpris-color "#CDD6F4" --mpris-bar-color "#585B70"
```

### ASCII mode

If you would like to use balaclava output in another program you should really use libbalaclava, but I understand that consuming strings in a unix pipe is easier for many people. For your convenience, balaclava has `--render ascii` that outputs one line per frame, each line having up to four space-separated sections:

```
<bars> <beat> [<bars2> <beat2>]
```

- **bars** — one character per bar, 32 levels (`0-9` and `a-v`)
- **beat** — single character for beat intensity
- **bars2** / **beat2** — same for secondary input (only with `--sink2`/`--source2`)

Example output (single input):

```
00236854210000000000000000000000 3
00358a96432100000000000000000000 8
002479b7543210000000000000000000 5
```

Run `balaclava --help` for all options.

---

Made with <3 by [t3.at](https://t3.at).

## Acknowledgements

[Cava](https://github.com/karlstav/cava) is of course the OG and balaclava copied liberally from the original. Balaclava has latency improvements and (in my opinion anyway) a better visual output; I made balaclava a new project so I could AI slop the codebase without having to go through peer review.
