---
title: Box
description: Bordered rectangular container widget with configurable edge glyphs, title alignment, interior fill, and focus styling
sidebar:
  label: Box
  order: 1
---

`koto::Box` draws a rectangular border, optional interior fill, and an optional top-edge title into an `area` (Header: `koto/box.hpp`).

## Quick example

```cpp title="Rounded box" render="34x5"
Box()
    .border_style(BorderStyle::Rounded)
    .title("System Status")
    .title_align(TitleAlign::Center)
    .fg(Color::Cyan)
    .filled(true)
    .draw(frame, box_area);
```

```text frame="terminal" title="Output"
╭──────── System Status ─────────╮
│                                │
│                                │
│                                │
╰────────────────────────────────╯
```

## Behavior

- **Hit-buffer tagging**: When `.id(Id(...))` or `.tag(Id(...))` is set to a non-empty `id`, `Box::draw()` calls `frame.tag_area(area, m_tag_id)` across the entire `area` (even when `filled(false)`), and also passes `m_tag_id` to every border and interior `frame.set_cell()` call.
- **Border styles and edge toggles**: `BorderStyle` selects the corner and edge glyphs (`None` uses spaces `U' '`, `Single` uses `┌┐└┘─│`, `Double` uses `╔╗╚╝═║`, `Rounded` uses `╭╮╰╯─│`, `Thick` uses `┏┓┗┛━┃`). `Borders` (`{top, bottom, left, right}`, all `true` by default) controls individual edges; when `borders.left` or `borders.right` is `false`, the top and bottom corner cells draw the horizontal glyph instead of a corner glyph.
- **Interior fill**: When `filled(false)` (the default), interior cells (`!on_border`) are skipped and retain whatever was previously in the `frame` back buffer. When `filled(true)`, interior cells are overwritten with `Cell{U' ', m_fg, m_bg}`.
- **Title rendering**: The title is drawn on `area.row` only when `!m_title.empty()` and `text_width(m_title) + 4 <= area.width`. `TitleAlign::Left` starts at `area.col + 3`, `TitleAlign::Center` starts at `area.col + (area.width - title_width) / 2`, and `TitleAlign::Right` starts at `area.right() - title_width - 2`. Single space cells are placed immediately before (`title_col - 1`) and after (`title_col + title_width`) the title text.
- **Focus styling**: When `focused(true)` is set, the border color switches from `m_fg` to `m_focused_border_fg` (`Color::Cyan` by default), the title foreground defaults to `border_color` if `m_title_fg == COLOR_DEFAULT`, and the border style switches to `m_focused_border_style` only if `.focused_border_style(...)` was explicitly called.

<!-- src: include/koto/box.hpp:11-68 -->
<!-- src: src/box.cpp:99-302 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Box()` | Constructs a `Box` with default single border and centered title alignment. | See member defaults |
| `fg(Color fg)` | Sets the unfocused border foreground color (and interior space foreground when `filled(true)`). | `COLOR_DEFAULT` |
| `bg(Color bg)` | Sets the border background color and interior fill background color when `filled(true)`. | `COLOR_DEFAULT` |
| `title_fg(Color fg)` | Sets the title text foreground color (if `COLOR_DEFAULT` and `focused(true)`, uses `focused_border_fg`). | `COLOR_DEFAULT` |
| `title_bg(Color bg)` | Sets the background color of the title text and its two flanking space cells. | `COLOR_DEFAULT` |
| `filled(bool fill)` | Enables or disables filling interior cells with `Cell{U' ', m_fg, m_bg}`. | `false` |
| `border_style(BorderStyle style)` | Sets the border glyph style (`None`, `Single`, `Double`, `Rounded`, `Thick`). | `BorderStyle::Single` |
| `borders(Borders borders)` | Enables or disables individual edges (`{top, bottom, left, right}`). | `{true, true, true, true}` |
| `title(std::string title)` | Sets the top-border title string. | `""` |
| `title_align(TitleAlign align)` | Sets horizontal title alignment (`Left`, `Center`, `Right`). | `TitleAlign::Center` |
| `focused(bool is_focused)` | Enables or disables focused border/title styling. | `false` |
| `focused_border_fg(Color fg)` | Sets the border foreground color used when `focused(true)`. | `Color::Cyan` |
| `focused_border_style(BorderStyle style)` | Sets an explicit border style to use when `focused(true)`. | `BorderStyle::Thick` (inactive unless called) |
| `id(Id tag_id)` | Sets the `id` token stamped into the `frame` `hit buffer`. | `Id::none()` |
| `tag(Id tag_id)` | Alias for `id(tag_id)`. | `Id::none()` |
| `draw(Frame& frame, Area area)` | Renders the box into `frame` within `area`. | None |

<!-- src: include/koto/box.hpp:32-68 -->

## Gotchas

- **There is no `.border_fg()` method**: Use `.fg(Color)` for the unfocused border color and `.focused_border_fg(Color)` for the focused border color.
- **Default `title_align` is `TitleAlign::Center`**: Unlike `List`, `Tabs`, and `TextArea` (which default to `TitleAlign::Left`), `Box` defaults to `TitleAlign::Center` (`include/koto/box.hpp:62`).
- **Narrow boxes omit the title**: If `text_width(title) + 4 > area.width`, `Box::draw()` does not truncate the title; it skips drawing the title entirely so the corner glyphs remain intact (`src/box.cpp:268`).
- **1x1 or 1-row areas**: `Box::draw()` only checks `if (!area.is_valid()) return;`. In an area with `height == 1` or `width == 1`, `on_top` and `on_bottom` (or `on_left` and `on_right`) are both true, so the top/left branch wins.

## See also

- [Fill](/widgets/fill/)
- [Rule](/widgets/rule/)
- [Layout](/api/layout/)
