# koto (琴)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-orange.svg?logo=linux)](https://kernel.org)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()

> A modern, dependency-free, high-performance C++23 Terminal User Interface (TUI) framework designed for deterministic state management, immediate-mode rendering, and automated snapshot testing.

---

## 🌟 Why koto?

Traditional terminal UI development in C and C++ has long been plagued by low-level, monolithic C APIs (`ncurses`), callback spaghetti, fragile escape sequence parsing, terminal teardown corruption, and a complete inability to run headless unit tests in CI/CD environments.

**koto** is engineered from scratch in modern **C++23** to solve these developer pain points:

- **Zero External Dependencies**: Pure standard library and direct POSIX system calls. No bloated ncurses, no terminfo dependencies.
- **Differential Double-Buffered Renderer**: Only dirty terminal cells emit ANSI SGR color sequences, batched into a single `write()` syscall per frame.
- **Mathematical Escape & Modifier Engine**: Decodes any ANSI, CSI, SS3, and CSI u keyboard sequence with exact bitmask modifier calculation ($M = 1 + \text{flags}$) across Kitty, Ghostty, Alacritty, WezTerm, Foot, GNOME Terminal, Konsole, xterm, tmux, and SSH.
- **Headless Snapshot Testing by Default**: Create an in-memory `Frame(rows, cols)` and assert on layout and widgets using `frame.to_plain_text()` without needing a live TTY.
- **Deterministic State Architecture ($\text{UI} = f(\text{state})$)**: State objects (`ListState`, `TextInputState`) own business and navigation logic, while widgets act as pure, lightweight views.
- **Hardware Cursor Synchronization**: Direct cursor positioning and show/hide logic synced with active input widgets.
- **Zero Warnings**: Compiles cleanly with `-Wall -Werror -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wold-style-cast`.

---

## 🏛️ Layered Architecture

```
+-------------------------------------------------------------+
| Layer 3: Declarative Application Runtime (koto::App)        |
| - RAII lifecycle, event dispatch, 60 FPS tick, on_render    |
+-------------------------------------------------------------+
| Layer 2: State-Driven Widgets & Layout Engine               |
| - Box (Rounded/Single/Double/Thick borders, TitleAlign)      |
| - Text (Hard \n preservation, 4-space tab stops, Ellipsis)  |
| - List (Full-width row highlights, Wrap/Clamp, prefix mark) |
| - TextInput (UTF-8 boundaries, sliding window scroll)       |
| - Layout (Vertical & Horizontal flex, boundary clamping)    |
+-------------------------------------------------------------+
| Layer 1: The Metal Core                                     |
| - Terminal (RAII raw-mode, alternate screen buffer)         |
| - Frame (Double-buffered cell grid, headless virtual mode)  |
| - EventLoop (signalfd SIGWINCH, universal escape parser)    |
| - Cell & Color (TrueColor 24-bit, 256-color, hex parsing)   |
| - UTF-8 (Codepoint width calculation, wide CJK support)     |
+-------------------------------------------------------------+
```

---

## 🚀 Quickstart

### Prerequisites
- Clang 16+ or GCC 13+ with **C++23** support
- CMake 3.10+
- Linux (POSIX compliant)

### Build and Run Demo
```bash
git clone https://github.com/saber-88/myTui.git koto
cd koto
cmake -B build -S .
cmake --build build

# Run the showcase application
./build/kotoapp
```

---

## 💻 Minimal Application Example

```cpp
#include <koto/koto.hpp>

using namespace koto;

int main()
{
    App app;

    ListState todo({"Build core engine", "Add headless testing", "Create Yazi file manager"});
    todo.navigation = ListNavigation::Wrap;

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q') {
            app.quit();
        }
        todo.handle_event(key); // Handles Up/Down arrows and j/k automatically
    });

    app.on_render([&](Frame& frame) {
        auto rows = Layout::vertical(frame.area(), {
            Constraint::fixed(3), // Header
            Constraint::fill(),   // List
            Constraint::fixed(1), // Status bar
        });

        Box()
            .border_style(BorderStyle::Rounded)
            .title(" koto Todo ")
            .title_align(TitleAlign::Left)
            .draw(frame, rows[0]);

        List(todo)
            .border_style(BorderStyle::Single)
            .highlight_fg("#00FFCC")
            .highlight_bg("#003344")
            .highlight_symbol("> ")
            .draw(frame, rows[1]);

        Text("Press [j/k] to navigate, [q] to quit")
            .fg("#888888")
            .draw(frame, rows[2]);
    });

    app.run();
    return 0;
}
```

---

## 🧪 Headless Snapshot Testing (CI/CD Ready)

Unlike legacy frameworks, `koto` does not require a physical terminal to test your interface. You can instantiate a headless `Frame` and verify rendering deterministically:

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

void test_todo_rendering()
{
    // Allocate a virtual 5 row x 20 column screen in memory
    Frame frame(5, 20);

    ListState state({"alpha", "beta"});
    state.selected = 0;

    List(state)
        .highlight_symbol("> ")
        .draw(frame, frame.area());

    // Assert on exact row content
    assert(frame.to_plain_text_row(0) == "> alpha");
    assert(frame.to_plain_text_row(1) == "  beta");
}
```

Run the included automated headless test suite:
```bash
./build/test6_headless_test
```

---

## 📂 Included Showcase Programs

The repository comes with 6 standalone executables demonstrating each layer of the library:

| Executable | Description |
|---|---|
| **`test1_tic_tac_toe`** | Fully playable 2-player game with Vim/Arrow navigation and win detection. |
| **`test2_key_visualizer`** | Live keystroke inspector reporting exact modifier flags, F1–F12, and raw hex codepoints. |
| **`test3_vbox`** | Demonstrates declarative flex and percentage layout composition (`VBox` / `HBox`). |
| **`test4_text_input`** | 3-layer input customization: Pre-hook interceptors, validation filters, and modal Vim editing with `Tab` / `Shift+Tab` focus cycling. |
| **`test5_yazi`** | A 3-column Miller column file explorer inspired by Yazi with live file previews via `TextArea` and wrapped navigation. |
| **`test6_headless_test`** | Automated headless snapshot test suite with 13 tests and zero terminal dependencies. |
| **`test7_dashboard`** | Real-time 60 FPS system telemetry monitor demonstrating Gauge with fractional sub-blocks, rolling Sparklines, titled Rules, and Color lerping. |
| **`test8_text_cjk_emoji`** | Comprehensive UTF-8 test suite asserting CJK ideographs, 2-column emojis, continuation cells, overwrite protection, and wide text alignment. |
| **`test9_chess_board`** | Fully playable 8x8 interactive Chess board demonstrating `Layout::grid_uniform`, `Frame::fill`, alternating wood squares, and Unicode chess piece rendering. |
| **`test10_cjk_terminal`** | Interactive live terminal inspector for Japanese (Kanji/Kana), Chinese, Korean (Hangul), 2-column Emojis, vertical alignment ruler, and live UTF-8 input with `Tabs`. |
| **`test11_text_area`** | Multi-line code editor with line numbers gutter, Sticky/Clamp cursor navigation, Margin/Clamp viewport scrolling, and real-time status telemetry. |
| **`test12_tabs`** | Multi-tab container orchestrating `TextArea`, `List`, `Gauge`, `Sparkline`, and `Grid` chess board with live tab style switching. |

Run any demo directly from `./build/`:
```bash
./build/test12_tabs
```

---

## 🎨 Core Widgets & Components

### `TextArea` & `TextAreaState`
Multi-line text editor featuring dynamic line numbers gutter, UTF-8 byte boundary alignment, and flexible navigation/scrolling modes:
* **`CursorColumnMode`**: `Sticky` (preserves your goal column when navigating across shorter lines) vs. `Clamp`.
* **`ScrollMode`**: `Margin` (maintains context padding around the cursor) vs. `Clamp`.
* **Line Operations**: Splitting (`newline`), backward boundary joining (`backspace`), forward pulling (`delete_forward`).
```cpp
TextAreaState editor("int main() {\n    return 0;\n}");
editor.cursor_mode = CursorColumnMode::Sticky;
editor.scroll_mode = ScrollMode::Margin;

TextArea(editor)
    .border_style(BorderStyle::Rounded)
    .title(" editor.cpp ")
    .line_numbers(true)
    .line_number_fg(Color::DarkGray)
    .draw(frame, area);
```

### `Tabs` & `TabsState`
Decoupled, lightweight horizontal tab bar for organizing multi-screen interfaces:
* **Styles**: `TabsStyle::Bar` (` Home │ Logs │ Settings `), `TabsStyle::Pill` (` [ Home ]  [ Logs ] `), and `TabsStyle::Underline` (`━` active underline).
* **Navigation**: Circular wrapping (`select_next()`, `select_prev()`), direct number key selection (`1`..`9`), and optional `on_change` reactive callbacks.
```cpp
TabsState tabs({"Editor", "Files", "Telemetry"});

Tabs(tabs)
    .style(TabsStyle::Underline)
    .active_bg(Color::Emerald)
    .inactive_fg(Color::Gray)
    .draw(frame, tab_area);
```

### `Box`
Supports 5 distinct border styles (`Rounded`, `Single`, `Double`, `Thick`, `None`), selective side borders, background fills, and 3-way title alignment (`Left`, `Center`, `Right`).
```cpp
Box()
    .border_style(BorderStyle::Rounded)
    .title("Processes")
    .title_align(TitleAlign::Left)
    .fg(Color::Cyan)
    .bg(Color::Rgb(17, 17, 17))
    .filled(true)
    .draw(frame, area);
```

### `List` & `ListState`
Full-width row background highlighting, custom selection indicators (`.highlight_symbol("> ")`), wrap vs. clamp boundary modes, and `on_change` reactive callbacks.
```cpp
ListState files({"main.cpp", "CMakeLists.txt", "README.md"});
files.navigation = ListNavigation::Wrap;

List(files)
    .border_style(BorderStyle::Rounded)
    .highlight_symbol("> ")
    .highlight_fg(Color::Emerald)
    .highlight_bg(Color::Rgb(0, 51, 34))
    .overflow(TextOverflow::Ellipsis)
    .draw(frame, area);
```

### `TextInput` & `TextInputState`
UTF-8 safe cursor navigation, automatic horizontal scrolling window, hardware cursor synchronization, and three layers of customization:
1. **Pre-Hook Interceptor (`.on_key`)**: Custom hotkeys (e.g. Enter to submit, Esc to clear).
2. **Filter Predicate (`.set_filter`)**: Dynamic character filtering (e.g. numeric only, max length).
3. **Low-Level Primitives**: Direct buffer manipulation for modal state machines (Vim normal vs. insert mode).
```cpp
TextInputState input("Initial text");
TextInput(input)
    .placeholder("Search...")
    .border_style(BorderStyle::Rounded)
    .focused_border_fg(Color::Emerald)
    .draw(frame, area);
```

### `Gauge` (Progress Bar)
Supports smooth sub-cell fractional Unicode resolution (1/8th cell increments: `▏▎▍▌▋▊▉█`), centered labels, custom fill/empty characters, and foreground/background colors.
```cpp
Gauge()
    .ratio(0.75f)
    .label(" 75% Complete ")
    .filled_fg(Color::Emerald)
    .empty_fg(Color::DarkGray)
    .draw(frame, area);
```

### `Sparkline` (Data Graph)
Renders compact time-series trends using 8-level Unicode bar heights (`  ▂ ▃ ▄ ▅ ▆ ▇ █`).
```cpp
std::vector<uint64_t> cpu = {10, 25, 40, 65, 80, 55, 30};
Sparkline()
    .data(cpu)
    .max(100)
    .fg(Color::Cyan)
    .draw(frame, area);
```

### `Rule` (Dividers & Section Separators)
Draws horizontal separator lines with optional embedded titles and alignment.
```cpp
Rule("System Logs")
    .style(BorderStyle::Double)
    .title_align(TitleAlign::Center)
    .fg(Color::DarkGray)
    .title_fg(Color::Yellow)
    .draw(frame, area);
```

### `Color` & Mathematical Gradients
Predefined palette constants, 24-bit RGB constructors, and linear interpolation (`lerp`):
```cpp
// 24-bit RGB
uint32_t c = Color::rgb(0, 229, 255);

// Linear gradient: green -> yellow -> red
uint32_t temp_color = Color::lerp(Color::Green, Color::Red, load_ratio);
```

### `Grid` (2D Coordinate Layout Engine)
Effortlessly generate 2D coordinate layouts for board games (Tic-Tac-Toe, Chess, Checkers) or responsive data matrices without manual nested loops:
```cpp
// 8x8 Chess Board in 1 line
auto board = Layout::grid_uniform(area, 8, 8);

board.for_each([&](size_t r, size_t c, const Area& square) {
    bool dark = ((r + c) % 2 == 1);
    uint32_t bg = dark ? Color::Rgb(181, 136, 99) : Color::Rgb(240, 217, 181);
    frame.fill(square, U' ', Color::Default, bg);
    // Draw piece in square!
});
```

---

## 🗺️ Project Roadmap

- [x] **Phase 1: Metal Core & Rendering**
  - [x] Double-buffered differential rendering with ANSI SGR caching
  - [x] Universal mathematical keyboard parser (ANSI, CSI, SS3, CSI u)
  - [x] Headless virtual `Frame` for automated snapshot tests
  - [x] Signal handling and alternate screen RAII lifecycle
- [x] **Phase 2: Production-Ready Layer 2 Widgets**
  - [x] `Box` with 3-way title alignment
  - [x] `Text` with hard newline preservation, tab stops, and multi-line clipping
  - [x] `List` with full-width row highlighting and configurable `TextOverflow`
  - [x] `TextInput` with sliding window scrolling and cursor sync
- [ ] **Phase 3: High-Level UI Runtime & Composition**
  - [ ] `FocusManager` with automatic `Tab` / `Shift+Tab` widget cycling
  - [ ] Unified `Component` base contract
  - [ ] Declarative layout container trees (`VStack`, `HStack` with `{}` syntax)
- [ ] **Phase 4: Advanced Components**
  - [ ] Table widget with column sizing
  - [ ] Modal dialogs and overlay popups
  - [ ] Mouse event handling (clicks, drags, scroll wheel)

---

## 📜 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
