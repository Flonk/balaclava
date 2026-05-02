# balaclava

Terminal audio visualizer backed by PipeWire and FFT spectrum analysis.

## Structure

- `libbalaclava/` — shared library: PipeWire audio capture, FFT analysis, visualizer effects
- `balaclava/` — terminal frontend

## Dependencies

- PipeWire
- FFTW3

## Build

```sh
just build
```
