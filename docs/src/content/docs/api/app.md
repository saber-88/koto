---
title: App
description: Application runtime that owns the terminal, frame, and event loop and dispatches input, ticks, and render passes
sidebar:
  label: App
  order: 1
---

## Purpose and header

`koto::App` is the top-level runtime coordinator that owns a `Terminal`, a `Frame`, and an `EventLoop`, connects optional user callbacks and a `FocusManager`, and runs the 60 FPS event and rendering loop.
Header: `koto/app.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/app.hpp:1-70 -->

## Types and members

| Member | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `App()` | `App()` | Constructs `m_term`, `m_frame(m_term)`, and `m_loop` without entering raw mode. | `m_running = true`, `m_needs_draw = true` |
| `~App()` | `~App() = default` | Destroys the application and restores terminal state via `m_term.~Terminal()`. | `= default` |
| `on_key()` | `App& on_key(std::function<void(const KeyEvent&)> handler)` | Registers the callback invoked for key events not consumed by `FocusManager`. | `nullptr` |
| `on_paste()` | `App& on_paste(std::function<void(const std::string&)> handler)` | Registers the callback invoked when a `PasteEvent` arrives. | `nullptr` |
| `on_mouse()` | `App& on_mouse(std::function<void(const MouseEvent&)> handler)` | Registers the mouse callback and immediately calls `m_term.enable_mouse_capture()`. | `nullptr` |
| `on_tick()` | `App& on_tick(std::function<void()> handler)` | Registers a parameterless periodic tick callback fired every 16 ms (~60 FPS). | `nullptr` |
| `on_tick(dt)` | `App& on_tick(std::function<void(double)> handler)` | Registers a periodic tick callback receiving elapsed seconds `dt` as a `double`. | `nullptr` |
| `on_render()` | `App& on_render(std::function<void(Frame&)> handler)` | Registers the callback that draws widgets into the `frame` back buffer. | `nullptr` |
| `use_focus()` | `App& use_focus(FocusManager& fm)` | Attaches `fm`, calls `m_term.enable_mouse_capture()`, syncs geometry after each draw, and routes input events through `fm`. | `nullptr` |
| `quit()` | `void quit()` | Sets `m_running = false` and calls `m_loop.wakeup()` to exit `run()`. | None |
| `request_redraw()` | `void request_redraw() noexcept` | Sets `m_needs_draw = true` and wakes the event loop via `m_loop.wakeup()`. | None |
| `run()` | `auto run() -> std::expected<void, TerminalErrorInfo>` | Enters raw mode, hides the cursor, runs the event/render loop until `quit()` or EOF, and restores the terminal. | None |
| `run(render_function)` | `auto run(std::function<void(Frame&)> render_function) -> std::expected<void, TerminalErrorInfo>` | Calls `on_render(std::move(render_function))` and then `run()`. | None |
| `area()` | `Area area() const noexcept` | Returns `m_frame.area()` (`Area{0, 0, cols, rows}`). | `{0, 0, 80, 24}` fallback |
| `hit_at(row, col)` | `Id hit_at(int row, int col) const` | Queries the `hit buffer` at `(row, col)` via `m_frame.hit_at(row, col)`. | `Id::none()` |
| `hit_at(mouse)` | `Id hit_at(const MouseEvent& mouse) const` | Queries `m_frame.hit_at(mouse.row, mouse.col)`. | `Id::none()` |
| `draw_frame()` | `void draw_frame(bool flush = true)` | Clears the back buffer, invokes `on_render`, calls `fm.sync_geometry(m_frame)`, and calls `m_frame.render(flush)`. | `flush = true` |
| `dispatch_event()` | `void dispatch_event(const Event& ev)` | Routes `ev` (`ResizeEvent`, `PasteEvent`, `MouseEvent`, or `KeyEvent`) to `FocusManager` and registered callbacks. | None |
| `frame()` | `Frame& frame() noexcept` / `const Frame& frame() const noexcept` | Returns a mutable or const reference to the internal `frame`. | None |

<!-- src: include/koto/app.hpp:12-51 -->
<!-- src: src/app.cpp:12-329 -->

### Event dispatch order in `App::dispatch_event`

| Event variant | Dispatch behavior |
| :--- | :--- |
| `ResizeEvent` | Updates `m_term.set_dimensions(rows, cols)`, calls `m_frame.handle_resize(rows, cols)`, and sets `m_needs_draw = true`. Does not invoke user callbacks. |
| `PasteEvent` | 1. If `on_paste` is set, calls `m_paste_handler(paste->text)` and returns. 2. Else if `m_focus_manager && m_focus_manager->handle_paste(paste->text)` returns `true`, returns. 3. Else iterates codepoints with `next_codepoint`, synthesizes a `KeyEvent` for each codepoint with `cp >= 32` or `cp == '\t'` (setting `shift = true` for `'A'..'Z'`), and routes each through `m_focus_manager->handle_key(ke)` then `m_key_handler(ke)`. Never synthesizes `Key::Enter`. |
| `MouseEvent` | Copies the event, populates `m.target = m_frame.hit_at(m.row, m.col)`, invokes `m_mouse_handler(m)` if registered, and then invokes `m_focus_manager->handle_mouse(m, m_frame)` if `use_focus` was called. |
| `KeyEvent` | Calls `m_focus_manager->handle_key(*key)` first if attached; only invokes `m_key_handler(*key)` if the `FocusManager` did not consume the key. |

<!-- src: src/app.cpp:84-182 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cstdio>

using namespace koto;

int main()
{
    App app;
    TextInputState name_input("koto");
    FocusManager fm;
    fm.add(name_input);
    app.use_focus(fm);

    app.on_key([&](const KeyEvent& key) {
        if (key.is(Key::Escape) || (key.ctrl && key.ch == 'C')) {
            app.quit();
        }
    });

    app.on_render([&](Frame& frame) {
        TextInput(name_input)
            .title("Project")
            .border_style(BorderStyle::Rounded)
            .draw(frame, Layout::center(frame.area(), 30, 3));
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

### Headless frame and event verification

```cpp
app.on_render([&](Frame& f) {
    Box().title("Headless").id(Id("panel")).draw(f, Area(0, 0, 20, 4));
});
app.draw_frame(false);
Id hit = app.hit_at(1, 1);
static_cast<void>(hit);
```

## Errors and edge cases

- **Non-TTY execution**: `App::run()` checks `m_term.is_tty()` before entering raw mode. If standard input or standard output is piped or redirected, `run()` immediately returns `std::unexpected(TerminalErrorInfo{TerminalError::NotATty, ENOTTY})`.
- **No default quit key**: `App` does not bind `q`, `Escape`, or `Ctrl+C` (raw mode disables `ISIG`). You must call `app.quit()` in an `on_key` or `on_mouse` handler, or let standard input reach EOF.
- **No `on_resize` callback**: `App` handles `ResizeEvent` internally by resizing `m_term` and `m_frame` and marking `m_needs_draw = true`, which triggers `on_render` on the same loop iteration.
- **Cross-thread wakeups**: `App` does not expose a `wakeup()` method directly; call `app.request_redraw()` from background worker threads to mark the frame dirty and wake the internal `EventLoop` self-pipe.

:::caution
When `app.use_focus(fm)` is active and a focused `TextInput` or `TextArea` is active, printable character keys (such as `'q'`) are consumed by the widget before `app.on_key()` runs. Use `Key::Escape` or a `ctrl` modifier combination such as `key.ctrl && key.ch == 'Q'` for global exit shortcuts.
:::

<!-- src: src/app.cpp:164-216 -->

## See also

- [Terminal](/api/terminal/)
- [Frame](/api/frame/)
- [Events](/api/events/)
- [FocusManager](/api/focus-manager/)
