---
title: TextInput
description: Single-line text input widget with grapheme-atomic editing, horizontal scroll, input filtering, and key interception
sidebar:
  label: TextInput
  order: 12
---

`koto::TextInput` renders a single-line editable field bound to a `koto::TextInputState` `state struct` with placeholder text, horizontal scrolling, and hardware cursor tracking (Header: `koto/text_input.hpp`).

## Quick example

```cpp title="Single-line text input" render="30x3"
TextInputState username("saber-88");

TextInput(username)
    .title("Username")
    .placeholder("Enter username...")
    .border_style(BorderStyle::Rounded)
    .draw(frame, input_area);
```

```text frame="terminal" title="Output"
╭───────── Username ─────────╮
│saber-88                    │
╰────────────────────────────╯
```

## Behavior

- **Automatic hit-buffer tagging**: `TextInput::draw()` sets `effective_id = m_id.is_none() ? Id(&m_state) : m_id` and calls `frame.tag_area(area, effective_id)` (`src/text_input.cpp:309-310`).
- **Placeholder rendering**: When `m_state.value.empty()`, if `!m_placeholder.empty()`, `TextInput` draws `Text(m_placeholder).fg(m_placeholder_fg).bg(m_bg).clip()` inside `inner` and, if `m_state.focused` is `true`, places the hardware cursor at `(inner.row, inner.col)` (`src/text_input.cpp:351-366`).
- **Horizontal scroll window**: When `m_state.value` is non-empty, `TextInput::draw()` segments `m_state.value` into `Grapheme` clusters, finds the visual column `cursor_visual_col` corresponding to byte offset `m_state.cursor`, and adjusts `m_state.scroll_offset` so `cursor_visual_col` stays within `[scroll_offset, scroll_offset + inner.width - 1]` (`src/text_input.cpp:383-415`).
- **Interceptor and filter hooks**:
  - `TextInputState::on_key(KeyInterceptor)` runs at the start of `handle_event(key)` before any built-in key bindings; if it returns `true`, the event is considered consumed (`src/text_input.cpp:185-188`).
  - `TextInputState::set_filter(InputFilter)` runs inside `insert(char32_t cp)` before encoding and inserting `cp`; if it returns `false`, `insert()` rejects the character and returns `false` (`src/text_input.cpp:55-58`).

<!-- src: include/koto/text_input.hpp:16-84 -->
<!-- src: src/text_input.cpp:18-436 -->

## Keys and mouse

Handled by `TextInputState::handle_event(const KeyEvent&)`:

| Input | Condition | Action |
| :--- | :--- | :--- |
| `on_key_handler(*this, key)` | `focused && on_key_handler != nullptr` | Runs user interceptor first; stops and returns `true` if handler returns `true`. |
| `Key::Left` (`key.is(Key::Left)`) | `focused` | Calls `move_left()` (returns `true` if `cursor > 0`, `false` if already at `0`). |
| `Key::Right` (`key.is(Key::Right)`) | `focused` | Calls `move_right()` (returns `true` if `cursor < value.size()`, `false` if at end). |
| `Key::Home` or `Ctrl+A` (`key.ctrl && (ch == 'A' \|\| ch == 'a')`) | `focused` | Calls `move_home()` (returns `true` if `cursor != 0`, `false` otherwise). |
| `Key::End` or `Ctrl+E` (`key.ctrl && (ch == 'E' \|\| ch == 'e')`) | `focused` | Calls `move_end()` (returns `true` if `cursor != value.size()`, `false` otherwise). |
| `Ctrl+U` (`key.ctrl && (ch == 'U' \|\| ch == 'u')`) | `focused` | If `!value.empty()`, calls `clear()` and returns `true`; otherwise returns `false`. |
| `Ctrl+W` (`key.ctrl && (ch == 'W' \|\| ch == 'w')`) | `focused` | Calls `delete_word()` (deletes preceding spaces then non-spaces back to `' '` or `0`). |
| `Key::Backspace` | `focused` | Calls `backspace()` (erases preceding grapheme cluster; returns `false` if at `0`). |
| `Key::Delete` | `focused` | Calls `delete_forward()` (erases following grapheme cluster; returns `false` if at end). |
| Printable character (`!key.ctrl && !key.alt && key.is_char()`) | `focused` | Calls `insert(static_cast<char32_t>(key.ch))` and returns `true` if inserted. |
| Left mouse press (via `FocusManager`) | Click inside `area` | `FocusManager` focuses `TextInputState` (`TextInputState` has no `handle_mouse` method). |
| `PasteEvent` (via `FocusManager`) | `HasPasteHandler<TextInputState>` | Calls `state.insert(str)` (passing each codepoint through `insert(char32_t)` and `input_filter`). |

<!-- src: src/text_input.cpp:177-240 -->

## State

`koto::TextInputState` satisfies `FocusableState` and `HasPasteHandler`:

| Member | Type / Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `KeyInterceptor` | `std::function<bool(TextInputState&, const KeyEvent&)>` | Pre-hook signature for `on_key()`. | None |
| `InputFilter` | `std::function<bool(std::string_view current_text, char32_t new_char)>` | Character validation signature for `set_filter()`. | None |
| `value` | `std::string` | Current UTF-8 text buffer. | `""` |
| `cursor` | `size_t` | UTF-8 byte offset of the caret in `value`. | `0` (or `initial_text.size()`) |
| `scroll_offset` | `int` | Horizontal visual column scroll offset. | `0` |
| `focused` | `bool` | Whether the input accepts keystrokes and displays the hardware cursor. | `true` |
| `on_key_handler` / `input_filter` | `KeyInterceptor` / `InputFilter` | Stored interceptor and filter callbacks. | `nullptr` |
| `TextInputState()` / `TextInputState(initial_text)` | Constructors | Initializes empty or pre-filled state (`cursor` placed at `value.size()`). | `""` |
| `handle_event(key)` | `bool handle_event(const KeyEvent& key)` | Processes `key` and returns `true` if state changed or consumed. | None |
| `on_key(handler)` / `set_filter(filter)` | `TextInputState&` | Registers `on_key_handler` or `input_filter` and returns `*this`. | None |
| `insert(char32_t)` / `insert(std::string_view)` | `bool` | Inserts character or string at `cursor` if accepted by `input_filter`. | None |
| `backspace()` / `delete_forward()` / `delete_word()` | `bool` | Deletes preceding grapheme, following grapheme, or preceding whitespace-delimited word. | None |
| `move_left()` / `move_right()` / `move_home()` / `move_end()` | `bool` | Moves `cursor` and returns `true` only if `cursor` actually changed. | None |
| `set_text(text)` / `text() const` / `clear()` | `void` / `std::string_view` / `void` | Replaces `value` (placing `cursor` at end), returns `std::string_view(value)`, or clears to empty. | None |

<!-- src: include/koto/text_input.hpp:16-53 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `TextInput(TextInputState& state)` | Constructs a `TextInput` bound to `state`. | None |
| `placeholder(std::string_view placeholder)` | Sets the hint string drawn when `m_state.value.empty()`. | `""` |
| `title(std::string_view title)` | Sets the border title (always centered on the top border by `Box` default). | `""` |
| `border_style(BorderStyle style)` | Sets the surrounding border style (`BorderStyle::None` disables border). | `BorderStyle::Single` |
| `fg(Color color)` | Sets text foreground color and unfocused border color. | `COLOR_DEFAULT` |
| `bg(Color color)` | Sets interior and border background color. | `COLOR_DEFAULT` |
| `placeholder_fg(Color color)` | Sets the placeholder text foreground color. | `0x00888888` |
| `focused_border_fg(Color color)` | Sets the border and title foreground color when focused. | `0x0055FFFF` |
| `focused(bool is_focused)` | Overrides `m_state.focused` for border highlighting. | Uses `m_state.focused` |
| `id(Id tag_id)` | Overrides automatic `Id(&m_state)` hit-buffer tag. | `Id::none()` |
| `draw(Frame& frame, Area area)` | Updates scroll offset, renders border and text, and positions the hardware cursor. | None |

<!-- src: include/koto/text_input.hpp:55-84 -->

## Gotchas

- **Hardware cursor checks `m_state.focused` directly**: Even if you call `TextInput(state).focused(true)`, `TextInput::draw()` checks `if (m_state.focused)` (`src/text_input.cpp:362, 428`) when calling `frame.set_cursor(...)`, whereas the border highlight uses `bool is_foc = m_has_focused_override ? m_focused : m_state.focused`. Keep `state.focused` synchronized (which `FocusManager` does automatically).
- **Boundary arrow keys return `false`**: Unlike `TextAreaState`, `TextInputState::move_left()` returns `false` when `cursor == 0`, and `move_right()` returns `false` when `cursor >= value.size()`. If `FocusNavPolicy::SpatialArrows` is enabled, pressing `Left` at column 0 or `Right` at the end of the text will bubble to `FocusManager` and jump focus to an adjacent widget.

<!-- TODO(verify): Should TextInput::draw() check is_foc instead of m_state.focused at src/text_input.cpp:362 and 428 when calling frame.set_cursor(), matching TextArea::draw() at src/text_area.cpp:679? -->

## See also

- [TextArea](/widgets/text-area/)
- [FocusManager](/api/focus-manager/)
- [Keyboard Guide](/guides/keyboard/)
