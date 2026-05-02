{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    cmake
    just
    pkg-config
  ];

  buildInputs = with pkgs; [
    fftw
    fftwFloat
    pipewire
  ];
}
