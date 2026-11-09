# Getting Started with libkoto

Comprehensive step-by-step guide to installing, configuring, and building modern C++23 terminal user interfaces using `libkoto`.

---

## 1. System Requirements

* **C++ Compiler**: Clang 17+ or GCC 13+ with standard C++23 support (`-std=c++23`).
* **Build System**: CMake 3.25+ (recommended with the Ninja build generator).
* **Operating System**: Linux or macOS (POSIX-compliant terminal subsystem).
* **Terminal Emulator**: Any modern terminal supporting ANSI escape sequences (e.g., Kitty, Ghostty, Alacritty, WezTerm, iTerm2, Windows Terminal, GNOME Terminal).

---

## 2. CMake Integration

### Option A: Using `FetchContent` (Recommended)

Add `libkoto` directly to your `CMakeLists.txt`. CMake will fetch the repository from GitHub:

```cmake
cmake_minimum_required(VERSION 3.25)
project(my_tui_app LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    koto
    GIT_REPOSITORY https://github.com/saber-88/koto.git
    GIT_TAG        main
)
FetchContent_MakeAvailable(koto)

add_executable(my_app src/main.cpp)
target_link_libraries(my_app PRIVATE koto::koto)
```

### Option B: Local Subdirectory (`add_subdirectory`)

If you prefer git submodules or vendoring the repository directly:

```bash
# Add as a git submodule
git submodule add https://github.com/saber-88/koto.git vendor/koto
```

Then in your `CMakeLists.txt`:

```cmake
add_subdirectory(vendor/koto)
target_link_libraries(my_app PRIVATE koto::koto)
```

The exported `koto::koto` CMake target automatically configures include paths (`include/`) and required compiler flags.

---

## 3. Building libkoto from Source

To compile the library, test suites, and interactive showcase examples locally:

```bash
# Clone the repository
git clone https://github.com/saber-88/koto.git
cd koto

# Configure with Ninja build generator
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# Compile all targets
cmake --build build

# Execute all test suites
ctest --test-dir build --output-on-failure
```

---

## 4. Your First Application: Step-by-Step

Create `src/main.cpp` with an interactive counter and dashboard:

```cpp
#include <koto/koto.hpp>
#include <format>
#include <string>

using namespace koto;

int main()
{
    // 1. Initialize the application engine
    App app;

    // 2. Local reactive application state
    int counter = 0;
    std::string status_msg = "Ready";

    // 3. Register keyboard event handlers
    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q') {
            app.quit();
        } else if (key.ch == '+' || key.ch == '=') {
            counter++;
            status_msg = "Incremented";
        } else if (key.ch == '-' || key.ch == '_') {
            counter--;
            status_msg = "Decremented";
        } else if (key.ch == 'r' || key.ch == 'R') {
            counter = 0;
            status_msg = "Reset to zero";
        }
    });

    // 4. Register render hook (invoked per frame)
    app.on_render([&](Frame& frame) {
        // Partition the terminal screen vertically:
        // - Header (3 rows)
        // - Body (Fill remaining height)
        // - Footer (1 row)
        auto [header_area, body_area, footer_area] = Layout::vertical(frame.area(), {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(1)
        });

        // Header with rounded borders
        Box()
            .title("Counter Dashboard")
            .border_style(BorderStyle::Rounded)
            .border_fg(Color::Cyan)
            .title_fg(Color::White)
            .draw(frame, header_area);

        Text("Press '+' to increment, '-' to decrement, 'r' to reset, 'q' to quit.")
            .align(TextAlign::Center)
            .fg(Color::DarkGray)
            .draw(frame, header_area.inset(1, 1, 1, 1));

        // Body partitioned horizontally: Card (Left 50%), Status (Right 50%)
        auto [left_body, right_body] = Layout::horizontal(body_area, {
            Constraint::percent(50),
            Constraint::percent(50)
        });

        // Counter display card
        Box()
            .title("Current Value")
            .border_style(BorderStyle::Single)
            .border_fg(Color::Emerald)
            .draw(frame, left_body);

        std::string counter_text = std::format("Value: {}", counter);
        Text(counter_text)
            .align(TextAlign::Center)
            .fg(counter >= 0 ? Color::Emerald : Color::Red)
            .draw(frame, left_body.inset(2, 2, 2, 2));

        // Status log card
        Box()
            .title("Activity Log")
            .border_style(BorderStyle::Single)
            .border_fg(Color::Yellow)
            .draw(frame, right_body);

        Text(status_msg)
            .align(TextAlign::Center)
            .fg(Color::White)
            .draw(frame, right_body.inset(2, 2, 2, 2));

        // Footer status line
        Text("libkoto v1.0.0 | Press 'q' to exit")
            .align(TextAlign::Left)
            .fg(Color::DarkGray)
            .draw(frame, footer_area);
    });

    // 5. Run the application event loop
    auto result = app.run();
    if (!result) {
        std::fprintf(stderr, "Fatal error: %.*s\n",
                     static_cast<int>(result.error().message().size()),
                     result.error().message().data());
        return 1;
    }

    return 0;
}
```

---

## 5. Next Steps

* Deep-dive into the [Rendering Pipeline](architecture/rendering-pipeline.md) to understand double-buffering and continuation cells.
* Explore the [Event Loop](architecture/event-loop.md) for Kitty keyboard protocol and async self-pipe wakeups.
* Browse the [Widget Catalog](widgets/box.md) for prebuilt interactive components.

---

## C++ Language Features Utilized

1. **`std::format` (C++20)**: Type-safe, high-speed string formatting (`std::format("Value: {}", counter)`).
2. **Structured Bindings (C++17)**: `auto [header, body, footer] = Layout::vertical(...)` unpacks `LayoutResult` into named areas.
3. **`std::expected<void, SystemError>` (C++23)**: Monadic return type for `app.run()`, eliminating unhandled exceptions.
4. **Lambda Capture Closures (`[&]`)**: Clean closures binding local reactive state (`counter`, `status_msg`) without heap allocation.
5. **Fluent Interface Pattern**: Declarative method chaining for all widget constructors.
