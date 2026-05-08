cmake_minimum_required(VERSION 3.16)

project(balaclava-cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/usage.txt _usage_txt)
string(REPLACE "\\" "\\\\" _usage_txt "${_usage_txt}")
string(REPLACE "\"" "\\\"" _usage_txt "${_usage_txt}")
string(REGEX REPLACE "([^\n]*)\n" "\"\\1\\\\n\"\n" _usage_txt "${_usage_txt}")
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/usage.inc "${_usage_txt}")

add_executable(balaclava main.cpp
  args/args.cpp
  render/common.cpp render/fullscreen/fullscreen.cpp
  render/mpris/mpris.cpp render/mpris/overlay.cpp
  render/oneline/oneline.cpp render/ascii/ascii.cpp)
target_link_libraries(balaclava PRIVATE balaclava_lib ${DBUS_LIBRARIES})
target_include_directories(balaclava PRIVATE ${DBUS_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
