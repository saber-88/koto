---
title: Button
description: Interactive bordered button widget with mouse click, Enter/Space activation, and FocusManager integration
sidebar:
  label: Button
  order: 2
---

`koto::Button` renders a filled, bordered button with a centered label and optionally registers a click and keyboard (`Key::Enter` / space) target in `FocusManager` (Header: `koto/button.hpp`).

## Quick example

```cpp title="Rounded button" render="20x3"
Button("Deploy")
    .id(Id("btn_deploy"))
    .border_style(BorderStyle::Rounded)
    .fg(Color::Cyan)
    .draw(frame, button_area);
```

```text frame="terminal" title="Output"
╭──────────────────╮
│      Deploy      │
╰──────────────────╯
```

## Behavior

- **Automatic `FocusManager` registration during `draw()`**: When both `.focus(fm)` and a non-empty `.id(Id(...))` are set on a `Button`:
  1. If `.on_click(callback)` is also set, `Button::draw()` constructs a `FocusTarget` with `.id = fm.scoped_id(m_id)`, `.bounds = area`, `.modal_scope = fm.active_modal_scope()`, and key/mouse closures, and calls `fm.register_target(std::move(target))` (`src/button.cpp:116-149`).
  2. Unless `.focused(bool)` was called manually on the `Button`, `m_is_focused` is queried automatically via `fm.is_focused(m_id)` (`src/button.cpp:151-154`).
- **Background and border rendering**: `Button::draw()` delegates its box frame to `Box().border_style(m_border_style).fg(border_col).bg(bg_col).id(m_id).focused(m_is_focused).focused_border_fg(m_focused_border_fg).filled(true).draw(frame, area)`.
- **Label centering and clipping**: When `m_label` is non-empty, `Button` computes a 1-row `text_area` at vertical center `area.row + (area.height - 1) / 2` (inset horizontally by 1 column on each side when `area.height > 2 && area.width > 2 && m_border_style != BorderStyle::None`) and draws `Text(m_label).fg(text_col).bg(bg_col).clip().align(TextAlign::Center)`.
- **Color propagation in `.fg()` and `.focused_fg()`**:
  - Calling `.fg(color)` sets `m_fg = color.value` and, if `.label_fg()` has not been called yet, also sets `m_label_fg = color.value` (`src/button.cpp:29-38`).
  - Calling `.focused_fg(color)` sets `m_focused_fg = color.value` and propagates `color.value` to `m_focused_label_fg` (if `.focused_label_fg()` was not called) and `m_focused_border_fg` (if `.focused_border_fg()` was not called) (`src/button.cpp:60-73`).

<!-- src: include/koto/button.hpp:17-72 -->
<!-- src: src/button.cpp:9-192 -->

## Keys and mouse

These bindings are registered in `FocusManager` when `.focus(fm)`, `.id(Id(...))`, and `.on_click(callback)` are all provided before `draw()`:

| Input | Condition | Action |
| :--- | :--- | :--- |
| `Key::Enter` (`k.is(Key::Enter)`) | Button is focused in `FocusManager` | Invokes `on_click` callback and returns `true`. |
| Space (`k.ch == ' '`) | Button is focused in `FocusManager` | Invokes `on_click` callback and returns `true`. |
| Left mouse press (`m.is_left_press()`) | Click lands on a cell tagged with `m_id` | Focuses the button in `FocusManager`, invokes `on_click` callback, and returns `true`. |

<!-- src: src/button.cpp:128-143 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Button()` | Default-constructs a button with an empty label. | `m_label = ""` |
| `Button(std::string_view label)` | Explicit constructor setting `m_label`. | `on_click = nullptr` |
| `Button(std::string_view label, std::function<void()> on_click)` | Constructs a button with `label` and `on_click` callback. | None |
| `label(std::string_view label)` | Sets the button text label. | `""` |
| `border_style(BorderStyle style)` | Sets the surrounding border style. | `BorderStyle::Rounded` |
| `fg(Color color)` | Sets unfocused border color (and unfocused label color if `label_fg` was not set). | `Color::DarkGray` |
| `bg(Color color)` | Sets unfocused interior background color. | `COLOR_DEFAULT` |
| `label_fg(Color color)` | Sets unfocused label text color explicitly. | `Color::LightGray` |
| `focused(bool is_foc)` | Manually overrides focus state (disables querying `fm.is_focused(m_id)`). | `false` |
| `focused_fg(Color color)` | Sets focused foreground and propagates to `focused_label_fg` and `focused_border_fg` if not explicitly set. | `Color::Cyan` |
| `focused_bg(Color color)` | Sets interior background color used when focused. | `Color::DarkGray` (inactive unless `focused_bg()` is called) |
| `focused_border_fg(Color color)` | Sets focused border color explicitly. | `Color::Cyan` |
| `focused_label_fg(Color color)` | Sets focused label text color explicitly. | `Color::White` |
| `id(Id id)` | Sets the `id` token for hit-testing and `FocusManager` registration. | `Id::none()` |
| `focus(FocusManager& fm)` | Attaches `FocusManager` so `draw()` syncs focus state and registers `on_click`. | `nullptr` |
| `on_click(std::function<void()> callback)` | Sets the action callback invoked on `Key::Enter`, `' '`, or left click. | `nullptr` |
| `draw(Frame& frame, Area area)` | Registers with `FocusManager` (if configured) and draws the button into `frame`. | None |

<!-- src: include/koto/button.hpp:17-72 -->

## Gotchas

:::caution
`Button::draw()` only calls `m_fm->register_target(...)` when all three of `.focus(fm)`, `.id(Id(...))`, and `.on_click(callback)` are set (`src/button.cpp:116-119`). If you omit `.on_click(...)`, `Button::draw()` does not register the button in `fm` unless you separately call `fm.add(Id("..."))`. Furthermore, because registration inside `draw()` happens during the render pass, a button that is only registered via `.focus(fm).on_click(...)` is added to `FocusManager` on the first `draw_frame()` pass.
:::

- **Minimum height for borders**: A bordered `Button` requires `area.height >= 3` and `area.width >= 3` to inset the label inside the top and bottom borders (`src/button.cpp:174`). In a 1-row or 2-row area with a border, `text_area` falls back to `area` and overwrites the border row.

## See also

- [Box](/widgets/box/)
- [FocusManager](/api/focus-manager/)
- [Modal Dialogs Guide](/guides/modal-dialogs/)
