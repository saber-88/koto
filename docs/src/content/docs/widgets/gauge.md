---
title: Gauge
description: Horizontal progress bar widget with 1/8th sub-block fractional resolution and background-preserving overlay label
sidebar:
  label: Gauge
  order: 4
---

`koto::Gauge` renders a horizontal progress bar across an `area` with 1/8th sub-cell fractional block characters and an optional centered text label (Header: `koto/gauge.hpp`).

## Quick example

```cpp title="Progress gauge" render="30x1"
Gauge()
    .percent(65)
    .label("65%")
    .filled_fg(Color::Emerald)
    .empty_fg(Color::DarkGray)
    .label_fg(Color::White)
    .draw(frame, gauge_area);
```

```text frame="terminal" title="Output"
█████████████65%███▌
```

## Behavior

- **Ratio clamping**: `ratio(float r)` clamps `r` to `[0.0f, 1.0f]` via `std::clamp(r, 0.0f, 1.0f)`. `percent(int p)` calls `ratio(static_cast<float>(p) / 100.0f)`.
- **Sub-cell calculation**: `Gauge::draw()` computes `total_subcells = area.width * 8` and `filled_subcells = std::clamp(static_cast<int>(std::lround(m_ratio * static_cast<float>(total_subcells))), 0, total_subcells)`. Each row in `area` renders:
  - `full_cells = filled_subcells / 8` columns using `{m_filled_ch, m_filled_fg, m_filled_bg}`.
  - Up to 1 fractional transition column at `col_idx == full_cells` when `remainder = filled_subcells % 8 > 0` and `m_fractional == true`, using `FRACTIONAL_BLOCKS[remainder]` (`U'▏'`, `U'▎'`, `U'▍'`, `U'▌'`, `U'▋'`, `U'▊'`, `U'▉'`) with foreground `m_filled_fg` and background `m_empty_bg`.
  - Remaining columns using `{m_empty_ch, m_empty_fg, m_empty_bg}`.
- **Centered label overlay**: When `!m_label.empty()` and `text_width(m_label) <= area.width`, the label is drawn on row `area.row + (area.height - 1) / 2` starting at `area.col + (area.width - label_w) / 2`. For each `Grapheme` in `m_label`, `Gauge` reads `Cell existing = frame.get_cell(label_row, c)` and writes `Cell{g, m_label_fg, existing.bg}`, preserving the underlying bar cell's background color.

<!-- src: include/koto/gauge.hpp:11-49 -->
<!-- src: src/gauge.cpp:10-153 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Gauge()` | Default-constructs a `Gauge` at `0.0f` ratio with fractional blocks enabled. | `m_ratio = 0.0f` |
| `ratio(float r)` | Sets progress ratio clamped to `[0.0f, 1.0f]`. | `0.0f` |
| `percent(int p)` | Calls `ratio(static_cast<float>(p) / 100.0f)`. | `0` |
| `label(std::string_view label)` | Sets the centered overlay label string. | `""` |
| `filled_char(char32_t ch)` | Sets the full-cell fill character and sets `m_fractional = false`. | `U'█'` |
| `empty_char(char32_t ch)` | Sets the unfilled cell character. | `U' '` |
| `use_fractional_blocks(bool enable)` | Enables or disables 1/8th sub-block rendering on the boundary cell. | `true` |
| `fg(Color fg)` | Alias for `filled_fg(fg)`. | `Color::Emerald` |
| `bg(Color bg)` | Alias for `empty_bg(bg)`. | `COLOR_DEFAULT` |
| `filled_fg(Color fg)` | Sets the foreground color of filled and fractional cells. | `Color::Emerald` |
| `filled_bg(Color bg)` | Sets the background color of full filled cells. | `COLOR_DEFAULT` |
| `empty_fg(Color fg)` | Sets the foreground color of empty cells. | `Color::DarkGray` |
| `empty_bg(Color bg)` | Sets the background color of empty cells and the fractional transition cell. | `COLOR_DEFAULT` |
| `label_fg(Color fg)` | Sets the foreground color of the centered label. | `Color::White` |
| `draw(Frame& frame, Area area)` | Draws the gauge into `frame` within `area`. | None |

<!-- src: include/koto/gauge.hpp:11-48 -->
<!-- src: src/gauge.cpp:14-90 -->

## Gotchas

- **Calling `.filled_char()` disables fractional blocks**: `Gauge::filled_char(ch)` sets `m_fractional = false` (`src/gauge.cpp:36`). If you want both a custom `filled_char` and fractional blocks, call `.use_fractional_blocks(true)` after `.filled_char(ch)`.
- **Label visibility over solid blocks**: By default, filled cells use `m_filled_ch = U'█'` (foreground `Color::Emerald`, background `COLOR_DEFAULT`). When the label overwrites a filled cell, it replaces `U'█'` with the label character and preserves `existing.bg` (`COLOR_DEFAULT`). Set `.filled_bg(...)` and `.empty_bg(...)` if you want a continuous colored background behind the label text.
- **No constructor taking `double` or `float`**: Always construct `Gauge()` and chain `.ratio(0.5f)` or `.percent(50)`.

## See also

- [Sparkline](/widgets/sparkline/)
- [Box](/widgets/box/)
- [Cell and Color](/api/cell-color/)
