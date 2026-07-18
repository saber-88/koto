cmake_minimum_required(VERSION 3.10)
project(eztui VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(fmt REQUIRED)

add_library(eztui
    src/terminal.cpp
    src/frame.cpp
    src/layout.cpp
    src/box.cpp
)
target_include_directories(eztui PUBLIC include)
target_link_libraries(eztui PRIVATE fmt::fmt)
target_compile_options(eztui PRIVATE -Wall)

# Main app
add_executable(app test/main.cpp)
target_link_libraries(app PRIVATE eztui)

# Auto-build all tests
file(GLOB TEST_SOURCES "tests/*.cpp")
foreach(TEST_FILE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} PRIVATE eztui)
endforeach()