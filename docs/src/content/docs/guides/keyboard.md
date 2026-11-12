---
title: Keyboard Input Handling
description: How to match navigation keys, printable characters, modifiers, and FocusManager key interceptors
sidebar:
  label: Keyboard Input
  order: 2
---

## Goal

Handle navigation keys (`Key::Up`, `Key::Enter`, `Key::Escape`, `Key::BackTab`), modifier combinations (`ctrl`, `shift`, `alt`, `meta`), printable Unicode characters, and pre-widget focus interceptors.

<!-- src: include/koto/event.hpp:14-74 -->
<!-- src: src/event.cpp:109-612 -->

## Complete working example

```cpp
#include <koto/koto.hpp>
#include <cstdio>
#include <string>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;
    TextInputState cmd_input;
    ListState log_list({"System ready"});

    fm.add(cmd_input).add(log_list);

    // Pre-hook: intercept Ctrl+L globally even when TextInput is focused
    fm.interceptor([&](FocusManager&, const KeyEvent& key) {
        if (key.ctrl && key.ch == 'L') {
            log_list.set_items({"Logs cleared"});
            return true;
        }
        return false;
    });

    // Submit command when pressing Enter inside cmd_input
    cmd_input.on_key([&](TextInputState& state, const KeyEvent& key) {
        if (key.is(Key::Enter) && !state.value.empty()) {
            log_list.items.push_back(state.value);
            log_list.select_last();
            state.clear();
            return true;
        }
        return false;
    });

    app.use_focus(fm);

    app.on_key([&](const KeyEvent& key) {
        if (key.is(Key::Escape) || (key.ctrl && key.ch == 'Q')) {
            app.quit();
        }
    });

    app.on_render([&](Frame& frame) {
        auto [logs_area, input_area] = Layout::vertical(frame.area(), {
            Constraint::fill(),
            Constraint::fixed(3),
        });

        List(log_list)
            .title("History")
            .border_style(BorderStyle::Single)
            .highlight_symbol("> ")
            .draw(frame, logs_area);

        TextInput(cmd_input)
            .title("Command (Enter to submit, Ctrl+L to clear, Esc to quit)")
            .border_style(BorderStyle::Rounded)
            .draw(frame, input_area);
    });

    if (auto res = app.run(); !res) {
        std::fprintf(stderr, "Error: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return 1;
    }
    return 0;
}
```

## Step-by-step explanation

1. **Matching functional keys with `KeyEvent::is(Key)`**: Call `key.is(Key::Enter)`, `key.is(Key::Escape)`, `key.is(Key::BackTab)`, or `key.is(Key::CtrlRight)`. For plain `Key::Up`, `Key::Down`, `Key::Left`, and `Key::Right`, `key.is(...)` checks `!shift && !ctrl && !alt` (`include/koto/event.hpp:66-68`) so `Shift+Up` never triggers plain `Key::Up`.
2. **Matching `Ctrl` + letter shortcuts**: Both legacy ASCII control characters (`1..26`) and Kitty CSI-u normalize `Ctrl` + letter to uppercase `'A'..'Z'` with `key.ctrl == true` (`src/event.cpp:303-306, 571-576`). Check `key.ctrl && (key.ch == 'Q' || key.ch == 'q')` (or `key.ch == 'Q'`).
3. **Intercepting keys before focused widgets**: When `app.use_focus(fm)` is active, `App::dispatch_event` passes every `KeyEvent` to `fm.handle_key(key)` before `app.on_key` (`src/app.cpp:166-180`). Use `fm.interceptor(...)` for global hotkeys that should run before `Tab` cycling or widget handlers, or use `TextInputState::on_key(...)` for widget-specific overrides like `Key::Enter`.

<!-- src: include/koto/event.hpp:50-74 -->
<!-- src: src/focus.cpp:537-589 -->

## Common mistakes

- **Looking for `Key::Space`**: `Key` has no `Space` enumerator (`include/koto/event.hpp:14-40`). Check `key.ch == ' '` (or `key.is_char()` for general printable input).
- **Expecting `Ctrl+C` to send `SIGINT` in raw mode**: `Terminal::enter_raw_mode()` disables `ISIG` (`src/terminal.cpp:246`), so pressing `Ctrl+C` arrives as `KeyEvent{.ch = 'C', .ctrl = true}` instead of raising `SIGINT`. Handle it in `on_key` or `fm.interceptor` and call `app.quit()`.
- **Using bare letter hotkeys in `app.on_key` alongside a `TextInput` or `TextArea`**: When a text editor state struct is focused, it consumes all unmodified printable characters (`!key.ctrl && !key.alt && key.is_char()`). Use `Key::Escape`, function keys (`Key::F1`..`Key::F12`), or `ctrl`/`alt` combinations to quit or switch modes.

## See also

- [Events and EventLoop](/api/events/)
- [FocusManager](/api/focus-manager/)
- [TextInput](/widgets/text-input/)
