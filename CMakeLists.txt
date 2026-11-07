cmake_minimum_required(VERSION 3.15)
project(koto VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(KOTO_BUILD_TESTS "Build koto test suite" ON)
option(KOTO_BUILD_EXAMPLES "Build koto example applications" ON)

add_library(koto
    src/terminal.cpp
    src/frame.cpp
    src/layout.cpp
    src/box.cpp
    src/event.cpp
    src/color.cpp
    src/text.cpp
    src/vbox.cpp
    src/hbox.cpp
    src/list.cpp
    src/utf8.cpp
    src/text_input.cpp
    src/app.cpp
    src/rule.cpp
    src/gauge.cpp
    src/sparkline.cpp
    src/text_area.cpp
    src/tabs.cpp
    src/focus.cpp
    src/fill.cpp
    src/button.cpp
)

add_library(koto::koto ALIAS koto)

target_include_directories(koto PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(koto PUBLIC cxx_std_23)

target_compile_options(koto PRIVATE 
    -Wall
    -Werror
    -Wextra
    -Wpedantic
    -Wconversion
    -Wsign-conversion
    -Wsign-compare
    -Wshadow
    -Wold-style-cast
)

# Installation targets
install(TARGETS koto
    EXPORT kotoTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/koto
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Export CMake package
install(EXPORT kotoTargets
    FILE kotoTargets.cmake
    NAMESPACE koto::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/koto
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/kotoConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/kotoConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/kotoConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/koto
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/kotoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/kotoConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/koto
)

# pkg-config file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/koto.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/koto.pc"
    @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/koto.pc"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

# Main app
if(KOTO_BUILD_EXAMPLES)
    add_executable(kotoapp test/main.cpp)
    target_link_libraries(kotoapp PRIVATE koto::koto)
endif()

# Auto-build all tests
if(KOTO_BUILD_TESTS)
    enable_testing()
    file(GLOB TEST_SOURCES "tests/*.cpp")
    foreach(TEST_FILE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_FILE})
        target_link_libraries(${TEST_NAME} PRIVATE koto::koto)
    endforeach()
    add_test(NAME headless_snapshot_tests COMMAND test6_headless_test)
    add_test(NAME cjk_emoji_tests COMMAND test8_text_cjk_emoji)
endif()
