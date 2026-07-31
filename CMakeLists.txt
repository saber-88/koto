cmake_minimum_required(VERSION 3.10)
project(eztui VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(koto
    src/terminal.cpp
    src/frame.cpp
    src/layout.cpp
    src/box.cpp
    src/event.cpp
    src/color.cpp
    src/text.cpp
)

target_include_directories(koto PUBLIC include)
target_compile_options(koto PRIVATE -Wall)

# Main app
add_executable(kotoapp test/main.cpp)
target_link_libraries(kotoapp PRIVATE koto)

# Auto-build all tests
file(GLOB TEST_SOURCES "tests/*.cpp")
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE koto)
endforeach()