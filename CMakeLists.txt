cmake_minimum_required(VERSION 3.10)
# set(CMAKE_C_FLAGS_DEBUG "-g -O0")
# set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(audioProgramming C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PORTAUDIO REQUIRED portaudio-2.0)

file(GLOB_RECURSE SOURCES
    src/*.c
)

# Enable warnings
add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wconversion
    -g
)

# AddressSanitizer (heap corruption, OOB, use-after-free)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

add_executable(main ${SOURCES})
target_compile_options(main PRIVATE -g -O0)

target_include_directories(main PRIVATE
    include
    ${PORTAUDIO_INCLUDE_DIRS}
)

target_link_libraries(main
    ${PORTAUDIO_LIBRARIES}
)

