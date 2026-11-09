# koto

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform: Linux](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://kernel.org)
[![Headless Tests](https://img.shields.io/badge/Headless%20Tests-37%20Passing-brightgreen.svg)]()
[![Unicode Tests](https://img.shields.io/badge/Unicode-9%20Passing-brightgreen.svg)]()
[![Compiler Warnings](https://img.shields.io/badge/Compiler%20Warnings-Zero-brightgreen.svg)]()

koto is a lightweight, dependency-free C++23 Terminal User Interface (TUI) library engineered for Linux and POSIX systems. It provides differential double-buffered rendering, DEC Mode 2026 synchronized updates, deterministic input decoding, and zero-allocation frame layouts.

Comprehensive architectural documentation, widget guides, and API specifications are maintained in the [`docs/`](docs/) directory.

---

## Architectural Highlights

* **Differential Double-Buffered Rendering**: Minimizes standard output write volume by comparing front and back cell buffers per frame with ANSI SGR color and attribute state caching.
* **DEC Mode 2026 Synchronized Updates**: Emits private mode sequences (`\033[?2026h` / `\033[?2026l`) to halt terminal rasterization during buffer emission, guaranteeing atomic, tear-free frame presentation on supported GPU-accelerated terminal emulators.
* **Deterministic Input Decoding**: Utilizes a monotonic 35 ms escape sequence deadline timer combined with `ioctl(FIONREAD)` burst draining, resolving escape ambiguity without CPU busy-spinning or input lag.
* **Zero-Allocation Layout Deduction**: Computes multi-element horizontal and vertical flex constraints at compile time using C++ Non-Type Template Parameters (NTTP), enabling structured bindings (`auto [left, right] = Layout::horizontal(...)`) with zero heap allocation.
* **Concept-Constrained Focus Engine**: Provides 1D Tab cycling and 2D Manhattan distance spatial routing constrained by the C++20 `FocusableState` concept.
* **Hardware-Style Hit-Buffer**: Tags back-buffer cells with 32-bit universal `Id` tokens for `O(1)` spatial mouse click, hover, and scroll wheel dispatch.
* **Wide Unicode & Grapheme Integrity**: Full UAX-11 East Asian Width calculation, 2-column continuation cells, 31-byte grapheme storage for multi-codepoint ZWJ emoji clusters, and boundary-safe clipping.
* **POSIX Stream I/O**: `EINTR`-safe `write_all()` loop with short-write protection, asynchronous signal handling via a POSIX self-pipe (`SIGWINCH`), and async-signal-safe terminal restoration.

---

## Architectural Contracts & Operational Guarantees

* **Application Lifecycle & Raw Mode**: Terminal raw mode, mouse reporting, and alternate screen buffer are initialized strictly when `App::run()` executes, not in the `App` constructor. The library does not intercept or hardcode any global exit hotkeys; developers manage application termination explicitly via `app.quit()`.
* **Crash Recovery & Signal Chaining**: Fatal crash handlers (`SIGSEGV`, `SIGABRT`, `SIGBUS`, `SIGFPE`, `SIGILL`) and `std::terminate` emit a minimal async-signal-safe escape sequence to restore terminal modes before chaining directly to preexisting host application handlers (`sa_sigaction` / `sa_handler`). Inherited `nohup` (`SIG_IGN` on `SIGHUP`) is respected without override.
* **Bracketed Paste Preservation**: Multi-byte bracketed paste streams (`\033[200~` through `\033[201~`) are exempted from the 35 ms short-escape timeout. Incoming chunks refresh the deadline timer, ensuring large or fragmented pastes across SSH or tmux arrive as atomic `PasteEvent`s without being sliced into raw keystrokes.
* **Non-ESC Stale Input Recovery**: Truncated or orphaned non-escape bytes (such as an incomplete UTF-8 lead byte) are discarded after the 35 ms timeout, preventing event loop stalling and preserving 60 FPS animation tick timing.
* **Focus State Lifetimes**: `FocusManager` stores non-owning references (`&state`) to widget state structs. States must outlive the manager, be explicitly unregistered using `fm.remove(state)` or `fm.remove(id)`, or be scoped inside an `IdScope` modal trap. In debug builds (`#ifndef NDEBUG`), pointer-to-hash collision assertions detect identical 32-bit tokens across distinct state addresses.
* **Terminal Protocol Capabilities**: Emits Kitty keyboard progressive enhancement flag 1 (`\033[>1u`) on startup and pops it on teardown (`\033[<u`). On terminals lacking Kitty keyboard support, input decoding falls back transparently to standard ANSI, CSI, and SS3 escape sequences.

---

## Quick Start

```cpp
#include <koto/koto.hpp>

using namespace koto;

int main()
{
    App app;

    ListState todo({"Refactor core", "DEC Mode 2026", "Zero-alloc layout"});
    todo.navigation = ListNavigation::Wrap;

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q' || (key.ctrl && (key.ch == 'c' || key.ch == 'C'))) {
            app.quit();
        }
        todo.handle_event(key);
    });

    app.on_render([&](Frame& frame) {
        auto [header, list_area, footer] = Layout::vertical(frame.area(), {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(1),
        });

        Box()
            .border_style(BorderStyle::Rounded)
            .title(" Tasks ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .draw(frame, header);

        List(todo)
            .border_style(BorderStyle::Single)
            .highlight_fg(Color::Emerald)
            .highlight_symbol("> ")
            .draw(frame, list_area);

        Text("Navigate: [j/k or Up/Down] | Quit: [q or Ctrl+C]")
            .fg(Color::DarkGray)
            .draw(frame, footer);
    });

    auto res = app.run();
    if (!res) {
        std::fprintf(stderr, "Terminal error: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return 1;
    }
    return 0;
}
```

---

## Building and Integration

### Prerequisites

* **Compiler**: GCC 13+ or Clang 17+ (C++23 standard support required)
* **Build System**: CMake 3.25+ and Ninja (or Make)
* **Target OS**: Linux / POSIX-compliant environment

### CMake Integration

Add `koto` as a subdirectory or submodule in your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_tui_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(koto)
add_executable(my_tui_app src/main.cpp)
target_link_libraries(my_tui_app PRIVATE koto)
```

### Compiling and Running Tests

```bash
# Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel $(nproc)

# Execute automated test suites via CTest
ctest --test-dir build --output-on-failure

# Or run individual modular suites directly
./build/test_headless_widgets
./build/test_headless_layout
./build/test_headless_focus_events
./build/test_headless_render_terminal
./build/test_headless_regressions
./build/test8_text_cjk_emoji
```

---

## Test Suite and Verification

The library includes 46 automated unit, layout, snapshot, and regression tests:

| Test Suite | Tests | Scope |
| :--- | :--- | :--- |
| `test_headless_widgets` | 11 | Rendering, borders, padding, and state behavior for Box, List, Text, TextArea, Tabs, Button, Gauge, Sparkline, and Rule. |
| `test_headless_layout` | 5 | Constraint clamping, zero-allocation compile-time deduction, 2D grids, and C++23 native operations. |
| `test_headless_focus_events` | 8 | Kitty CSI-u decoding, hit-buffer tagging, FocusManager 1D/2D routing, modal focus trapping, and mouse routing. |
| `test_headless_render_terminal` | 6 | Color downscaling, terminal TTY detection, POSIX self-pipe signaling, and differential buffer updates. |
| `test_headless_regressions` | 7 | C1 escape timeout, H3/M2 SIGHUP nohup inheritance, R1 slow paste preservation, R5 truncated input, and Move-only Frame semantics. |
| `test8_text_cjk_emoji` | 9 | UAX-11 widths, 2-column continuation cells, multi-codepoint ZWJ emoji clusters, and grapheme-atomic editing. |

### Quality Gates

* **Compiler Flags**: Compiles cleanly with zero warnings under `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wsign-compare -Wshadow -Wold-style-cast -D_FORTIFY_SOURCE=2 -O2`.
* **Sanitizers**: Validated with zero memory leaks and zero runtime issues under AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).
* **Release Assertions**: Build configurations explicitly undefine `NDEBUG` for test targets (`-UNDEBUG`), ensuring all assertions actively evaluate in both Release and Debug builds.

---

## Showcase Applications

The repository contains standalone demo applications demonstrating individual subsystem capabilities:

| Binary | Description |
| :--- | :--- |
| `kotoapp` | Multi-pane workspace featuring List, TextArea, Gauge, and dynamic layout splits. |
| `test1_tic_tac_toe` | Interactive 2-player board game with keyboard navigation and mouse click hit-testing. |
| `test2_key_visualizer` | Keystroke inspector reporting modifier bitmasks, Kitty CSI-u codes, and hex codepoints. |
| `test4_text_input` | Interactive text field with horizontal sliding window scrolling and cursor synchronization. |
| `test5_yazi` | 3-column file manager layout demonstrating parent-child synchronized navigation. |
| `test7_dashboard` | Systems monitor layout with real-time gauges, sparkline trends, and status tables. |
| `test9_chess_board` | 8x8 chessboard with 2D spatial navigation and piece placement hit-testing. |
| `test11_text_area` | Multi-line code editor with line number gutter, sticky cursor, and scrolling margins. |
| `test12_tabs` | Multi-page tab navigation across Bar, Pill, and Underline styles. |
| `test15_modal_focus_trap` | Modal popup dialog with background click shielding and focus restoration. |
| `test17_solar_system` | 60 FPS ASCII simulation rendering orbital bodies and vector math. |
| `test18_3d_engine` | Real-time software 3D wireframe rasterizer with rotational matrices. |

---

## Documentation Index

Detailed documentation is available in the [`docs/`](docs/) directory:

* **Getting Started**: [`docs/getting-started.md`](docs/getting-started.md)
* **Architecture**:
  * [Rendering Pipeline](docs/architecture/rendering-pipeline.md)
  * [Event Loop & Input Parsing](docs/architecture/event-loop.md)
  * [Focus Management & Navigation](docs/architecture/focus-system.md)
* **API Reference**:
  * [Application Core (`koto::App`)](docs/api/app.md)
  * [Virtual Frame & Hit-Buffer (`koto::Frame`)](docs/api/frame.md)
  * [Layout Engine (`koto::Layout`)](docs/api/layout.md)
  * [Terminal Abstraction (`koto::Terminal`)](docs/api/terminal.md)
  * [Event System (`koto::Event`)](docs/api/event.md)
  * [Color Management (`koto::Color`)](docs/api/color.md)
* **Widget Guides**:
  * [Box](docs/widgets/box.md), [Button](docs/widgets/button.md), [List](docs/widgets/list.md), [Tabs](docs/widgets/tabs.md), [Text](docs/widgets/text.md), [TextArea](docs/widgets/text-area.md), [TextInput](docs/widgets/text-input.md), [Gauge](docs/widgets/gauge.md), [Sparkline](docs/widgets/sparkline.md), [Rule](docs/widgets/rule.md)
* **Technical Guides**:
  * [Unicode & CJK Handling](docs/etc/unicode-handling.md)
  * [Performance Optimization](docs/etc/performance-guide.md)
  * [Troubleshooting & Terminal Compatibility](docs/etc/troubleshooting.md)

---

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
