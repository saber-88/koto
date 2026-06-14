cmake_minimum_required( VERSION 3.10)

project( TUI VERSION 1.0 LANGUAGES CXX)

set( CMAKE_CXX_STANDARD 20)
set( CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package( fmt REQUIRED )



add_library(tui 

    src/colors.cpp 
    src/component.cpp 
    src/screen.cpp
)
target_include_directories(tui PUBLIC include)
target_link_libraries(tui PRIVATE fmt::fmt)

# The Main Application Target
add_executable(tui_app 
    test/main.cpp
)
target_link_libraries(tui_app PRIVATE tui)
