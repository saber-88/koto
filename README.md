# koto (琴)

[![C++23](https://img.shields.io/badge/C%2B%2B-23-blue.svg?style=flat&logo=c%2B%2B)](https://en.cppreference.com/w/cpp/23)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-orange.svg?logo=linux)](https://kernel.org)
[![Headless Tests](https://img.shields.io/badge/Headless%20Tests-23%20Passing-brightgreen.svg)]()
[![CJK & Unicode](https://img.shields.io/badge/Unicode-7%20Passing-brightgreen.svg)]()
[![Zero Warnings](https://img.shields.io/badge/Compiler%20Warnings-Zero-brightgreen.svg)]()

> **koto** (琴) is a modern, dependency-free, high-performance C++23 Terminal User Interface (TUI) engine engineered for deterministic state management (`UI = f(state)`), differential double-buffered rendering, tear-free atomic GPU frame presentation, universal keyboard escape decoding, hardware-like $O(1)$ hit-buffer spatial mouse interaction, and CI/CD automated headless snapshot testing.

---

## 🌟 Why koto?

Traditional terminal UI development in C and C++ has long been burdened by low-level, stateful C APIs (`ncurses`), callback spaghetti, fragile escape sequence parsing, terminal teardown corruption, screen tearing, input lag, and an inability to execute headless unit tests in CI/CD pipelines.

**koto** is written from scratch in modern **C++23** to solve these architectural pain points:

- **Zero External Dependencies**: Pure standard C++23 library and direct POSIX system calls. No `ncurses`, no `terminfo`, no dynamic link requirements.
- **DEC Mode 2026 Synchronized Updates**: Emits private CSI sequences (`\033[?2026h` / `\033[?2026l`) to halt terminal rasterization during buffer writes, enabling atomic, tear-free, flicker-free single-flip frame presentation in GPU-accelerated terminals.
- **Differential Double-Buffered Renderer**: Compares front and back cell buffers per frame. Only modified cells emit ANSI SGR sequences, batched into a single formatted `write()` syscall with SGR color state compression.
- **Zero-Latency Event Loop & Burst Draining**: Drains pending input bursts with `ioctl(FIONREAD)` to completely eliminate escape sequence splitting, mouse/key inversions, and keyboard lag.
- **First-Class Focus Engine (`FocusManager`)**: Combines 1D tab rings (`Tab` / `Shift+Tab`) and auto-harvested 2D spatial navigation (`Arrow` keys) across screen geometry with C++20 concept constraints (`FocusableState`).
- **Hardware-like Frame Hit-Buffer**: Tags back-buffer cells with 32-bit universal compile-time `Id` tokens for $O(1)$ spatial mouse hit-testing, direct row/tab clicking, and mouse wheel scrolling.
- **Modal Scoping & Focus Trapping (`IdScope`)**: RAII-guarded modal scopes that automatically shield background clicks, trap keyboard navigation within active dialogs, and restore previous focus on dismissal.
- **Universal Keyboard Engine & Kitty Protocol (`CSI u`)**: Automatically negotiates Kitty keyboard enhancement flag 1 (`\033[>1u` / `\033[<u`) to disambiguate `Tab` vs `Ctrl+I`, `Enter` vs `Ctrl+M`, `Escape`, modifier bitmasks (formula: `M = 1 + flags`, including Meta / Super / Windows keys), and functional keycodes across Kitty, Ghostty, Alacritty, WezTerm, Foot, GNOME Terminal, Konsole, xterm, tmux, and SSH.
- **Bracketed Paste Mode (`\033[?2004h`)**: Captures clipboard streams inside dedicated `PasteEvent`s, protecting against runaway auto-indentation ("staircase effect"), frame render storms, and malicious terminal command injection.
- **Deterministic State Architecture (`UI = f(state)`)**: Decouples business logic and navigation (`TextAreaState`, `ListState`, `TextInputState`, `TabsState`) from pure, immutable views (`TextArea`, `List`, `TextInput`, `Tabs`).
- **Native C++23 Language Features**: Multidimensional 2D subscripting (`frame[r, c]`, `grid[r, c]`), exception-free `std::expected` error handling (`try_parse_color`), `std::views::enumerate` range pipelines, and defaulted comparison operators (`Area`, `Id`).
- **Headless Snapshot Testing by Default**: Create an in-memory `Frame(rows, cols)` without a live TTY or virtual framebuffer, draw any widget, and assert on exact row text with `frame.to_plain_text_row(r)`.
- **Wide Unicode, CJK & Emoji Engine**: Full UAX-11 East Asian Width calculation, 2-column continuation cells, emoji rendering, and safe overwrite clipping.
- **Zero Warnings**: Compiles cleanly under strict compiler flags: `-Wall -Werror -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wsign-compare -Wshadow -Wold-style-cast`.

---

## 🏛️ Layered Architecture

```
+---------------------------------------------------------------------------------+
| Layer 3: Declarative Application Runtime & Focus Engine (koto::App, FocusManager) |
| - RAII terminal lifecycle, 60 FPS tick, signalfd SIGWINCH resize handling       |
| - FocusManager: 1D Tab Ring, 2D Spatial Arrow Jumps, Pre-dispatch Interceptors  |
| - IdScope: RAII Modal Scoping, Focus Trapping & Background Shielding            |
| - Event dispatch: on_key, on_mouse, on_paste, on_tick, on_render                |
+---------------------------------------------------------------------------------+
| Layer 2: State-Driven Widgets & Layout Engine                                   |
| - TextArea: Gutter line numbers, Sticky/Clamp cursor, Margin/Clamp scroll       |
| - Tabs: Bar, Pill, Underline styles, sub-item mouse hit detection & wheel cycle |
| - List: Full-width row highlights, Wrap/Clamp navigation, sub-item row clicking |
| - TextInput: UTF-8 sliding window scroll, pre-hook filters, cursor sync         |
| - Box: Rounded, Single, Double, Thick borders with 3-way title alignment        |
| - Gauge: Unicode fractional sub-cell resolution (1/8th increments: ▏▎▍▌▋▊▉█)     |
| - Sparkline: 8-level Unicode time-series data trend graphs (  ▂ ▃ ▄ ▅ ▆ ▇ █)     |
| - Rule: Horizontal dividers with embedded titles and border styles              |
| - Layout: Flexbox horizontal & vertical constraints (Fixed, Percentage, Fill)   |
| - Grid: 2D coordinate matrix generator for dashboards & board games             |
+---------------------------------------------------------------------------------+
| Layer 1: The Metal Core                                                         |
| - Terminal: RAII raw mode, alternate screen buffer, bracketed paste negotiation |
| - Frame: Double-buffered cell grid, DEC Mode 2026 atomic synchronized updates   |
| - Hit-Buffer: 32-bit spatial ID raster map for O(1) mouse hit-testing           |
| - EventLoop: POSIX signalfd SIGWINCH, ioctl FIONREAD burst draining, SGR 1006   |
| - Cell & Color: 24-bit TrueColor RGB, 256-color palette, linear lerp gradients |
| - UTF-8: Codepoint widths, multi-byte decoding, wide CJK & 2-column emojis      |
+---------------------------------------------------------------------------------+
```

---

## 🚀 Getting Started

### Prerequisites
- **Compiler**: Clang 16+ or GCC 13+ with **C++23** support
- **Build System**: CMake 3.10+
- **Platform**: Linux / POSIX compliant

### Build & Run
```bash
git clone https://github.com/saber-88/koto.git
cd koto
cmake -B build -S .
cmake --build build

# Run the primary showcase application
./build/kotoapp
```

### Adding koto to Your CMake Project
```cmake
add_subdirectory(koto)
target_link_libraries(my_tui_app PRIVATE koto)
```

---

## 💻 Minimal Application Example

```cpp
#include <koto/koto.hpp>

using namespace koto;

int main()
{
    App app;

    ListState todo({"Refactor metal core", "Implement DEC Mode 2026", "Add Kitty CSI u"});
    todo.navigation = ListNavigation::Wrap;

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
        }
        todo.handle_event(key); // Automatically navigates with Up/Down arrows and j/k
    });

    app.on_render([&](Frame& frame) {
        // Divide frame into 3 vertical regions
        auto sections = Layout::vertical(frame.area(), {
            Constraint::fixed(3), // Header box
            Constraint::fill(),   // List viewport
            Constraint::fixed(1), // Footer status bar
        });

        Box()
            .border_style(BorderStyle::Rounded)
            .title(" koto Task Manager ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .draw(frame, sections[0]);

        List(todo)
            .border_style(BorderStyle::Single)
            .highlight_fg(Color::Emerald)
            .highlight_bg(Color::Rgb(0, 40, 30))
            .highlight_symbol("▶ ")
            .draw(frame, sections[1]);

        Text("Navigate: [j/k or ↑/↓] | Quit: [Ctrl+Q]")
            .fg(Color::DarkGray)
            .draw(frame, sections[2]);
    });

    app.run();
    return 0;
}
```

---

## 🎨 Complete Widget & Component Guide

### 1. `TextArea` & `TextAreaState`
Multi-line text editor designed for code editing, notes, and log inspection.
* **Line Number Gutter**: Configurable line numbers with customizable gutter colors.
* **Cursor Modes**:
  * `CursorColumnMode::Sticky`: Preserves the intended column when navigating across shorter lines.
  * `CursorColumnMode::Clamp`: Clamps cursor to line length.
* **Scroll Modes**:
  * `ScrollMode::Margin`: Maintains context padding rows/columns around the cursor.
  * `ScrollMode::Clamp`: Scrolls only when reaching the viewport boundary.
* **Granular Mutations**: `insert(cp)`, `insert(std::string_view)`, `newline()`, `backspace()`, `delete_forward()`, `move_left()`, `move_right()`, `move_up()`, `move_down()`.

```
╭─ editor.cpp ────────────────────────────────╮
│ 1 │ #include <iostream>                     │
│ 2 │ int main() {                            │
│ 3 │     std::cout << "Hello, koto!\n";      │
│ 4 │     return 0;                           │
│ 5 │ }                                       │
╰─────────────────────────────────────────────╯
```

```cpp
TextAreaState editor(
    "#include <iostream>\n"
    "int main() {\n"
    "    std::cout << \"Hello, koto!\\n\";\n"
    "    return 0;\n"
    "}"
);
editor.cursor_mode = CursorColumnMode::Sticky;
editor.scroll_mode = ScrollMode::Margin;

TextArea(editor)
    .border_style(BorderStyle::Rounded)
    .title(" editor.cpp ")
    .title_align(TitleAlign::Left)
    .line_numbers(true)
    .line_number_fg(Color::DarkGray)
    .focused(true)
    .draw(frame, area);
```

---

### 2. `Tabs` & `TabsState`
Decoupled horizontal navigation bar for organizing multi-pane interfaces.
* **3 Visual Styles**:
  * `TabsStyle::Bar`: ` Files │ Editor │ Logs │ Telemetry `
  * `TabsStyle::Pill`: ` [ Files ]  [ Editor ]  [ Logs ] `
  * `TabsStyle::Underline`: Renders text on row 0 and a thick accent underline (`━`) under the active tab on row 1 (`─` under inactive).
* **Navigation**: Circular wrapping (`select_next()`, `select_prev()`), number shortcuts (`1`..`9`), and reactive `on_change` callbacks.

```
 Files  Editor  Logs  Telemetry
━━━━━━━─────────────────────────
```

```cpp
TabsState tabs({"Files", "Editor", "Logs", "Telemetry"});
tabs.set_selected(1);

Tabs(tabs)
    .style(TabsStyle::Underline)
    .active_fg(Color::Cyan)
    .inactive_fg(Color::DarkGray)
    .active_bg(Color::Default)
    .draw(frame, area);
```

---

### 3. `List` & `ListState`
Selection list with full-width row backgrounds, prefix markers, and text clipping.
* **Navigation**: `ListNavigation::Wrap` (loops around ends) vs. `ListNavigation::Clamp`.
* **Overflow**: `TextOverflow::Ellipsis` (`filename_too_long…`) or `TextOverflow::Clip`.

```
╭─ Files ─────────────────────────────────────╮
│ ▶ src/main.cpp                              │
│   src/frame.cpp                             │
│   include/koto/koto.hpp                     │
╰─────────────────────────────────────────────╯
```

```cpp
ListState files({"src/main.cpp", "src/frame.cpp", "include/koto/koto.hpp"});
files.navigation = ListNavigation::Wrap;

List(files)
    .border_style(BorderStyle::Rounded)
    .title(" Files ")
    .highlight_symbol("▶ ")
    .highlight_fg(Color::Cyan)
    .highlight_bg(Color::Rgb(20, 30, 45))
    .overflow(TextOverflow::Ellipsis)
    .draw(frame, area);
```

---

### 4. `TextInput` & `TextInputState`
Single-line editable field with UTF-8 byte boundary alignment, horizontal scrolling window, and hardware cursor synchronization.
* **Custom Interceptor**: Intercept keys (e.g. Enter to submit, Esc to blur).
* **Validation Filter**: Restrict inputs (e.g. numeric only, max length).

```
╭─ Search (Press [Enter] to submit) ──────────╮
│ query: modern tui framework█                │
╰─────────────────────────────────────────────╯
```

```cpp
TextInputState input;

// Intercept Enter key to trigger a search action
input.on_key([](TextInputState& s, const KeyEvent& k) {
    if (k.is(Key::Enter)) {
        // Execute search on s.text()
        return true; // Mark event consumed
    }
    return false;
});

// Restrict to max 64 characters
input.set_filter([](std::string_view current, char32_t /*ch*/) {
    return current.size() < 64;
});

TextInput(input)
    .title(" Search ")
    .placeholder("Type here...")
    .border_style(BorderStyle::Rounded)
    .focused_border_fg(Color::Emerald)
    .draw(frame, area);
```

---

### 5. `Gauge` (Progress Bar)
Renders smooth progress with **fractional sub-cell Unicode resolution** (1/8th cell increments: `▏▎▍▌▋▊▉█`), centered labels, and linear color interpolation.

```
████████████████████████████▌              68%
```

```cpp
float progress = 0.68f;

Gauge()
    .ratio(progress)
    .label(" 68% ")
    .filled_fg(Color::lerp(Color::Cyan, Color::Emerald, progress))
    .empty_fg(Color::DarkGray)
    .draw(frame, area);
```

---

### 6. `Sparkline` (Time-Series Graph)
Renders compact data trends using 8-level Unicode bar heights (`  ▂ ▃ ▄ ▅ ▆ ▇ █`).

```
 ▂▃▅▆▇█▇▆▅▄▃▂ 
```

```cpp
std::vector<uint64_t> metrics = {5, 12, 28, 45, 67, 89, 100, 92, 70, 54, 32, 18, 8};

Sparkline()
    .data(metrics)
    .max(100)
    .fg(Color::Cyan)
    .draw(frame, area);
```

---

### 7. `Rule` (Dividers & Separators)
Horizontal section dividers with embedded titles and border styles.

```
─────── System Metrics ───────────────────────
```

```cpp
Rule(" System Metrics ")
    .style(BorderStyle::Single)
    .title_align(TitleAlign::Left)
    .fg(Color::DarkGray)
    .title_fg(Color::Yellow)
    .draw(frame, area);
```

---

### 8. `Box`
Container frame supporting 5 border styles (`Rounded`, `Single`, `Double`, `Thick`, `None`), 3-way title alignment (`Left`, `Center`, `Right`), and background color fills.

```cpp
Box()
    .border_style(BorderStyle::Double)
    .title(" Diagnostics ")
    .title_align(TitleAlign::Center)
    .fg(Color::Magenta)
    .bg(Color::Rgb(15, 15, 20))
    .filled(true)
    .draw(frame, area);
```

---

### 9. `Text`
Text widget supporting hard newlines (`\n`), soft boundary wrapping, 4-space tab stop expansion, and multi-line alignment (`Left`, `Center`, `Right`).

```cpp
Text("Line 1: System Online\nLine 2: All Services Green")
    .align(TextAlign::Center)
    .fg(Color::Green)
    .draw(frame, area);
```

---

### 10. `Grid` (2D Coordinate Layout Engine)
Generate 2D coordinate matrices for board games (Chess, Checkers, Tic-Tac-Toe), spreadsheets, or responsive dashboard cards without nested loops:

```cpp
// Generate an 8x8 Chess Board in 1 line
auto board = Layout::grid_uniform(frame.area(), 8, 8);

board.for_each([&](size_t row, size_t col, const Area& square) {
    bool dark = ((row + col) % 2 == 1);
    uint32_t bg = dark ? Color::Rgb(181, 136, 99) : Color::Rgb(240, 217, 181);
    frame.fill(square, U' ', Color::Default, bg);
    // Render chess piece into square!
});
```

---

### 11. `FocusManager` & `IdScope` (Spatial Navigation, Mouse Hit-Testing & Modal Trapping)
The `FocusManager` orchestrates bi-directional keyboard navigation and mouse hit-testing across arbitrary widgets with zero heap allocations in the hot draw path:

* **1D Tab Ring**: Cycles forward with `Tab` and backward with `Shift+Tab`.
* **2D Spatial Arrow Navigation**: Computes Euclidean distance and directional angle cones between bounding box centroids, enabling natural `Up`, `Down`, `Left`, and `Right` arrow navigation across multi-column grids or disjoint cards.
* **C++20 Concept Constraint (`FocusableState`)**: Automatically accepts any widget state object exposing `handle_key(const KeyEvent&) -> bool` (`TextInputState`, `TextAreaState`, `ListState`, `TabsState`).
* **Pre-Dispatch Interceptor Hook**: `fm.interceptor([](const KeyEvent& k, FocusTarget* active) -> bool)` allows application developers to intercept keys (e.g. global shortcuts, modal escape, Vim modes) before focus routing.
* **Modal Focus Trapping & Background Shielding (`IdScope`)**: RAII modal scopes (`push_scope(id, ScopeMode::Modal)`) automatically trap keyboard focus within the dialog, shield background elements from accidental mouse clicks, and restore focus to the opening widget on dismissal.
* **$O(1)$ SGR Mouse Hit-Testing & Sub-Item Interaction**: Left-clicking any widget instantly focuses it. For complex widgets, mouse coordinates are resolved to sub-items:
  * **`Tabs`**: Direct left-click selects tab headers; mouse wheel scrolls and cycles active tabs.
  * **`List`**: Direct left-click activates rows; mouse wheel scrolls through items.
  * **`TextArea`**: Direct left-click positions the editing cursor; mouse wheel scrolls the viewport.
  * **Buttons**: Clicking any target with an `on_key` callback automatically synthesizes a `Key::Enter` event, firing its action handler with zero boilerplate.

```cpp
FocusManager fm;
TextInputState username;
TextInputState password;
bool submit_clicked = false;

// 1. Declaratively register interactive targets
fm.register_target(ID("user"), username, user_area);
fm.register_target(ID("pass"), password, pass_area);
fm.register_target(ID("submit"), submit_area, [&](const KeyEvent& k) {
    if (k.is(Key::Enter)) {
        submit_clicked = true;
        return true;
    }
    return false;
});

// 2. Attach focus manager to the application runtime
app.use_focus(fm);

// 3. Render widgets with automatic visual focus feedback
Box().title(" Username ")
     .focused(fm.is_focused(ID("user")))
     .draw(frame, user_area);
TextInput(username).draw(frame, user_area);
```

---

## ⚡ Metal Core & Terminal Protocols

### DEC Mode 2026 (Synchronized Updates)
Screen tearing in terminal emulators happens when an application writes frame bytes in multiple OS chunks while the emulator's GPU rasterizer renders a partial frame.

`koto` eliminates tearing by bracketing every `Frame::render()` pass with DEC Mode 2026:
- **Begin**: `\033[?2026h`
- **Emit dirty cells**
- **End**: `\033[?2026l`

Supporting emulators (Ghostty, Kitty, WezTerm, Alacritty, iTerm2, modern Windows Terminal) hold rasterization until the end marker arrives, presenting the frame atomically in a single GPU flip.

---

### Kitty Keyboard Protocol (`CSI u`) & Disambiguation
In legacy terminals, `Ctrl+I` is byte-for-byte identical to `Tab` (`0x09`), `Ctrl+M` is identical to `Enter` (`0x0D`), and `Escape` (`0x1B`) is ambiguous with the start of an escape sequence.

On startup, `koto` automatically negotiates **Kitty progressive enhancement flag 1** (`\033[>1u`) and pops it on exit (`\033[<u`):
- `Shift + Enter` → `\033[13;2u`
- `Ctrl + Enter` → `\033[13;5u`
- `Ctrl + Tab` → `\033[9;5u` (distinct from `Ctrl+I`!)
- `Shift + Tab` → `\033[9;2u` (satisfies both `key.is(Key::Tab)` with `shift` and `key.is(Key::BackTab)`)
- `Ctrl + Backspace` → `\033[127;5u`
- `Meta / Super + Key` → Decodes modifier bitmask 8 / 32 (`key.meta == true`)
- Functional PUA keys: Keypad Enter (`57414`), Keypad Arrows (`57417`–`57420`), Delete (`57426`)
- Automatically ignores key release events (`:3`) to prevent double-firing keystrokes.

---

### Bracketed Paste Mode (`\033[?2004h`)
When a user pastes text, the terminal emulator wraps the payload in `\033[200~` ... `\033[201~`.

`koto` parses this into an atomic `PasteEvent`:
1. **Prevents Runaway Auto-Indentation ("Staircase Effect")**: Text editors know the text is pasted, temporarily disabling auto-indent.
2. **Atomic Single-Step Undo**: Pasting 500 characters creates 1 history entry, undone with a single `Ctrl+Z`.
3. **No 60 FPS Render Storms**: 1 paste operation triggers 1 layout and 1 render pass, rather than 500 separate redraw cycles.
4. **Security**: Pasted newlines are treated as literal text, never accidentally executing shell commands.

```cpp
app.on_paste([&](const std::string& text) {
    if (editor_focused) {
        editor.insert(text); // Atomic multi-line paste
    }
});
```
*Note: If an app omits `on_paste`, `App::run()` automatically falls back to feeding characters into `on_key`, so simple apps never drop pasted text.*

---

### SGR 1006 Extended Mouse Protocol & Frame Hit-Buffer
Modern terminal emulators communicate mouse interaction (presses, releases, drags, and wheel turns) through the SGR 1006 extended mouse protocol (`\033[?<mode>h`):
- `\033[?1000h` / `\033[?1002h`: Enable normal click and button-motion mouse tracking.
- `\033[?1006h`: Enable SGR extended coordinate reporting (`<btn>;<col>;<row>M` / `m`), breaking free of the legacy 223-column ANSI byte limit.

`koto` pairs this with a hardware-like **32-bit universal Frame Hit-Buffer**:
- Every cell in the back buffer stores a 32-bit `Id` tag (defaulting to `-1`).
- Widgets tag rendered screen cells via `frame.set_hit_id(r, c, id)`.
- When an `SGR 1006` mouse event arrives at `(row, col)`, `frame.hit_test(row, col)` performs an instantaneous $O(1)$ lookup, mapping physical screen pixels directly to the target widget.
- **Sub-Item Coordinates**: Complex widgets calculate internal hits (`TabsState::tab_at(col)`, `ListState::row_at(row)`, `TextAreaState::coord_at(row, col)`).
- **Automatic Button Synthesis**: Clicking any target registered with an `on_key` callback automatically synthesizes a `Key::Enter` event to trigger its action handler with zero boilerplate.

---

### Zero-Latency Event Loop & `ioctl(FIONREAD)` Burst Draining
Rapid mouse drags or wheel bursts can emit dozens of escape sequences per millisecond. Traditional TUI loops that read byte-by-byte or fixed chunks risk splitting escape sequences across reads or starving the keyboard.

`koto` eliminates this entirely:
- Queries pending bytes via `ioctl(FIONREAD)` on the TTY file descriptor.
- Batches burst reads into an internal stream buffer.
- Complete escape sequences are parsed atomically before dispatch.
- Completely prevents mouse/keyboard inversions, cursor lag, or torn escape packets.

---

## 🧪 Headless Snapshot Testing (CI/CD Ready)

`koto` was engineered from day one for test-driven development. You can construct an in-memory `Frame(rows, cols)` without attaching to a terminal, execute layout and widget draws, and assert on exact characters:

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

void test_box_and_list()
{
    // Allocate an in-memory virtual screen (5 rows x 20 columns)
    Frame frame(5, 20);

    ListState state({"alpha", "beta"});
    state.selected = 0;

    List(state)
        .border_style(BorderStyle::Rounded)
        .title("Items")
        .title_align(TitleAlign::Left)
        .highlight_symbol("> ")
        .draw(frame, frame.area());

    // Assert exact characters on any row
    assert(frame.to_plain_text_row(0) == "╭─ Items ──────────╮");
    assert(frame.to_plain_text_row(1) == "│ > alpha          │");
    assert(frame.to_plain_text_row(2) == "│   beta           │");
    assert(frame.to_plain_text_row(4) == "╰──────────────────╯");
}
```

Run the built-in test suites:
```bash
# 23 comprehensive snapshot tests (Box, List, Layout, Gauge, Sparkline, Grid, TextArea, Tabs, Kitty CSI u, Paste, C++23 native features, Hit-Buffer, FocusManager, Spatial 2D, Modal Scoping, Sub-item clicks)
./build/test6_headless_test

# 7 UTF-8 / CJK / Emoji width & continuation cell tests
./build/test8_text_cjk_emoji
```

---

## 📂 Included Showcase Applications

The repository includes **15 standalone showcase programs** demonstrating every feature of the library:

| Executable | Description |
|---|---|
| **`kotoapp`** | Primary application showcase featuring interactive layout splitting, List, TextArea, and Gauge. |
| **`test1_tic_tac_toe`** | Fully playable 2-player game with Vim/Arrow navigation and automatic win detection. |
| **`test2_key_visualizer`** | Live keystroke inspector reporting exact modifier flags (Ctrl, Alt, Shift, Meta), Kitty CSI u codes, and raw hex codepoints. |
| **`test3_vbox`** | Declarative flex and percentage layout composition (`VBox` / `HBox`). |
| **`test4_text_input`** | 3-layer input customization: Pre-hook interceptors, validation filters, and modal Vim editing with `Tab` / `Shift+Tab` focus cycling. |
| **`test5_yazi`** | A 3-column Miller column file explorer inspired by Yazi with live file previews via `TextArea`. |
| **`test6_headless_test`** | Automated headless snapshot test suite with 23 tests and zero terminal dependencies. |
| **`test7_dashboard`** | Real-time 60 FPS system telemetry monitor demonstrating Gauge with fractional sub-blocks, rolling Sparklines, titled Rules, and Color lerping. |
| **`test8_text_cjk_emoji`** | Comprehensive UTF-8 test suite asserting CJK ideographs, 2-column emojis, continuation cells, overwrite protection, and wide text alignment. |
| **`test9_chess_board`** | Fully playable 8x8 interactive Chess board demonstrating `Layout::grid_uniform`, `Frame::fill`, alternating wood squares, and Unicode chess piece rendering. |
| **`test10_cjk_terminal`** | Interactive live terminal inspector for Japanese (Kanji/Kana), Chinese, Korean (Hangul), 2-column Emojis, vertical alignment ruler, and live UTF-8 input with `Tabs`. |
| **`test11_text_area`** | Multi-line code editor with line numbers gutter, Sticky/Clamp cursor navigation, Margin/Clamp viewport scrolling, and real-time status telemetry. |
| **`test12_tabs`** | Multi-tab container orchestrating `TextArea`, `List`, `Gauge`, `Sparkline`, and `Grid` chess board with live tab style switching. |
| **`test13_focus_form`** | Interactive multi-field user registration form with 1D Tab cycle, `TextInputState`, `ListState`, `TabsState`, button activation, and dynamic visual focus states. |
| **`test14_spatial_2d_nav`** | 2D spatial navigation demo with 3x3 disconnected card grid, Euclidean centroid jumps across Arrow keys (`Up`, `Down`, `Left`, `Right`), and direct mouse click selection. |
| **`test15_modal_focus_trap`** | Modal dialog workflow with confirmation popups, background mouse click shielding, automatic modal focus trapping, Escape dismiss, and focus restoration. |

Run any showcase directly from `./build/`:
```bash
./build/test7_dashboard
./build/test9_chess_board
./build/test12_tabs
./build/test13_focus_form
./build/test14_spatial_2d_nav
./build/test15_modal_focus_trap
```

---

## 🗺️ Project Roadmap

- [x] **Phase 1: Metal Core & Terminal Protocols**
  - [x] Double-buffered differential rendering with ANSI SGR state caching
  - [x] DEC Mode 2026 synchronized updates (tear-free atomic frame presentation)
  - [x] Universal keyboard engine (ANSI, CSI, SS3, and Kitty `CSI u` protocol)
  - [x] Bracketed Paste Mode (`\033[?2004h`) with `PasteEvent` and newline normalization
  - [x] Headless virtual `Frame` for CI/CD automated snapshot testing
  - [x] POSIX `signalfd` SIGWINCH terminal resize handling and RAII alternate screen buffer
- [x] **Phase 2: State-Driven Widgets & Layouts**
  - [x] `Box` with 5 border styles and 3-way title alignment
  - [x] `Text` with hard newline preservation, soft wrapping, and tab stops
  - [x] `List` with full-width row backgrounds, prefix markers, and `TextOverflow`
  - [x] `TextInput` with sliding window scrolling, cursor sync, and pre-hook interceptors
  - [x] `TextArea` multi-line code editor with line numbers gutter, Sticky cursor, and Margin scrolling
  - [x] `Tabs` container with Bar, Pill, and Underline styles
  - [x] `Gauge` with Unicode fractional sub-cell resolution (1/8th increments)
  - [x] `Sparkline` with 8-level Unicode time-series data graphs
  - [x] `Rule` horizontal dividers with embedded titles
  - [x] `Layout::grid` and `Layout::grid_uniform` 2D coordinate matrices
- [x] **Phase 3: Focus Engine, Spatial Navigation & Modal Scoping**
  - [x] `FocusManager` 1D ring focus graph traversal (`Tab` / `Shift+Tab`)
  - [x] Auto-harvested 2D spatial arrow navigation (`Up`, `Down`, `Left`, `Right`) via centroid distance & angle cone
  - [x] Hardware-like 32-bit `Frame` hit-buffer with $O(1)$ spatial coordinate tagging
  - [x] SGR 1006 mouse protocol support (clicks, releases, drags, scroll wheel up/down)
  - [x] Sub-item direct mouse hit detection (`Tabs` headers, `List` rows, `TextArea` lines/cols)
  - [x] `IdScope` RAII modal focus trapping and background click shielding
  - [x] Pre-dispatch key interceptor hook for custom shortcuts and navigation overrides
  - [x] `App::use_focus(fm)` seamless declarative runtime integration
  - [x] `ioctl(FIONREAD)` input burst draining to eliminate input latency and packet splitting
- [ ] **Phase 4: Advanced Data & Tree Components**
  - [ ] `Table` widget with sortable columns, header styles, horizontal scroll, and cell formatters
  - [ ] `Tree` widget for hierarchical directory structures and expandable nodes
  - [ ] Multi-select lists and checkboxes

---

## 📜 License

This project is licensed under the MIT License — see the [LICENSE](LICENSE) file for details.
