---
title: List
description: Scrollable vertical item list widget with ListState selection, Vim and arrow navigation, mouse support, and highlight styling
sidebar:
  label: List
  order: 6
---

`koto::List` renders a scrollable vertical list bound to a `koto::ListState` `state struct` with optional borders, row highlight colors, and a prefix highlight symbol (Header: `koto/list.hpp`).

## Quick example

```cpp title="Bordered list" render="30x6"
ListState state({"alpha.internal", "beta.internal", "gamma.internal"});
state.set_selected(1);

List(state)
    .border_style(BorderStyle::Rounded)
    .title("Hosts")
    .highlight_symbol("> ")
    .highlight_fg(Color::Emerald)
    .draw(frame, list_area);
```

```text frame="terminal" title="Output"
╭─ Hosts ────────────────────╮
│  alpha.internal            │
│> beta.internal             │
│  gamma.internal            │
│                            │
╰────────────────────────────╯
```

## Behavior

- **Automatic hit-buffer tagging**: `List::draw()` computes `effective_id = m_id.is_none() ? Id(&m_state) : m_id` and calls `frame.tag_area(area, effective_id)` (`src/list.cpp:313-314`). Even if you do not call `.id(...)`, `List` stamps `Id(&m_state)` into the `hit buffer` so `FocusManager` harvests its geometry and routes mouse events automatically.
- **Border and inner viewport**: `List::draw()` always draws a `Box` over `area` with `.filled(true)` (`src/list.cpp:316-328`). If `m_border_style == BorderStyle::None` (the default), `inner` equals `area`; otherwise `inner` is `area.inset(1, 1, 1, 1)`. `draw()` stores `m_state.rendered_area = area` and `m_state.rendered_inner = inner` for mouse row hit-testing.
- **State normalization and viewport scrolling**: During `draw()`, `normalize_state()` clamps or wraps `m_state.selected` into `[0, items.size() - 1]` (or resets `selected = 0, scroll = 0` if `items` is empty). It then adjusts `m_state.scroll` so `m_state.selected` lies within `[m_state.scroll, m_state.scroll + visible_rows - 1]` (`src/list.cpp:343-351`).
- **Highlight symbol alignment**: When `m_highlight_symbol` is non-empty, `List::draw()` measures `sym_w = text_width(m_highlight_symbol)`. The selected row renders `m_highlight_symbol` in the first `sym_w` columns, and unselected rows advance `col += sym_w` so all item labels stay vertically aligned (`src/list.cpp:377-388`).

<!-- src: include/koto/list.hpp:18-93 -->
<!-- src: src/list.cpp:16-400 -->

## Keys and mouse

Handled by `ListState::handle_event(const KeyEvent&)` and `ListState::handle_mouse(const MouseEvent&)`:

| Input | Condition | Action |
| :--- | :--- | :--- |
| `key.ch == 'j'` or `key.is(Key::Down)` | `focused && !items.empty()` | Calls `select_next()` and returns `true`. |
| `key.ch == 'k'` or `key.is(Key::Up)` | `focused && !items.empty()` | Calls `select_prev()` and returns `true`. |
| `key.ch == 'g'` or `key.is(Key::Home)` | `focused && !items.empty()` | Calls `select_first()` (`set_selected(0)`) and returns `true`. |
| `key.ch == 'G'` or `key.is(Key::End)` | `focused && !items.empty()` | Calls `select_last()` (`set_selected(items.size() - 1)`) and returns `true`. |
| `MouseButton::WheelUp` | `!items.empty()` | Calls `select_prev()` and returns `true`. |
| `MouseButton::WheelDown` | `!items.empty()` | Calls `select_next()` and returns `true`. |
| `MouseAction::Press` + `MouseButton::Left` | `rendered_inner.contains(mouse.row, mouse.col)` and valid `scroll + row_offset` | Calls `set_selected(scroll + (mouse.row - rendered_inner.row))` and returns `true`. |

<!-- src: src/list.cpp:127-184 -->

## State

`koto::ListState` satisfies `FocusableState` and `HasMouseHandler`:

| Member | Type / Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `items` | `std::vector<std::string>` | Item strings displayed by `List`. | `{}` |
| `selected` | `int` | 0-based index of the currently selected item. | `0` |
| `scroll` | `int` | 0-based index of the top visible item in the viewport. | `0` |
| `focused` | `bool` | Whether `handle_event` accepts keyboard input and `List` draws a focused border. | `true` |
| `navigation` | `ListNavigation` | Boundary behavior (`ListNavigation::Clamp` or `ListNavigation::Wrap`). | `ListNavigation::Clamp` |
| `on_change` | `std::function<void(int, const std::string&)>` | Callback invoked by `set_selected()` (and thus `select_next`/`prev`/`first`/`last`). | `nullptr` |
| `rendered_area` / `rendered_inner` | `Area` | Outer and inner rectangles recorded during the most recent `List::draw()`. | `{0, 0, 0, 0}` |
| `ListState()` / `ListState(items, nav)` | Constructors | Constructs an empty or initialized `ListState`. | `nav = ListNavigation::Clamp` |
| `select_next()` / `select_prev()` / `select_first()` / `select_last()` | `void` | Moves selection and triggers `on_change` via `set_selected()`. | None |
| `set_selected(int index)` | `void` | Clamps or wraps `index` according to `navigation` and invokes `on_change` if set. | None |
| `set_items(new_items)` | `void` | Replaces `items` and clamps `selected` (resets `scroll = 0` if empty). | None |
| `selected_item()` | `std::optional<std::string_view> selected_item() const` | Returns a view of the selected item, or `std::nullopt` if out of bounds/empty. | `std::nullopt` |
| `selected_item_ptr()` | `const std::string* selected_item_ptr() const` | Returns a pointer to the selected item string, or `nullptr` if out of bounds/empty. | `nullptr` |
| `handle_event(key)` / `handle_mouse(mouse)` | `bool` | Processes keyboard or mouse input and returns `true` if consumed. | None |

<!-- src: include/koto/list.hpp:18-50 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `List(ListState& state)` | Explicit constructor binding `List` to `state`. | None |
| `fg(Color color)` | Sets unfocused border foreground and unselected item text color. | `COLOR_DEFAULT` |
| `bg(Color color)` | Sets list background color. | `COLOR_DEFAULT` |
| `highlight_fg(Color color)` | Sets foreground color of the selected row and highlight symbol. | `COLOR_DEFAULT` |
| `highlight_bg(Color color)` | Sets full-width background color of the selected row. | `COLOR_DEFAULT` |
| `border_style(BorderStyle border)` | Sets border style (`BorderStyle::None` draws no border and uses zero inset). | `BorderStyle::None` |
| `highlight_symbol(std::string symbol)` | Sets prefix string drawn before the selected item (and padded with spaces on other items). | `""` |
| `title(std::string title)` | Sets top border title (drawn when `border_style != BorderStyle::None`). | `""` |
| `title_fg(Color color)` | Sets title foreground color. | `COLOR_DEFAULT` |
| `title_bg(Color color)` | Sets title background color. | `COLOR_DEFAULT` |
| `title_align(TitleAlign align)` | Sets horizontal title alignment (`Left`, `Center`, `Right`). | `TitleAlign::Left` |
| `overflow(TextOverflow o)` | Sets item text overflow mode (`Clip` or `Ellipsis`). | `TextOverflow::Ellipsis` |
| `focused(bool is_focused)` | Overrides `m_state.focused` for border highlighting. | Uses `m_state.focused` |
| `focused_border_fg(Color color)` | Sets border color when focused. | `Color::Cyan` |
| `id(Id tag_id)` | Overrides the automatic `Id(&m_state)` hit-buffer tag. | `Id::none()` (uses `Id(&m_state)`) |
| `draw(Frame& frame, Area area)` | Normalizes state, updates scroll, and renders the list into `frame`. | None |

<!-- src: include/koto/list.hpp:52-93 -->

## Gotchas

:::caution
In `ListState::handle_event()`, the checks `if (key.ch == 'j' || key.is(Key::Down))` (`src/list.cpp:131`), `'k'`, `'g'`, and `'G'` do not check `!key.ctrl && !key.alt`. If a `ListState` is focused, pressing `Alt+j` or `Alt+k` is consumed by the list before reaching `app.on_key()`.
:::

- **Default `border_style` is `BorderStyle::None`**: Unlike `Box`, `TextInput`, and `TextArea` (which default to `BorderStyle::Single`), `List` defaults to `BorderStyle::None` (`include/koto/list.hpp:80`). If you set `.title("...")` without setting `.border_style(...)`, the box border is drawn with blank spaces around the title and items are not inset.
- **Default `highlight_fg` and `highlight_symbol`**: `m_highlighted_fg` and `m_highlighted_bg` both default to `COLOR_DEFAULT`, and `m_highlight_symbol` defaults to `""`. Always set at least one of `.highlight_symbol(...)`, `.highlight_fg(...)`, or `.highlight_bg(...)` so the user can see which row is selected.

<!-- TODO(verify): Should ListState::handle_event() check !key.ctrl && !key.alt before matching 'j', 'k', 'g', and 'G' at src/list.cpp:131-150? -->

## See also

- [Tabs](/widgets/tabs/)
- [FocusManager](/api/focus-manager/)
- [Text](/widgets/text/)
