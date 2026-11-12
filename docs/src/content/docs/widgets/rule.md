---
title: Rule
description: Horizontal divider line widget with Single, Double, Rounded, or Thick glyphs and an optional aligned title
sidebar:
  label: Rule
  order: 7
---

`koto::Rule` draws a horizontal divider line across the vertical center of an `area` with an optional space-padded title (Header: `koto/rule.hpp`).

## Quick example

```cpp title="Titled rule" render="34x1"
Rule("Section")
    .style(BorderStyle::Double)
    .title_align(TitleAlign::Center)
    .fg(Color::Cyan)
    .title_fg(Color::White)
    .draw(frame, rule_area);
```

```text frame="terminal" title="Output"
════════════ Section ═════════════
```

## Behavior

- **Vertical centering**: `Rule::draw()` draws on a single row at `row = area.row + (area.height - 1) / 2` (`src/rule.cpp:64`). Rows above and below `row` in `area` are untouched.
- **Line glyph selection**: `BorderStyle::Single` and `BorderStyle::Rounded` use `U'─'`, `BorderStyle::Double` uses `U'═'`, `BorderStyle::Thick` uses `U'━'`, and `BorderStyle::None` uses `U' '` (`src/rule.cpp:66-83`).
- **Title padding and alignment**: When `m_title` is non-empty, `Rule` constructs `padded = " " + m_title + " "` and measures `title_w = text_width(padded)`. If `title_w > area.width`, the title is skipped and the solid line remains intact. Otherwise:
  - `TitleAlign::Left` places `padded` at `area.col + 2` (clamped to `[area.col, area.right() - title_w]`).
  - `TitleAlign::Center` places `padded` at `area.col + (area.width - title_w) / 2`.
  - `TitleAlign::Right` places `padded` at `area.right() - 2 - title_w` (clamped to `[area.col, area.right() - title_w]`).
- **Title color fallback**: If `m_title_fg == COLOR_DEFAULT`, the title uses `m_fg`; if `m_title_bg == COLOR_DEFAULT`, the title uses `m_bg` (`src/rule.cpp:122-123`).

<!-- src: include/koto/rule.hpp:12-35 -->
<!-- src: src/rule.cpp:9-135 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Rule(std::string_view title = "")` | Constructs a `Rule` with an optional title string. | `title = ""` |
| `title(std::string_view title)` | Sets the embedded title text. | `""` |
| `title_align(TitleAlign align)` | Sets horizontal title alignment (`Left`, `Center`, `Right`). | `TitleAlign::Center` |
| `style(BorderStyle style)` | Sets the horizontal line glyph style (`Single`, `Rounded`, `Double`, `Thick`, `None`). | `BorderStyle::Single` |
| `fg(Color fg)` | Sets the line foreground color (and fallback title foreground). | `COLOR_DEFAULT` |
| `bg(Color bg)` | Sets the line background color (and fallback title background). | `COLOR_DEFAULT` |
| `title_fg(Color fg)` | Sets an explicit foreground color for the padded title. | `COLOR_DEFAULT` (falls back to `fg`) |
| `title_bg(Color bg)` | Sets an explicit background color for the padded title. | `COLOR_DEFAULT` (falls back to `bg`) |
| `draw(Frame& frame, Area area)` | Renders the horizontal rule into `frame` within `area`. | None |

<!-- src: include/koto/rule.hpp:12-35 -->

## Gotchas

- **Method name is `.style(...)`, not `.border_style(...)`**: Unlike `Box`, `Button`, `List`, `Tabs`, `TextArea`, and `TextInput`, `Rule` names its `BorderStyle` setter `.style(BorderStyle)` (`include/koto/rule.hpp:19`).
- **No `.id(...)` method**: `Rule` is a purely decorative divider and does not expose `.id(Id)`.

## See also

- [Box](/widgets/box/)
- [Text](/widgets/text/)
