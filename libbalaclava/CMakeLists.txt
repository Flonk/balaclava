cmake_minimum_required(VERSION 3.16)

project(libbalaclava VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFTW REQUIRED fftw3f)
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)

add_library(balaclava_lib
    src/balaclava.cpp
    src/audiocollector.cpp
    src/spectrumanalyzer.cpp
    src/visualizereffects.cpp
)

set_target_properties(balaclava_lib PROPERTIES OUTPUT_NAME balaclava)

target_include_directories(balaclava_lib
    PUBLIC include
    PRIVATE ${PIPEWIRE_INCLUDE_DIRS} ${FFTW_INCLUDE_DIRS}
)

target_link_libraries(balaclava_lib
    PRIVATE ${PIPEWIRE_LIBRARIES} ${FFTW_LIBRARIES}
)
