---
title: Sparkline
description: Single-row time-series bar graph widget rendered with 8 vertical Unicode sub-block characters
sidebar:
  label: Sparkline
  order: 8
---

`koto::Sparkline` renders a sequence of `uint64_t` values as a compact single-row vertical bar chart using 8 Unicode block characters (Header: `koto/sparkline.hpp`).

## Quick example

```cpp title="Sparkline trend" render="24x1"
std::vector<uint64_t> values = {1, 3, 2, 5, 8, 6, 7, 4, 9, 10};
Sparkline()
    .data(values)
    .fg(Color::Cyan)
    .draw(frame, spark_area);
```

```text frame="terminal" title="Output"
               ▃▂▅▇▅▆▄██
```

## Behavior

- **Bottom-row drawing and upper-row clearing**: In a multi-row `area`, `Sparkline::draw()` renders bars on the bottom row `target_row = area.bottom() - 1` and clears all rows above `target_row` (`area.row .. target_row - 1`) with `Cell{U' ', m_fg, m_bg}` (`src/sparkline.cpp:60-69`).
- **Auto-scaling vs. fixed ceiling**: If `m_max == 0` (the default), `effective_max` is computed as the maximum element in `m_data` (clamped to at least `1` if all elements are `0`). Otherwise `effective_max = m_max` (`src/sparkline.cpp:80-88`).
- **Windowing and right alignment**: If `m_data.size() > area.width`, only the last `area.width` elements are displayed (`start_idx = num_points - available_cols`). If `m_data.size() < area.width` and `m_align_right == true` (the default), leading columns on the left are filled with spaces so the data hugs the right edge (`src/sparkline.cpp:93-106`).
- **8-level quantization**: For each point `val`:
  - If `val == 0`, the cell is a space `U' '`.
  - If `val > 0`, `bar_level = std::min<uint64_t>(7, (val * 8) / effective_max)` indexes into `SPARKLINE_BARS = {U'▁', U'▂', U'▃', U'▄', U'▅', U'▆', U'▇', U'█'}` (`src/sparkline.cpp:8-10`, `src/sparkline.cpp:121-130`).

<!-- src: include/koto/sparkline.hpp:12-34 -->
<!-- src: src/sparkline.cpp:8-135 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Sparkline()` | Constructs an empty `Sparkline`. | `m_max = 0`, `m_align_right = true`, `m_fg = Color::Cyan`, `m_bg = COLOR_DEFAULT` |
| `data(std::span<const uint64_t> values)` | Binds a non-owning view over `values` and clears `m_owned_data`. | Empty span |
| `data(const std::vector<uint64_t>& values)` | Copies `values` into `m_owned_data` and points `m_data` at the copy. | Empty vector |
| `max(uint64_t max_value)` | Sets a fixed scaling ceiling (`0` enables auto-scaling to the maximum element). | `0` |
| `align_right(bool right = true)` | When `true`, right-aligns shorter data series; when `false`, left-aligns them. | `true` |
| `fg(Color fg)` | Sets the foreground color of the sparkline bars. | `Color::Cyan` |
| `bg(Color bg)` | Sets the background color of the sparkline cells. | `COLOR_DEFAULT` |
| `draw(Frame& frame, Area area)` | Draws the sparkline into `frame` within `area`. | None |

<!-- src: include/koto/sparkline.hpp:12-34 -->

## Gotchas

- **`data(std::span<const uint64_t>)` vs. `data(const std::vector<uint64_t>&)`**: Passing a `std::vector<uint64_t>` directly matches the `const std::vector<uint64_t>&` overload, which copies the vector into `m_owned_data` (`src/sparkline.cpp:21-26`). To avoid copying a `std::vector<uint64_t> vec` on every frame, pass `std::span<const uint64_t>(vec)`.
- **Single-row bar height**: Even if `area.height > 1`, `Sparkline` only draws 1-row-tall bars on `area.bottom() - 1` and fills the rows above with blank spaces.

## See also

- [Gauge](/widgets/gauge/)
- [Performance Deep Dive](/deep-dives/performance/)
