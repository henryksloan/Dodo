cmake_minimum_required(VERSION 3.14)

project(Dodo)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(SOURCES
    src/bus.cpp
    src/cpu.cpp
    src/gameboy.cpp
    src/timer.cpp
    src/mbc/mbc0.cpp
    src/main.cpp
)

add_executable(dodo ${SOURCES})

target_include_directories(dodo PRIVATE ${PROJECT_SOURCE_DIR}/include)
