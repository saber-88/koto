---
title: System Requirements
description: Compiler, CMake, operating system, and terminal requirements verified from CMakeLists.txt and CI workflows
sidebar:
  label: Requirements
  order: 1
---

## Compiler and C++ standard

`libkoto` requires a C++23 compiler (`set(CMAKE_CXX_STANDARD 23)` and `target_compile_features(koto PUBLIC cxx_std_23)` in `CMakeLists.txt:4-5, 46`). It uses `<expected>` (`std::expected`), `<ranges>` (`std::views::enumerate`), `<concepts>`, `<span>`, and C++23 multidimensional subscript operators (`operator[](int row, int col)`).

The continuous integration matrix in `.github/workflows/ci.yml` builds and tests against:

| Compiler | Version in CI | Standard Library | Notes |
| :--- | :--- | :--- | :--- |
| **GCC** | `g++-13` | `libstdc++-13-dev` | Tested in `Release` (`-O2 -D_FORTIFY_SOURCE=2`) and `Debug` (`-fsanitize=address,undefined`). |
| **Clang** | `clang++-17` | `libstdc++-13-dev` | `CMakeLists.txt:48-53` defines `__cpp_concepts=202002L` and `-Wno-builtin-macro-redefined` for Clang `< 19.0` so `libstdc++`'s `<expected>` header is enabled. |

<!-- src: CMakeLists.txt:1-54 -->
<!-- src: .github/workflows/ci.yml:15-44 -->

## Build tools

| Tool | Requirement | Source reference |
| :--- | :--- | :--- |
| **CMake** | `3.15` or newer (`cmake_minimum_required(VERSION 3.15)`) | `CMakeLists.txt:1` |
| **Build generator** | Ninja (`ninja-build`) or Make | `.github/workflows/ci.yml:52, 59` |
| **External library dependencies** | None (zero third-party dependencies; uses only the C++23 standard library and POSIX system headers) | `CMakeLists.txt:15-37` |

<!-- src: CMakeLists.txt:1-37 -->

## Operating system and POSIX headers

`libkoto` targets Linux and POSIX systems and includes the following POSIX headers directly in `include/` and `src/`:
- `<termios.h>` (`include/koto/terminal.hpp:6`, `src/terminal.cpp:10`)
- `<unistd.h>` (`include/koto/event.hpp:10`, `include/koto/io.hpp:9`, `src/terminal.cpp:11`)
- `<poll.h>`, `<fcntl.h>`, `<sys/ioctl.h>` (`src/event.cpp:8, 10, 11`, `src/terminal.cpp:12`)
- `<csignal>` (`sigaction`, `sigaltstack`, `SIGWINCH`, `SIGHUP`, `SIGTERM`, `SIGINT`, `SIGQUIT`, `SIGSEGV`, `SIGABRT`, `SIGBUS`, `SIGFPE`, `SIGILL`) (`src/terminal.cpp:5, 140-209`, `src/event.cpp:1061-1065`)

:::caution
Native Windows (Win32 Console API / MSVC without a POSIX layer) is not supported because `<termios.h>`, `<poll.h>`, `<sys/ioctl.h>`, and `pipe()` are required at compile time. On Windows, build and run inside WSL2.
:::

<!-- src: include/koto/terminal.hpp:6 -->
<!-- src: src/event.cpp:8-12 -->

## Terminal protocols used

At runtime, `Terminal` and `Frame` emit standard ECMA-48 / VT100 and xterm extension sequences:
- Alternate screen buffer: `[?1049h` and `[?1049l` (`src/terminal.cpp:281, 290`)
- Cursor visibility: `[?25l` and `[?25h` (`src/terminal.cpp:299, 308`, `src/frame.cpp:418, 422`)
- Bracketed paste mode: `[?2004h` and `[?2004l` (`src/terminal.cpp:333, 342`)
- Kitty keyboard protocol flag 1: `[>1u` and `[<u` (`src/terminal.cpp:352, 362`)
- SGR mouse capture: `[?1000h[?1006h` (`src/terminal.cpp:373`), plus optional drag `[?1002h` and hover `[?1003h`
- DEC Mode 2026 synchronized output: `[?2026h` and `[?2026l` (`src/frame.cpp:363, 426`)

Terminals that do not implement DEC Mode 2026 or Kitty keyboard protocol ignore those private sequences and fall back to standard CSI/SS3 input sequences.

<!-- src: src/terminal.cpp:277-423 -->
<!-- src: src/frame.cpp:363, 426 -->

## See also

- [Installation](/getting-started/installation/)
- [First Application](/getting-started/first-app/)
- [Building and Running](/getting-started/building-running/)
