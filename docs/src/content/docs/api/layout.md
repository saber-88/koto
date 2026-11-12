---
title: Layout, Area, Constraint, and Grid
description: Rectangular Area geometry, Fixed/Percent/Fill constraints, 16-element stack LayoutResult, and 2D Grid partitioning
sidebar:
  label: Layout
  order: 8
---

## Purpose and header

`koto/layout.hpp` provides `Area` rectangle arithmetic, `Constraint` sizing rules, the 16-element stack-buffered `LayoutResult` container, the 2D `Grid` container, and the `Layout` namespace functions (`vertical`, `horizontal`, `center`, `grid`, `grid_uniform`, `grid_fixed`).
Header: `koto/layout.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/layout.hpp:1-306 -->

## Types and members

### `Area` and `Constraint`

| Symbol | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `Area::row`, `col`, `width`, `height` | `int` fields | 0-based top-left coordinate `(row, col)` and dimensions `(width, height)`. | `0, 0, 0, 0` |
| `Area(r, c, w, h)` | `Area(int r, int c, int w, int h)` | Constructs an `Area` at row `r`, column `c`, width `w`, height `h`. | `0, 0, 0, 0` |
| `Area::right()` / `bottom()` | `int right() const` / `int bottom() const` | Returns `col + width` and `row + height`. | None |
| `Area::inset(top, left, bottom, right)` | `Area inset(int top, int left, int bottom, int right) const` | Shrinks area by the given margins, clamping resulting width and height to `>= 0`. | None |
| `Area::is_valid()` | `bool is_valid() const` | Returns `width > 0 && height > 0`. | `false` on `Area{}` |
| `Area::contains(r, c)` | `bool contains(int r, int c) const` | Returns `r >= row && r < bottom() && c >= col && c < right()`. | None |
| `Area::center_row()` / `center_col()` | `int center_row() const` / `int center_col() const` | Returns `row + height / 2` and `col + width / 2`. | None |
| `Area::union_with(other)` | `Area union_with(Area other) const` | Returns the bounding rectangle containing both valid areas (ignores invalid areas). | None |
| `Area::set_row` / `set_col` / `set_height` / `set_width` | `Area set_*(int) const` | Returns a copy of `*this` with the specified field replaced. | None |
| `Constraint::Type` | `enum class Type { Fixed, Fill, Percent }` | Constraint calculation mode. | None |
| `Constraint::fixed(n)` | `static Constraint fixed(int n)` | Allocates `n` cells (`n < 0` clamped to `0`). | None |
| `Constraint::percent(n)` | `static Constraint percent(int n)` | Allocates `(total_size * n) / 100` cells (`n < 0` clamped to `0`). | None |
| `Constraint::fill()` | `static Constraint fill()` | Shares remaining space after `Fixed` and `Percent` constraints equally. | None |

<!-- src: include/koto/layout.hpp:10-104 -->

### `LayoutResult`, `Grid`, and `Layout` functions

| Symbol | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `LayoutResult::InlineCapacity` | `static constexpr size_t InlineCapacity = 16` | Number of `Area` elements stored inline on the stack (`std::array<Area, 16>`) before spilling to `m_heap`. | `16` |
| `LayoutResult` accessors | `size()`, `empty()`, `operator[](idx)`, `at(idx)`, `begin()`, `end()`, `cbegin()`, `cend()`, `front()`, `back()`, `to_vector()` | Stack-or-heap container interface (`at(idx)` returns static `Area(0,0,0,0)` if out of bounds). | Empty |
| `Grid` | Class | 2D matrix of `Area`s supporting `rows()`, `cols()`, `size()`, `bounds()`, `at(r, c)`, `operator()(r, c)`, `operator[r, c]`, `operator[](r)[c]`, `for_each(func)`, and `cells()`. | `0 x 0` |
| `Layout::vertical(area, span)` | `LayoutResult vertical(Area area, std::span<const Constraint> constraints)` | Splits `area` top-to-bottom into `LayoutResult`. | None |
| `Layout::horizontal(area, span)` | `LayoutResult horizontal(Area area, std::span<const Constraint> constraints)` | Splits `area` left-to-right into `LayoutResult`. | None |
| `Layout::vertical<N>(area, arr)` | `template<size_t N> std::array<Area, N> vertical(Area area, const Constraint (&)[N])` (and `std::array<Constraint, N>`) | Compile-time array overloads returning `std::array<Area, N>` for C++17 structured bindings. | None |
| `Layout::horizontal<N>(area, arr)` | `template<size_t N> std::array<Area, N> horizontal(Area area, const Constraint (&)[N])` (and `std::array<Constraint, N>`) | Compile-time array overloads returning `std::array<Area, N>` for C++17 structured bindings. | None |
| `Layout::center(area, w, h)` | `Area center(Area area, int width, int height)` | Centers a `width x height` box inside `area`, clamping to `area.width` and `area.height`. | None |
| `Layout::grid(area, rows, cols)` | `Grid grid(Area area, std::span<const Constraint> row_constraints, std::span<const Constraint> col_constraints)` | Splits `area` vertically by `row_constraints` and each row horizontally by `col_constraints`. | None |
| `Layout::grid_uniform(area, r, c)` | `Grid grid_uniform(Area area, size_t rows, size_t cols)` | Builds a `rows x cols` `Grid` using `Constraint::fill()` for all rows and columns. | None |
| `Layout::grid_fixed(area, r, c, cw, ch)` | `Grid grid_fixed(Area area, size_t rows, size_t cols, int cell_width, int cell_height)` | Builds a `rows x cols` `Grid` using `Constraint::fixed(cell_height)` and `Constraint::fixed(cell_width)`. | None |

<!-- src: include/koto/layout.hpp:109-305 -->
<!-- src: src/layout.cpp:8-167 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

int main()
{
    Area screen(0, 0, 80, 24);

    auto [header, body, footer] = Layout::vertical(screen, {
        Constraint::fixed(3),
        Constraint::fill(),
        Constraint::fixed(1),
    });
    assert(header.height == 3 && body.height == 20 && footer.height == 1);

    Grid g = Layout::grid_uniform(body, 2, 2);
    assert((g[0, 0].width == 40 && g[0, 0].height == 10));
    return 0;
}
```

## Errors and edge cases

- **Remainder distribution in `Constraint::fill()`**: When `remaining` cells do not divide evenly by `fill_count`, the first `remaining % fill_count` fill segments each receive `+1` extra cell (`src/layout.cpp:32-49`).
- **Over-constrained clamping**: Each segment size is clamped to `std::max(0, (origin + total_size) - current_pos)` (`src/layout.cpp:55-56`). If earlier `Fixed` or `Percent` constraints exhaust the parent dimension, subsequent segments receive `width = 0` or `height = 0` and never overflow `area.right()` or `area.bottom()`.
- **Structured bindings use the array template overload**: `auto [a, b] = Layout::horizontal(area, {Constraint::fixed(10), Constraint::fill()});` binds to the `const Constraint (&constraints)[N]` overload returning `std::array<Area, N>`. `LayoutResult` itself does not specialize `std::tuple_size` or `std::tuple_element`.

<!-- src: include/koto/layout.hpp:247-294 -->
<!-- src: src/layout.cpp:31-67 -->

## See also

- [Layout Concept](/concepts/layout/)
- [Layout Recipes Guide](/guides/layout-recipes/)
- [Vbox](/widgets/vbox/)
- [Hbox](/widgets/hbox/)
