cmake_minimum_required(VERSION 3.14)

project(Dodo)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

set(SOURCES
    src/bus.cpp
    src/cpu.cpp
    src/gameboy.cpp
    src/timer.cpp
    src/ppu.cpp
    src/mbc/mbc1.cpp
    src/mbc/mbc3.cpp
    src/mbc/mbc5.cpp
    src/main.cpp
)

add_executable(dodo ${SOURCES})

target_include_directories(dodo PRIVATE ${PROJECT_SOURCE_DIR}/include)

find_package(SDL2 REQUIRED)
include_directories(SYSTEM ${SDL2_INCLUDE_DIRS})
target_link_libraries(dodo PRIVATE ${SDL2_LIBRARIES})

add_library(project_warnings INTERFACE)
include(cmake/CompilerWarnings.cmake)
set_project_warnings(project_warnings)
target_link_libraries(dodo INTERFACE project_warnings)
