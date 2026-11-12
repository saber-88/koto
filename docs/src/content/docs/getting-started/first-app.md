---
title: First Application
description: Build a complete interactive task list and gauge application with FocusManager, Layout, and error handling
sidebar:
  label: First App
  order: 3
---

## Minimal interactive application

Create `main.cpp` and include `<koto/koto.hpp>`. This program creates an `App`, a `FocusManager`, a `ListState`, and a `TextInputState`, splits the screen with `Layout::vertical` and `Layout::horizontal`, and checks the `std::expected<void, TerminalErrorInfo>` returned by `app.run()`:

```cpp
#include <koto/koto.hpp>
#include <cstdio>
#include <string>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;

    ListState tasks({"Configure CMake", "Write widgets", "Run ctest"}, ListNavigation::Wrap);
    TextInputState new_task;

    new_task.on_key([&](TextInputState& input, const KeyEvent& key) {
        if (key.is(Key::Enter) && !input.value.empty()) {
            tasks.items.push_back(input.value);
            tasks.select_last();
            input.clear();
            return true;
        }
        return false;
    });

    fm.add(tasks).add(new_task);
    app.use_focus(fm);

    app.on_key([&](const KeyEvent& key) {
        if (key.is(Key::Escape) || (key.ctrl && key.ch == 'Q')) {
            app.quit();
        }
    });

    app.on_render([&](Frame& frame) {
        auto [header_area, main_area, input_area, footer_area] = Layout::vertical(frame.area(), {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(3),
            Constraint::fixed(1),
        });

        Box()
            .title("koto Task Manager")
            .border_style(BorderStyle::Rounded)
            .fg(Color::Cyan)
            .draw(frame, header_area);

        List(tasks)
            .title("Tasks")
            .border_style(BorderStyle::Rounded)
            .highlight_symbol("> ")
            .highlight_fg(Color::Emerald)
            .draw(frame, main_area);

        TextInput(new_task)
            .title("New Task (Enter to add)")
            .placeholder("Type a task...")
            .border_style(BorderStyle::Rounded)
            .draw(frame, input_area);

        Text("Tab: switch focus | j/k or Up/Down: select | Esc or Ctrl+Q: quit")
            .fg(Color::DarkGray)
            .draw(frame, footer_area);
    });

    if (auto res = app.run(); !res) {
        std::fprintf(stderr, "Terminal error: %.*s (errno: %d)\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data(),
                     res.error().system_errno);
        return 1;
    }
    return 0;
}
```

<!-- src: include/koto/app.hpp:12-51 -->
<!-- src: include/koto/focus.hpp:139-295 -->

## How it works

1. **State structs live in `main()`**: `tasks` (`ListState`) and `new_task` (`TextInputState`) persist across frames and outlive `fm` (`FocusManager`).
2. **Focus wiring (`app.use_focus(fm)`)**: `fm.add(tasks).add(new_task)` registers both states in the `focus ring`, giving initial focus to `tasks` (`index 0`). Calling `app.use_focus(fm)` enables SGR mouse tracking, syncs widget bounding boxes after every render pass, and forwards `Tab`, `Shift+Tab`, mouse clicks, paste events, and typing to the active target (`src/app.cpp:50-55, 104-174`).
3. **Custom Enter key on `TextInputState`**: `new_task.on_key(...)` intercepts `Key::Enter` when `new_task` is focused, appends `input.value` to `tasks.items`, selects the new item, and clears the input buffer.
4. **Error handling on `app.run()`**: If standard input or output is not a terminal (`TerminalError::NotATty`), `app.run()` returns `std::unexpected(TerminalErrorInfo)` without modifying terminal attributes.

## See also

- [Building and Running](/getting-started/building-running/)
- [App](/api/app/)
- [FocusManager](/api/focus-manager/)
- [Immediate-Mode Concept](/concepts/immediate-mode/)
