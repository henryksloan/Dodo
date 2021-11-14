cmake_minimum_required(VERSION 3.14)

project(Dodo)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

set(SOURCES
    src/bus.cpp
    src/cpu.cpp
    src/gameboy.cpp
    src/timer.cpp
    src/ppu.cpp
    src/mbc/mbc1.cpp
    src/mbc/mbc3.cpp
    src/main.cpp
)

add_executable(dodo ${SOURCES})

target_include_directories(dodo PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(dodo ${SDL2_LIBRARIES})
