cmake_minimum_required(VERSION 3.18)

add_subdirectory(tools)

set(LLVM_MOS_PLATFORM rp6502)
find_package(llvm-mos-sdk REQUIRED)

project(MY-RP6502-PROJECT C CXX ASM)

add_executable(RPTracker)
rp6502_executable(RPTracker
    DATA file
    RESET file
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.hlp
)
target_sources(RPTracker PRIVATE
    src/main.c
    src/input.c
    src/instruments.c
    src/opl.c
    src/player.c
    src/screen.c
)
