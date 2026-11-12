---
title: Tabs
description: Horizontal tab bar widget supporting Bar, Pill, and Underline styles, keyboard navigation, and sub-tab mouse hit-testing
sidebar:
  label: Tabs
  order: 9
---

`koto::Tabs` renders a horizontal strip of tab titles from either a mutable `koto::TabsState` `state struct` or a `std::vector<std::string>` (Header: `koto/tabs.hpp`).

## Quick example

```cpp title="Bar style tabs" render="36x1"
TabsState tabs({"Overview", "Metrics", "Settings"}, 0);

Tabs(tabs)
    .style(TabsStyle::Bar)
    .active_fg(Color::Black)
    .active_bg(Color::Cyan)
    .draw(frame, tabs_area);
```

```text frame="terminal" title="Output"
 Overview │ Metrics │ Settings
```

## Behavior

- **Automatic hit-buffer tagging**: `Tabs::draw()` sets `effective_id = !m_id.is_none() ? m_id : (m_state ? Id(m_state) : Id::none())` and calls `frame.tag_area(area, effective_id)` when `effective_id` is not `Id::none()` (`src/tabs.cpp:293-297`).
- **Three visual styles (`TabsStyle`)**:
  - `TabsStyle::Bar`: Pads each title with `" "` on each side (`" Title "`) and draws `m_separator` (`"│"` by default) in `m_separator_fg` / `m_inactive_bg` between tabs.
  - `TabsStyle::Pill`: Wraps the active tab in `"[ Title ]"` and inactive tabs in `"  Title  "`, separated by a single `" "` space.
  - `TabsStyle::Underline`: Pads each title as `" Title "` on `inner.row` separated by `" "`, and if `inner.height >= 2`, draws an underline bar on `inner.row + 1` using `U'━'` in `m_active_bg` under the active tab and `U'─'` in `m_separator_fg` under all other columns (`src/tabs.cpp:431-445`).
- **Sub-tab mouse geometry (`rendered_tabs`)**: When bound to a `TabsState`, `Tabs::draw()` populates `m_state->rendered_tabs` with a `TabRect{start_col, end_col, inner.row, inner.bottom()}` for each visible tab so `TabsState::handle_mouse()` can determine which tab was clicked (`src/tabs.cpp:396-404`).

<!-- src: include/koto/tabs.hpp:19-102 -->
<!-- src: src/tabs.cpp:20-446 -->

## Keys and mouse

Handled by `TabsState::handle_event(const KeyEvent&)` and `TabsState::handle_mouse(const MouseEvent&)`:

| Input | Condition | Action |
| :--- | :--- | :--- |
| `key.is(Key::Right)` or `key.ch == 'l'` | `focused && !titles.empty()` | Calls `select_next()` (wraps circularly to `0` after last tab) and returns `true`. |
| `key.is(Key::Left)` or `key.ch == 'h'` | `focused && !titles.empty()` | Calls `select_prev()` (wraps circularly to `titles.size() - 1` from `0`) and returns `true`. |
| `key.is(Key::Home)` | `focused && !titles.empty()` | Calls `select_first()` (`selected = 0`) and returns `true`. |
| `key.is(Key::End)` | `focused && !titles.empty()` | Calls `select_last()` (`selected = titles.size() - 1`) and returns `true`. |
| `'1'` .. `'9'` (`key.ch >= '1' && key.ch <= '9'`) | `focused && !titles.empty() && !key.ctrl && !key.alt` and `(key.ch - '1') < titles.size()` | Calls `set_selected(static_cast<size_t>(key.ch - '1'))` and returns `true`. |
| `MouseButton::WheelUp` / `MouseButton::WheelDown` | `!titles.empty()` | Calls `select_prev()` / `select_next()` and returns `true`. |
| `MouseAction::Press` + `MouseButton::Left` | `mouse.row` and `mouse.col` fall inside `rendered_tabs[i]` | Calls `set_selected(i)` and returns `true`. |

<!-- src: src/tabs.cpp:93-164 -->

## State

`koto::TabsState` satisfies `FocusableState` and `HasMouseHandler`:

| Member | Type / Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `titles` | `std::vector<std::string>` | Tab label strings. | `{}` |
| `selected` | `size_t` | 0-based active tab index. | `0` |
| `focused` | `bool` | Whether `handle_event` processes keyboard input and `Tabs` draws a focused border. | `true` |
| `on_change` | `std::function<void(size_t, const std::string&)>` | Callback fired whenever `select_next`, `select_prev`, `select_first`, `select_last`, or `set_selected` changes or sets selection. | `nullptr` |
| `rendered_tabs` | `std::vector<TabRect>` | Per-tab hit rectangles populated by `Tabs::draw()`. | `{}` |
| `TabsState()` / `TabsState(titles, selected)` | Constructors | Constructs empty or initialized state (`selected` clamped to `titles.size() - 1`). | `initial_selected = 0` |
| `select_next()` / `select_prev()` / `select_first()` / `select_last()` / `set_selected(index)` | `void` | Updates `selected` (`set_selected` ignores `index >= titles.size()`) and invokes `on_change`. | None |
| `selected_title()` | `std::optional<std::string_view> selected_title() const` | Returns active tab title view, or `std::nullopt` if empty. | `std::nullopt` |
| `selected_title_ptr()` | `const std::string* selected_title_ptr() const` | Returns pointer to active tab title string, or `nullptr` if empty. | `nullptr` |
| `handle_event(key)` / `handle_mouse(mouse)` | `bool` | Handles keyboard or mouse input and returns `true` if consumed. | None |

<!-- src: include/koto/tabs.hpp:26-55 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Tabs()` | Default-constructs a stateless `Tabs` widget. | `m_state = nullptr` |
| `Tabs(TabsState& state)` | Explicit constructor binding `Tabs` to `state`. | None |
| `Tabs(std::vector<std::string> titles)` | Explicit constructor for stateless rendering with `titles`. | `m_selected = 0` |
| `selected(size_t index)` | Sets active tab index (and calls `m_state->set_selected(index)` if bound to a `TabsState`). | `0` |
| `style(TabsStyle s)` | Sets visual style (`TabsStyle::Bar`, `TabsStyle::Pill`, `TabsStyle::Underline`). | `TabsStyle::Bar` |
| `separator(std::string_view sep)` | Sets separator string drawn between tabs in `TabsStyle::Bar`. | `"│"` |
| `separator_fg(Color color)` | Sets foreground color of `Bar` separators and `Underline` inactive rule (`─`). | `Color::DarkGray` |
| `active_fg(Color color)` | Sets foreground color of the active tab label. | `Color::Black` |
| `active_bg(Color color)` | Sets background color of the active tab label (and foreground of the active `━` underline in `TabsStyle::Underline`). | `Color::Cyan` |
| `inactive_fg(Color color)` | Sets foreground color of inactive tab labels. | `Color::White` |
| `inactive_bg(Color color)` | Sets background color of inactive tab labels, separators, and underline row. | `COLOR_DEFAULT` |
| `border_style(BorderStyle border)` | Sets optional outer border style (`None` draws no border). | `BorderStyle::None` |
| `title(std::string_view title)` | Sets outer box border title (used when `border_style != BorderStyle::None`). | `""` |
| `title_align(TitleAlign align)` | Sets outer box border title alignment. | `TitleAlign::Left` |
| `focused(bool is_focused)` | Overrides `m_state->focused` for border highlighting. | Uses `m_state->focused` |
| `focused_border_fg(Color color)` | Sets outer border color when focused. | `Color::Cyan` |
| `id(Id tag_id)` | Overrides automatic `Id(m_state)` hit-buffer tag. | `Id::none()` |
| `draw(Frame& frame, Area area)` | Renders the tab bar into `frame` within `area`. | None |

<!-- src: include/koto/tabs.hpp:57-102 -->

## Gotchas

- **`TabsStyle::Underline` requires at least 2 inner rows**: The underline bar (`━` / `─`) is drawn on `inner.row + 1` only when `inner.height >= 2` (`src/tabs.cpp:431`). If you also enable `.border_style(...)`, the widget needs `area.height >= 4` (`2` border rows + `2` inner rows).
- **In `TabsStyle::Underline`, the active underline color comes from `active_bg`**: On row 1 of `TabsStyle::Underline`, the active tab's `U'━'` glyph is drawn with foreground `m_active_bg` (`Color::Cyan` by default), while the active tab text on row 0 uses `m_active_fg` (`Color::Black` by default) on background `m_active_bg` (`src/tabs.cpp:367-368, 438`).

<!-- TODO(verify): Should TabsState::handle_event() check !key.ctrl && !key.alt before matching 'l' and 'h' at src/tabs.cpp:100-109, matching the check on '1'..'9' at line 122? -->

## See also

- [List](/widgets/list/)
- [FocusManager](/api/focus-manager/)
