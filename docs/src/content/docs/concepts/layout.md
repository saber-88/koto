---
title: Declarative Layout and Constraints
description: How Area, Constraint, LayoutResult, and Grid partition terminal space with zero heap allocation for up to 16 segments
sidebar:
  label: Layout
  order: 6
---

## The idea in plain words

Terminal windows can be resized at any time. Instead of hardcoding `(row, col)` coordinates, you start from `frame.area()` and subdivide rectangles vertically, horizontally, or into a 2D `Grid` using `Constraint::fixed(n)`, `Constraint::percent(n)`, and `Constraint::fill()`.

## How koto implements it

1. **Two-pass 1D constraint solver (`split_1d`)**: Both `Layout::vertical` and `Layout::horizontal` call `split_1d` (`src/layout.cpp:8-70`):
   - **Pass 1**: Sums `c.value` for `Constraint::Type::Fixed` and `(total_size * c.value) / 100` for `Constraint::Type::Percent` into `used`, and counts `Constraint::Type::Fill` entries in `fill_count`.
   - **Remainder calculation**: Computes `remaining = std::max(0, total_size - used)`, `fill_size = remaining / fill_count`, and `fill_remainder = remaining % fill_count`.
   - **Pass 2**: Assigns each segment its size (giving the first `fill_remainder` fill segments `fill_size + 1` and the rest `fill_size`), clamps `size = std::min(size, std::max(0, (origin + total_size) - current_pos))`, appends the `Area` to `LayoutResult`, and advances `current_pos += size`.
2. **Small Buffer Optimization in `LayoutResult`**: `LayoutResult` embeds `std::array<Area, 16> m_stack` (`include/koto/layout.hpp:112, 180`). As long as a layout split has `<= 16` constraints, `push_back` writes solely into `m_stack` with zero heap allocation. Only when a 17th constraint is pushed does `LayoutResult` copy `m_stack` into `std::vector<Area> m_heap` (`include/koto/layout.hpp:124-129`).
3. **Compile-time array overloads for structured bindings**: When you pass a braced array literal `Layout::vertical(area, {Constraint::fixed(3), Constraint::fill()})`, C++ deduces `template <size_t N> std::array<Area, N> vertical(Area, const Constraint (&)[N])` (`include/koto/layout.hpp:249-258`), allowing C++17 structured bindings (`auto [top, bottom] = ...`).

<!-- src: include/koto/layout.hpp:95-305 -->
<!-- src: src/layout.cpp:8-167 -->

```cpp
auto [top_bar, main_pane, status_bar] = Layout::vertical(frame.area(), {
    Constraint::fixed(3),
    Constraint::fill(),
    Constraint::fixed(1),
});
static_cast<void>(top_bar);
static_cast<void>(main_pane);
static_cast<void>(status_bar);
```

## Consequences for your code

- **Use `area.inset(1, 1, 1, 1)` inside bordered boxes**: A `Box` with a visible border occupies 1 row on top and bottom and 1 column on left and right. Call `box_area.inset(1, 1, 1, 1)` to get the interior `area` for child widgets.
- **Note that `Layout::grid` allocates a `std::vector<Area>`**: While 1D `Layout::vertical` and `Layout::horizontal` are zero-allocation up to 16 constraints, `Grid` stores its cells in `std::vector<Area> m_cells` (`include/koto/layout.hpp:237`).

## See also

- [Layout API](/api/layout/)
- [Layout Recipes Guide](/guides/layout-recipes/)
- [Vbox](/widgets/vbox/)
- [Hbox](/widgets/hbox/)
