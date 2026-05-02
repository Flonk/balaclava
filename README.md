

https://github.com/user-attachments/assets/31196ae2-2fb3-49e5-90e6-cf8df934799e

# Balaclava

![balaclava](./assets/balaclava.jpg)

Terminal audio visualizer backed by PipeWire.



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

Requires CMake, pkg-config, PipeWire, and FFTW3 (single precision).

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

Or ask your favourite AI agent!

## Usage

```sh
balaclava [OPTIONS] [TARGET]
```

`TARGET` is a PipeWire sink/source name (default: `@DEFAULT_SINK@`).

```sh
# Visualize default audio output
balaclava

# Visualize a specific sink
balaclava easyeffects_sink

# Capture from microphone
balaclava --source

# One-line mode
balaclava --render oneline

# Pipe-friendly ASCII output (one char per bar, one line per frame)
balaclava --render ascii

# Tweak the visualizer
balaclava --eq-bass 3.0 --contrast 1.5 --gravity-rise 0.8
```

Run `balaclava --help` for all options.

## ASCII mode

`--render ascii` outputs one line per frame, one character per bar. Each character represents the bar level using 32 discrete levels:

```
0123456789abcdefghijklmnopqrstuv
```

`0` is silence, `v` is maximum. Example output:

```
00236854210000000000000000000000
00358a96432100000000000000000000
002479b7543210000000000000000000
```

---

made with <3 by [t3.at](https://t3.at).

## Credits

Music: Tomas Novoa - Seis Continentes
