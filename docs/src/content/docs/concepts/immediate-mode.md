---
title: Immediate-Mode Rendering and State Structs
description: How libkoto separates transient per-frame widget builders from persistent state struct objects
sidebar:
  label: Immediate Mode
  order: 1
---

## The idea in plain words

In `libkoto`, you do not keep a persistent tree of UI `widget` objects that you mutate across callbacks. Instead, you store your data and cursor/selection positions in plain C++ variables or a `state struct`, and inside `App::on_render` you construct lightweight `widget` builders on the stack and call `.draw(frame, area)` to paint the current state into the `frame`.

## How koto implements it

Every drawable component derives from `koto::Widget` (`include/koto/widget.hpp:8`) and implements a single virtual method:

```cpp
// frame and body_area are provided by the render context
ListState tasks({"Compile library", "Run ctest"});
List(tasks)
    .border_style(BorderStyle::Rounded)
    .title("Tasks")
    .highlight_symbol("> ")
    .draw(frame, body_area);
```

- **Stateless widgets** (`Box`, `Text`, `Fill`, `Clear`, `Rule`, `Gauge`, `Sparkline`) hold only their styling and content parameters (`m_fg`, `m_bg`, `m_title`, `m_ratio`) and write cells directly into `Frame::m_back_buffer` during `draw()`.
- **Stateful widgets** (`List`, `Tabs`, `TextInput`, `TextArea`) take a reference to a persistent `state struct` (`ListState&`, `TabsState&`, `TextInputState&`, `TextAreaState&`). During `draw()`, the `widget` reads items or text from the `state struct`, normalizes scroll offsets (`m_state.scroll`, `m_state.scroll_row`, `m_state.scroll_col`, `m_state.scroll_offset`) against the current `area` dimensions, and records hit rectangles (`m_state.rendered_area`, `m_state.rendered_inner`, `m_state->rendered_tabs`) for subsequent mouse events.
- **Render pass lifecycle**: In `App::draw_frame(bool flush)` (`src/app.cpp:184-197`), `App` clears the back buffer via `m_frame.clear_back_buffer()`, invokes `m_render_handler(m_frame)`, calls `m_focus_manager->sync_geometry(m_frame)` to harvest bounding boxes, and calls `m_frame.render(flush)`.

<!-- src: include/koto/widget.hpp:8-14 -->
<!-- src: src/app.cpp:184-197 -->
<!-- src: src/list.cpp:305-400 -->

## Consequences for your code

- **Declare every `state struct` outside `on_render`**: If you declare `ListState` or `TextInputState` inside the `on_render` lambda, its selection and cursor reset on every frame and any reference registered in `FocusManager` dangles as soon as `on_render` returns.
- **Conditional UI requires no teardown**: To hide a panel or close a dialog, stop calling `.draw(frame, area)` for it inside `on_render`. Because `m_frame.clear_back_buffer()` runs at the start of every `draw_frame()`, omitted widgets disappear on the next frame and `FocusManager::sync_geometry()` sees `frame.bounds_of(id) == std::nullopt`, skipping the hidden target during `Tab` navigation (`src/focus.cpp:382-406`).

<!-- src: src/focus.cpp:382-406 -->

## See also

- [App](/api/app/)
- [FocusManager](/api/focus-manager/)
- [Frames and Diff Renderer](/concepts/frames-diff-renderer/)
