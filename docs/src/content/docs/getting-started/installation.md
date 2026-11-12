---
title: Installation and CMake Integration
description: How to add libkoto to a CMake project via FetchContent, add_subdirectory, find_package, or pkg-config
sidebar:
  label: Installation
  order: 2
---

## CMake options and exported targets

`CMakeLists.txt` defines the static library target `koto` and the alias target `koto::koto` (`CMakeLists.txt:15, 39`) with `PUBLIC` compile feature `cxx_std_23` (`CMakeLists.txt:46`).

| CMake Option | Default | Description |
| :--- | :--- | :--- |
| `KOTO_BUILD_TESTS` | `ON` | Builds all test executables in `tests/*.cpp` and registers 6 CTest suites. |
| `KOTO_BUILD_EXAMPLES` | `ON` | Builds the `kotoapp` executable from `test/main.cpp`. |
| `KOTO_ENABLE_WERROR` | `OFF` | Appends `-Werror` to `koto`'s private compile options. |

<!-- src: CMakeLists.txt:11-47 -->

## Option 1: CMake `FetchContent`

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_koto_app LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(
    koto
    GIT_REPOSITORY https://github.com/saber-88/koto.git
    GIT_TAG        main
)
set(KOTO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(KOTO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(koto)

add_executable(my_koto_app src/main.cpp)
target_link_libraries(my_koto_app PRIVATE koto::koto)
```

## Option 2: Vendored subdirectory (`add_subdirectory`)

Clone or add `koto` as a Git submodule and include it with `add_subdirectory`:

```shell
git submodule add https://github.com/saber-88/koto.git external/koto
```

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_koto_app LANGUAGES CXX)

set(KOTO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(KOTO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/koto)

add_executable(my_koto_app src/main.cpp)
target_link_libraries(my_koto_app PRIVATE koto::koto)
```

Because `koto` exports `target_compile_features(koto PUBLIC cxx_std_23)` and the Clang `< 19.0` `__cpp_concepts=202002L` workaround as `PUBLIC` (`CMakeLists.txt:46-53`), linking against `koto::koto` propagates both the include directory and the C++23 requirement to `my_koto_app`.

<!-- src: CMakeLists.txt:39-53 -->

## Option 3: System installation (`find_package` and `pkg-config`)

`CMakeLists.txt:70-117` installs the library, headers (`include/koto`), CMake package config files (`kotoTargets.cmake`, `kotoConfig.cmake`, `kotoConfigVersion.cmake` under `${CMAKE_INSTALL_LIBDIR}/cmake/koto`), and `koto.pc` under `${CMAKE_INSTALL_LIBDIR}/pkgconfig`:

```shell
cmake -B build -DCMAKE_BUILD_TYPE=Release -DKOTO_BUILD_TESTS=OFF -DKOTO_BUILD_EXAMPLES=OFF
cmake --build build --parallel
sudo cmake --install build
```

Consumer `CMakeLists.txt` using `find_package`:

```cmake
cmake_minimum_required(VERSION 3.15)
project(my_koto_app LANGUAGES CXX)

find_package(koto 1.0.0 REQUIRED)

add_executable(my_koto_app src/main.cpp)
target_link_libraries(my_koto_app PRIVATE koto::koto)
```

Or via `pkg-config` (`cmake/koto.pc.in:1-10`):

```shell
g++ -std=c++23 main.cpp $(pkg-config --cflags --libs koto) -o my_koto_app
```

<!-- src: CMakeLists.txt:70-117 -->
<!-- src: cmake/koto.pc.in:1-10 -->
<!-- src: cmake/kotoConfig.cmake.in:1-5 -->

## See also

- [System Requirements](/getting-started/requirements/)
- [First Application](/getting-started/first-app/)
- [Building and Running](/getting-started/building-running/)
