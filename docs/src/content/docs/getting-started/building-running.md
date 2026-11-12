---
title: Building and Running Tests and Examples
description: Commands to build libkoto, run the 6 CTest suites, and launch the interactive test binaries
sidebar:
  label: Building and Running
  order: 4
---

## Building the repository

From the root of the `koto` repository, configure and build with CMake and Ninja:

```shell
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)
```

To build with AddressSanitizer and UndefinedBehaviorSanitizer as configured in `.github/workflows/ci.yml:24-29`:

```shell
cmake -B build_debug -G Ninja   -DCMAKE_BUILD_TYPE=Debug   -DCMAKE_CXX_FLAGS="-Wall -Wextra -Werror -Wpedantic -Wconversion -Wsign-conversion -Wsign-compare -Wshadow -Wold-style-cast -fsanitize=address,undefined -fno-omit-frame-pointer -g"
cmake --build build_debug --parallel $(nproc)
```

<!-- src: CMakeLists.txt:11-68 -->
<!-- src: .github/workflows/ci.yml:17-72 -->

## Running the automated CTest suite

`CMakeLists.txt:126-141` compiles every `tests/*.cpp` with `-UNDEBUG` (so `assert(...)` statements remain active even in `Release` builds) and registers 6 headless test executables with CTest:

```shell
ctest --test-dir build --output-on-failure
```

| CTest Name | Executable | Source File |
| :--- | :--- | :--- |
| `headless_widgets` | `test_headless_widgets` | `tests/test_headless_widgets.cpp` (11 widget tests) |
| `headless_layout` | `test_headless_layout` | `tests/test_headless_layout.cpp` (5 layout and C++23 tests) |
| `headless_focus_events` | `test_headless_focus_events` | `tests/test_headless_focus_events.cpp` (8 focus and event tests) |
| `headless_render_terminal` | `test_headless_render_terminal` | `tests/test_headless_render_terminal.cpp` (6 color, self-pipe, and TTY tests) |
| `headless_regressions` | `test_headless_regressions` | `tests/test_headless_regressions.cpp` (8 regression and audit test functions) |
| `cjk_emoji_tests` | `test8_text_cjk_emoji` | `tests/test8_text_cjk_emoji.cpp` (9 UTF-8, CJK, and emoji tests) |

You can also run `build/test6_headless_test` (which runs the 5 `test_headless_*` binaries sequentially in isolated child processes).

<!-- src: CMakeLists.txt:126-141 -->
<!-- src: tests/test6_headless_test.cpp:28-36 -->

## Interactive demo binaries

When `KOTO_BUILD_EXAMPLES=ON` and `KOTO_BUILD_TESTS=ON` (both `ON` by default), the build directory also contains interactive terminal demos that must be run inside a real TTY:

| Binary | Source File | Description |
| :--- | :--- | :--- |
| `kotoapp` | `test/main.cpp` | Split layout with `List`, `Gauge`, and `Sparkline`. |
| `test1_tic_tac_toe` | `tests/test1_tic_tac_toe.cpp` | 3x3 grid game with keyboard and mouse input. |
| `test2_key_visualizer` | `tests/test2_key_visualizer.cpp` | Inspector displaying `KeyEvent` codepoints and modifiers. |
| `test3_vbox` | `tests/test3_vbox.cpp` | Nested `Vbox` and `Hbox` layout demo. |
| `test4_text_input` | `tests/test4_text_input.cpp` | Interactive `TextInput` showcase. |
| `test5_yazi` | `tests/test5_yazi.cpp` | 3-pane file browser layout. |
| `test7_dashboard` | `tests/test7_dashboard.cpp` | System dashboard with `Gauge`, `Sparkline`, and `List`. |
| `test9_chess_board` | `tests/test9_chess_board.cpp` | 8x8 `Layout::grid_uniform` board with spatial navigation. |
| `test10_cjk_terminal` | `tests/test10_cjk_terminal.cpp` | Interactive CJK and emoji rendering demo. |
| `test11_text_area` | `tests/test11_text_area.cpp` | Interactive `TextArea` editor demo. |
| `test12_tabs` | `tests/test12_tabs.cpp` | Interactive `Tabs` (`Bar`, `Pill`, `Underline`) demo. |
| `test13_focus_form` | `tests/test13_focus_form.cpp` | Multi-field form using `FocusManager`. |
| `test14_spatial_2d_nav` | `tests/test14_spatial_2d_nav.cpp` | 2D spatial arrow navigation demo. |
| `test15_modal_focus_trap` | `tests/test15_modal_focus_trap.cpp` | Modal focus trapping and background click shielding demo. |
| `test16_animation` | `tests/test16_animation.cpp` | 60 FPS `on_tick` animation demo. |
| `test17_solar_system` | `tests/test17_solar_system.cpp` | Interactive orbital simulation using `on_tick(double dt)`. |
| `test18_3d_engine` | `tests/test18_3d_engine.cpp` | Half-block 3D software renderer demo. |

<!-- src: CMakeLists.txt:120-134 -->

## See also

- [System Requirements](/getting-started/requirements/)
- [Installation](/getting-started/installation/)
- [Headless Testing Guide](/guides/headless-testing/)
