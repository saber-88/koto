---
title: Mouse Tracking and Hit-Testing
description: How to capture clicks, wheel scrolling, and drag or hover events using SGR 1006 mouse tracking and Frame hit tags
sidebar:
  label: Mouse Input
  order: 3
---

## Goal

Enable SGR mouse capture, tag widgets with `Id` tokens, handle left/right clicks and scroll wheel events, and optionally enable drag or hover motion tracking.

<!-- src: include/koto/event.hpp:90-129 -->
<!-- src: src/app.cpp:28-33, 50-55, 144-162 -->

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

    ListState items({"Item Alpha", "Item Beta", "Item Gamma", "Item Delta"});
    int click_counter = 0;
    std::string last_hit = "None";

    fm.add(items);
    app.use_focus(fm);

    app.on_mouse([&](const MouseEvent& mouse) {
        if (mouse.is_left_press()) {
            if (mouse.target == Id("card_counter")) {
                click_counter++;
                last_hit = "card_counter";
            } else if (mouse.target == Id(&items)) {
                last_hit = "items list";
            }
        }
    });

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.is(Key::Escape)) {
            app.quit();
        }
    });

    app.on_render([&](Frame& frame) {
        auto [left_area, right_area] = Layout::horizontal(frame.area(), {
            Constraint::percent(50),
            Constraint::fill(),
        });

        List(items)
            .title("Click or Wheel Scroll")
            .border_style(BorderStyle::Rounded)
            .highlight_symbol("> ")
            .highlight_fg(Color::Emerald)
            .draw(frame, left_area);

        auto [card_area, btn_area] = Layout::vertical(right_area, {
            Constraint::fill(),
            Constraint::fixed(3),
        });

        Box()
            .title("Clickable Panel")
            .border_style(BorderStyle::Rounded)
            .id(Id("card_counter"))
            .draw(frame, card_area);

        std::string info = "Clicks: " + std::to_string(click_counter) + "\nLast target: " + last_hit;
        Text(info).draw(frame, card_area.inset(1, 2, 1, 2));

        Button("Reset Counter")
            .id(Id("btn_reset"))
            .focus(fm)
            .on_click([&]() { click_counter = 0; })
            .draw(frame, btn_area);
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

1. **Enabling mouse capture**: Calling either `app.on_mouse(...)` or `app.use_focus(fm)` invokes `m_term.enable_mouse_capture()` (`[?1000h[?1006h`), which enables SGR 1006 click and wheel reporting (`src/app.cpp:31, 53`, `src/terminal.cpp:373`).
2. **Automatic `MouseEvent::target` enrichment**: When a `MouseEvent` arrives in `App::dispatch_event`, `App` sets `m.target = m_frame.hit_at(m.row, m.col)` before calling `m_mouse_handler(m)` and `m_focus_manager->handle_mouse(m, m_frame)` (`src/app.cpp:146-160`).
3. **Sub-item routing in `FocusManager`**:
   - Left-clicking a `List`, `Tabs`, or `Button` registered in `FocusManager` focuses that widget and forwards the `MouseEvent` to `ListState::handle_mouse` (selecting the clicked row), `TabsState::handle_mouse` (selecting the clicked tab), or `Button`'s `on_click` callback (`src/focus.cpp:673-681`).
   - Scrolling `MouseButton::WheelUp` or `MouseButton::WheelDown` over a registered `List`, `Tabs`, or `TextArea` routes the wheel event to the hovered widget via `frame.hit_at(mouse.row, mouse.col)` even if another widget currently holds keyboard focus (`src/focus.cpp:663-670`).

<!-- src: src/app.cpp:144-162 -->
<!-- src: src/focus.cpp:636-700 -->

## Common mistakes

- **Drawing child text over a tagged `Box` and losing the tag**: Wait—does `Text::draw()` overwrite the `hit buffer`? Let's check `src/frame.cpp:127`: `Frame::set_cell(row, col, c, tag_id)` only writes to `m_hit_buffer` when `!tag_id.is_none()`. Because `Text::draw()` calls `frame.set_cell` with the default `tag_id = Id::none()`, drawing `Text` inside a tagged `Box` preserves the `Box`'s `id` in `m_hit_buffer` (`src/frame.cpp:127-140`). However, if you draw an untagged child widget that explicitly passes a different `Id`, the child's tag overwrites those cells.
- **Expecting `MouseAction::Move` without enabling drag or hover**: Standard `enable_mouse_capture()` enables mode `1000h` (press, release, and wheel only) to avoid flooding stdin with motion events (`src/terminal.cpp:371-373`). Call `Terminal::enable_mouse_drag()` (`?1002h`) or `Terminal::enable_mouse_hover()` (`?1003h`) if you need `MouseAction::Move` events.

## See also

- [IDs and Hit Buffer Concept](/concepts/ids-hit-buffer/)
- [Events and EventLoop](/api/events/)
- [FocusManager](/api/focus-manager/)
