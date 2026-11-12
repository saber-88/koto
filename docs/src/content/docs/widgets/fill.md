---
title: Fill and Clear
description: Utility widgets that fill an Area with a repeated character and colors or reset an Area to default blank cells
sidebar:
  label: Fill and Clear
  order: 3
---

`koto::Fill` fills a rectangular `area` with a repeated codepoint, foreground/background colors, and an optional `id` tag, while `koto::Clear` resets an `area` to spaces (`U' '`) with `COLOR_DEFAULT` colors (Header: `koto/fill.hpp`).

## Quick example

```cpp title="Fill and Clear" render="24x4"
Fill(Color::DarkGray)
    .fg(Color::Cyan)
    .char_code(U'.')
    .id(Id("backdrop"))
    .draw(frame, fill_area);
```

```text frame="terminal" title="Output"
........................
........................
........................
........................
```

## Behavior

- **`Fill::draw(frame, area)`**: Calls `frame.fill(area, m_ch, m_fg, m_bg, m_tag_id)` (`src/fill.cpp:35-38`), which writes `Cell{m_ch, m_fg, m_bg, 1, false}` to every coordinate in `area` and, if `m_tag_id` is not `Id::none()`, stamps `m_hit_buffer` and updates `m_tag_bounds`.
- **`Clear::draw(frame, area)`**: Calls `frame.fill(area, U' ', COLOR_DEFAULT, COLOR_DEFAULT, m_tag_id)` (`src/fill.cpp:46-49`), resetting every cell in `area` to a default blank space and optionally tagging the region with `m_tag_id`.

<!-- src: include/koto/fill.hpp:14-45 -->
<!-- src: src/fill.cpp:7-50 -->

## Methods

### `Fill` methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Fill()` | Default-constructs a `Fill` widget with `U' '` and `COLOR_DEFAULT` colors. | `m_ch = U' '`, `m_fg = COLOR_DEFAULT`, `m_bg = COLOR_DEFAULT` |
| `Fill(Color bg)` | Explicit constructor setting `m_bg = bg.value`. | `m_ch = U' '`, `m_fg = COLOR_DEFAULT` |
| `bg(Color color)` | Sets the fill background color. | `COLOR_DEFAULT` |
| `fg(Color color)` | Sets the fill character foreground color. | `COLOR_DEFAULT` |
| `char_code(char32_t ch)` | Sets the codepoint written to each cell. | `U' '` |
| `id(Id id)` | Sets the `id` token stamped into the `hit buffer` across `area`. | `Id::none()` |
| `draw(Frame& frame, Area area)` | Fills `area` in `frame`. | None |

### `Clear` methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Clear()` | Default-constructs a `Clear` widget. | `m_tag_id = Id::none()` |
| `id(Id id)` | Sets the `id` token stamped into the `hit buffer` across `area`. | `Id::none()` |
| `draw(Frame& frame, Area area)` | Resets `area` in `frame` to `U' '` with `COLOR_DEFAULT` foreground and background. | None |

<!-- src: include/koto/fill.hpp:14-45 -->

## Gotchas

- **`Fill::char_code` uses width 1 in `Frame::fill`**: `Frame::fill(area, m_ch, m_fg, m_bg, m_tag_id)` constructs `Cell{ch, fg, bg, 1, false}` (`src/frame.cpp:167-170`), which `Frame::set_cell` promotes to width 2 if `codepoint_width(ch) == 2`. Avoid passing a 2-column CJK or emoji codepoint to `Fill::char_code`, because `Frame::fill` steps `col` by 1 across every column and each column will overwrite the continuation cell of the previous column.

## See also

- [Box](/widgets/box/)
- [Frame](/api/frame/)
- [IDs and Hit Buffer Concept](/concepts/ids-hit-buffer/)
