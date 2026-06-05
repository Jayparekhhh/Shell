cmake_minimum_required(VERSION 3.10)

project(codecrafters-shell)

file(GLOB_RECURSE SOURCE_FILES src/*.c src/*.h)

set(CMAKE_C_STANDARD 23)

add_executable(shell ${SOURCE_FILES}
        src/helpers.h)

target_link_libraries(shell PRIVATE readline)
