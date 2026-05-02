cmake_minimum_required(VERSION 3.16)

project(balaclava-cli VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(balaclava main.cpp args.cpp)
target_link_libraries(balaclava PRIVATE balaclava_lib)
