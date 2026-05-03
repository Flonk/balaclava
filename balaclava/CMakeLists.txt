cmake_minimum_required(VERSION 3.16)

project(balaclava-cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)

add_executable(balaclava main.cpp args.cpp render.cpp mpris.cpp)
target_link_libraries(balaclava PRIVATE balaclava_lib ${DBUS_LIBRARIES})
target_include_directories(balaclava PRIVATE ${DBUS_INCLUDE_DIRS})
