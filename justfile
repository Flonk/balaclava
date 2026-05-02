default: build

configure:
    cmake -B build -DCMAKE_BUILD_TYPE=Release

build: configure
    cmake --build build -j$(nproc)

clean:
    rm -rf build
