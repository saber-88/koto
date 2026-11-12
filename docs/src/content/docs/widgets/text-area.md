---
title: TextArea
description: Multi-line text editor widget with dynamic line-number gutter, sticky visual column navigation, and margin scrolling
sidebar:
  label: TextArea
  order: 11
---

`koto::TextArea` renders a multi-line editable document backed by a `koto::TextAreaState` `state struct`, complete with a dynamic line-number gutter, 2D viewport scrolling, and hardware cursor positioning (Header: `koto/text_area.hpp`).

## Quick example

```cpp title="Multi-line editor" render="36x6"
TextAreaState editor("int main() {\n    return 0;\n}");

TextArea(editor)
    .title("main.cpp")
    .border_style(BorderStyle::Rounded)
    .line_numbers(true)
    .draw(frame, editor_area);
```

```text frame="terminal" title="Output"
╭─ main.cpp ───────────────────────╮
│ 1 │int main() {                  │
│ 2 │    return 0;                 │
│ 3 │}                             │
│   │                              │
╰──────────────────────────────────╯
```

## Behavior

- **Automatic hit-buffer tagging**: `TextArea::draw()` computes `effective_id = m_id.is_none() ? Id(&m_state) : m_id` and calls `frame.tag_area(area, effective_id)` (`src/text_area.cpp:502-503`).
- **Dynamic line-number gutter**: When both `m_show_line_numbers` (`.line_numbers(true)`) and `m_state.show_line_number` are `true`, `TextArea` computes `digit_w = std::max(2, digits_in(total_lines))` and `gutter_w = digit_w + 2` (right-aligned line number + space + `'│'`). If `inner.width <= gutter_w + 1`, the gutter automatically hides so content still has room to render (`src/text_area.cpp:544-561`).
- **Sticky vs. Clamp vertical cursor movement (`CursorColumnMode`)**:
  - `CursorColumnMode::Sticky` (default): Horizontal edits and left/right moves update `preferred_col` to the current visual display column. Moving `Up` or `Down` maps `preferred_col` into the target line via `visual_col_to_byte_offset(line, preferred_col)` without overwriting `preferred_col`, so passing across a short line and onto a longer line restores the original column (`src/text_area.cpp:214-252`).
  - `CursorColumnMode::Clamp`: Moving `Up` or `Down` updates `preferred_col` to the clamped visual column of the target line.
- **Margin vs. Clamp viewport scrolling (`ScrollMode`)**:
  - `ScrollMode::Margin` (default, with `scroll_margin = 1`): Keeps `v_margin = std::min(scroll_margin, view_h / 2)` rows vertically and `h_margin = std::min(scroll_margin * 2, view_w / 2)` columns horizontally visible ahead of the cursor (`src/text_area.cpp:575-615`).
  - `ScrollMode::Clamp`: Uses `v_margin = 0` and `h_margin = 0`, scrolling only when the cursor leaves the viewport edge.
- **Hardware cursor**: When focused (`is_foc == true`) and the cursor lies inside the visible viewport, `TextArea::draw()` calls `frame.set_cursor(content_area.row + cursor_rel_row, content_area.col + cursor_rel_col)` (`src/text_area.cpp:679-689`).

<!-- src: include/koto/text_area.hpp:14-99 -->
<!-- src: src/text_area.cpp:17-690 -->

## Keys and mouse

Handled by `TextAreaState::handle_event(const KeyEvent&)` and `TextAreaState::handle_mouse(const MouseEvent&)`:

| Input | Condition | Action |
| :--- | :--- | :--- |
| `Key::Up` / `Key::Down` | `focused` | Calls `move_up()` / `move_down()` and returns `true`. |
| `Key::Left` / `Key::Right` | `focused` | Calls `move_left()` / `move_right()` (wraps across line boundaries) and returns `true`. |
| `Key::Home` or `Ctrl+A` (`key.ctrl && (ch == 'A' \|\| ch == 'a')`) | `focused` | Calls `move_home()` (`cursor_col = 0`, `preferred_col = 0`) and returns `true`. |
| `Key::End` or `Ctrl+E` (`key.ctrl && (ch == 'E' \|\| ch == 'e')`) | `focused` | Calls `move_end()` (`cursor_col = line.size()`) and returns `true`. |
| `Key::Backspace` | `focused` | Calls `backspace()` (erases previous grapheme or merges with previous line at `cursor_col == 0`) and returns `true`. |
| `Key::Delete` | `focused` | Calls `delete_forward()` (erases next grapheme or merges next line at end of line) and returns `true`. |
| `Key::Enter` | `focused` | Calls `newline()` (splits current line at `cursor_col`) and returns `true`. |
| `Key::Tab` | `focused` | Calls `insert("    ")` (4 spaces) and returns `true`. |
| Printable character (`!key.ctrl && !key.alt && key.is_char()`) | `focused` | Calls `insert(static_cast<char32_t>(key.ch))` and returns `true`. |
| `MouseButton::WheelUp` / `MouseButton::WheelDown` | Any | Calls `move_up()` / `move_down()` and returns `true`. |
| `PasteEvent` (via `FocusManager`) | `HasPasteHandler<TextAreaState>` | Calls `state.insert(str)` (`\n` splits lines, `\r` is ignored). |

<!-- src: src/text_area.cpp:331-410 -->

## State

`koto::TextAreaState` satisfies `FocusableState`, `HasMouseHandler`, and `HasPasteHandler`:

| Member | Type / Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `text` | `std::vector<std::string>` | Document lines (guaranteed non-empty by `clamp_state`, at least `{""}`). | `{""}` |
| `cursor_row` | `size_t` | 0-based active line index. | `0` |
| `cursor_col` | `size_t` | 0-based UTF-8 byte offset within `text[cursor_row]`. | `0` |
| `preferred_col` | `size_t` | Target visual display column used in `CursorColumnMode::Sticky`. | `0` |
| `scroll_row` / `scroll_col` | `size_t` | Top visible line index and left visible visual column offset. | `0`, `0` |
| `scroll_margin` | `size_t` | Margin size used when `scroll_mode == ScrollMode::Margin`. | `1` |
| `show_line_number` | `bool` | State-level toggle for the line-number gutter. | `true` |
| `focused` | `bool` | Whether the editor processes key events and displays the hardware cursor. | `true` |
| `cursor_mode` | `CursorColumnMode` | `CursorColumnMode::Sticky` or `CursorColumnMode::Clamp`. | `CursorColumnMode::Sticky` |
| `scroll_mode` | `ScrollMode` | `ScrollMode::Margin` or `ScrollMode::Clamp`. | `ScrollMode::Margin` |
| `TextAreaState()` / `TextAreaState(initial_text)` / `TextAreaState(initial_lines)` | Constructors | Initializes empty, newline-split, or line-vector document. | Cursor at `(0, 0)` |
| `insert(char32_t)` / `insert(std::string_view)` | `void` | Inserts a codepoint or UTF-8 string at `(cursor_row, cursor_col)`. | None |
| `newline()` / `backspace()` / `delete_forward()` | `void` | Splits line, deletes backward by grapheme/line, or deletes forward by grapheme/line. | None |
| `move_left()` / `right()` / `up()` / `down()` / `home()` / `end()` | `void` | Moves the cursor by grapheme or visual column. | None |
| `clear()` / `set_text(new_text)` / `to_string() const` | Document helpers | Clears to `{""}`, replaces text (stripping `\r` before `\n`), or joins `text` with `\n`. | None |
| `handle_event(key)` / `handle_mouse(mouse)` | `bool` | Handles keyboard or mouse wheel events. | None |

<!-- src: include/koto/text_area.hpp:26-64 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `TextArea(TextAreaState& state)` | Constructs a `TextArea` bound to `state`. | None |
| `border_style(BorderStyle style)` | Sets border style (`BorderStyle::None` disables border and inset). | `BorderStyle::Single` |
| `title(std::string_view title)` | Sets border title string. | `""` |
| `title_align(TitleAlign align)` | Sets horizontal border title alignment. | `TitleAlign::Left` |
| `line_numbers(bool show)` | Enables or disables the line-number gutter. | `true` |
| `fg(Color fg)` | Sets text foreground color (and unfocused border/title color). | `COLOR_DEFAULT` |
| `bg(Color bg)` | Sets editor background color. | `COLOR_DEFAULT` |
| `line_number_fg(Color fg)` | Sets foreground color of the gutter digits and `'│'` separator. | `Color::DarkGray` |
| `focused(bool f)` | Overrides `m_state.focused` for border styling and hardware cursor display. | Uses `m_state.focused` |
| `focused_border_fg(Color fg)` | Sets border and title foreground color when focused. | `Color::Cyan` |
| `id(Id tag_id)` | Overrides automatic `Id(&m_state)` hit-buffer tag. | `Id::none()` |
| `cursor_mode(CursorColumnMode mode)` | Mutates `m_state.cursor_mode` (`Sticky` or `Clamp`). | `CursorColumnMode::Sticky` |
| `scroll_mode(ScrollMode mode, size_t margin = 1)` | Mutates `m_state.scroll_mode` and `m_state.scroll_margin`. | `ScrollMode::Margin`, `margin = 1` |
| `draw(Frame& frame, Area area)` | Clamps state, updates scroll, renders gutter and lines, and positions cursor. | None |

<!-- src: include/koto/text_area.hpp:66-99 -->

## Gotchas

:::caution
`TextAreaState::handle_event()` always returns `true` for `Key::Up`, `Key::Down`, `Key::Left`, and `Key::Right` even when the cursor is already at the top-left `(0, 0)` or bottom-right boundary (`src/text_area.cpp:338-357`). Consequently, when `FocusNavPolicy::SpatialArrows` is active on `FocusManager`, arrow keys will never bubble out of a focused `TextArea` to move focus to another widget; use `Tab` / `Shift+Tab` or `FocusNavPolicy::AltArrows` instead.
:::

- **Left-clicking a `TextArea` focuses the widget but does not move `cursor_row`/`cursor_col`**: `TextAreaState::handle_mouse()` handles `MouseButton::WheelUp` and `MouseButton::WheelDown`, returning `false` for left clicks (`src/text_area.cpp:397-410`), while `FocusManager::handle_mouse()` shifts focus to the `TextArea`.

<!-- TODO(verify): Since FocusManager::handle_key() intercepts Key::Tab at step 2 (src/focus.cpp:555) before calling the focused target's handle_key() at step 4 (src/focus.cpp:570), is Key::Tab in TextAreaState::handle_event() (src/text_area.cpp:383) intended only for standalone usage or custom FocusKeyInterceptors? -->

## See also

- [TextInput](/widgets/text-input/)
- [Text](/widgets/text/)
- [UTF-8 and Grapheme](/api/utf8/)
