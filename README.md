# balaclava

Audio visualizer plugin for QuickShell. Captures audio via PipeWire, runs FFT spectrum analysis, and exposes the data as a QML texture.

## Dependencies

- Qt 6 (Core, Qml, Quick)
- PipeWire
- FFTW3

## Build

```sh
./build.sh
```

Or manually:

```sh
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```
