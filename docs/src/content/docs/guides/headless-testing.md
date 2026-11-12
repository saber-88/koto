---
title: Headless Snapshot and Input Testing
description: How to test widgets, layouts, FocusManager routing, and escape sequence parsing in CI without a TTY
sidebar:
  label: Headless Testing
  order: 7
---

## Goal

Write deterministic, TTY-free unit tests that render widgets into an in-memory `Frame`, assert on plain-text snapshots, cells, and hit-buffer tags, and simulate keyboard, mouse, and paste events through `EventLoop` and `App`.

<!-- src: include/koto/frame.hpp:20, 47-58 -->
<!-- src: include/koto/app.hpp:47-50 -->
<!-- src: include/koto/event.hpp:168-184 -->

## Complete working example

```cpp
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <koto/koto.hpp>
#include <cassert>
#include <string>

using namespace koto;

int main()
{
    // 1. Standalone headless Frame snapshot test
    {
        Frame frame(5, 24);
        Box()
            .title("Files")
            .title_align(TitleAlign::Left)
            .border_style(BorderStyle::Rounded)
            .id(Id("files_box"))
            .draw(frame, frame.area());

        assert(frame.to_plain_text_row(0).starts_with("\xE2\x95\xAD\xE2\x94\x80 Files \xE2\x94\x80"));
        assert(frame.hit_at(2, 2) == Id("files_box"));
        assert(frame.bounds_of(Id("files_box")) == Area(0, 0, 24, 5));
    }

    // 2. Full App + FocusManager headless integration test without a TTY
    {
        App app;
        FocusManager fm;
        TextInputState input("init");
        bool clicked = false;

        fm.add(input);
        app.use_focus(fm);

        app.on_render([&](Frame& frame) {
            auto [top_area, bottom_area] = Layout::vertical(frame.area(), {
                Constraint::fixed(3),
                Constraint::fixed(3),
            });
            TextInput(input).draw(frame, top_area);
            Button("Run")
                .id(Id("btn_run"))
                .focus(fm)
                .on_click([&]() { clicked = true; })
                .draw(frame, bottom_area);
        });

        // Resize headless frame and render with flush = false
        app.frame().handle_resize(10, 30);
        app.draw_frame(false);

        // Parse synthetic Kitty CSI-u key and dispatch
        Event tab_ev = EventLoop::parse("\033[9u");
        app.dispatch_event(tab_ev);
        assert(fm.is_focused(Id("btn_run")));

        // Parse SGR mouse click at row 4, col 5 (1-based: [<0;6;5M)
        Event click_ev = EventLoop::parse("\033[<0;6;5M");
        app.dispatch_event(click_ev);
        assert(clicked);
    }

    return 0;
}
```

## Step-by-step explanation

1. **In-memory `Frame(rows, cols)`**: Constructing `Frame(rows, cols)` sets `m_term = nullptr` (`src/frame.cpp:28-35`). You can draw any widget into it, inspect `frame.to_plain_text_row(r)`, `frame.to_plain_text()`, `frame.get_cell(r, c)`, `frame.hit_at(r, c)`, `frame.bounds_of(id)`, `frame.cursor_row()`, and `frame.cursor_col()`, and call `frame.render(false)` to inspect the generated ANSI sequence via `frame.raw_output()`.
2. **Driving `App` without `app.run()`**: Because `app.run()` requires `m_term.is_tty() == true` (`src/app.cpp:201`), headless tests drive `App` directly using:
   - `app.frame().handle_resize(rows, cols)` (or `app.frame() = Frame(rows, cols)`) to set deterministic screen dimensions.
   - `app.draw_frame(false)` to clear the back buffer, invoke `on_render`, call `fm.sync_geometry(m_frame)`, and run `m_frame.render(false)` without writing to `STDOUT_FILENO` (`src/app.cpp:184-197`).
   - `app.dispatch_event(ev)` to route synthetic `KeyEvent`, `MouseEvent`, `PasteEvent`, or `ResizeEvent` objects through `FocusManager` and `App` callbacks (`src/app.cpp:84-182`).
3. **Testing streaming input with `EventLoop::feed` and `poll(0)`**: To test split escape sequences or partial pastes, call `loop.feed(chunk1)`, check `loop.has_pending()` and `loop.has_pending_event()`, then call `loop.feed(chunk2)` and `loop.poll(0)` (`include/koto/event.hpp:173-184`).

<!-- src: src/app.cpp:84-197 -->
<!-- src: src/frame.cpp:28-35, 537-580 -->

## Common mistakes

- **Calling `app.run()` in a CI test runner**: In CI (`ctest`), `stdin` and `stdout` are pipes (`isatty() == 0`), so `app.run()` immediately returns `std::unexpected(TerminalErrorInfo{TerminalError::NotATty, ENOTTY})`. Always use `app.draw_frame(false)` and `app.dispatch_event(...)` in automated tests.
- **Dispatching spatial arrow keys or mouse clicks before calling `app.draw_frame(false)`**: Both `FocusManager::move(Direction)` and `FocusManager::handle_mouse(...)` rely on bounding boxes and hit tags stamped into `Frame` during `on_render` and harvested by `sync_geometry(frame)`. Call `app.draw_frame(false)` once before dispatching mouse clicks or 2D spatial navigation keys.

## See also

- [Frame](/api/frame/)
- [App](/api/app/)
- [Events and EventLoop](/api/events/)
- [Building and Running](/getting-started/building-running/)
