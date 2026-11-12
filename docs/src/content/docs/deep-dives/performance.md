---
title: Performance and Memory Architecture
description: Source-verified analysis of the 64-byte Cell layout, 16-element LayoutResult stack buffer, and zero-dirty-cell frame skipping
sidebar:
  label: Performance
  order: 1
---

## 64-byte cache-line `Cell` layout

`koto::Cell` (`include/koto/cell.hpp:29-75`) is sized to exactly 64 bytes (`static_assert(sizeof(Cell) == 64, "Cell must be exactly 64 bytes (1 cache line)")`):

| Field | Type | Size (bytes) | Offset (bytes) |
| :--- | :--- | :--- | :--- |
| `grapheme.data` | `char[36]` | `36` | `0..35` |
| `grapheme.size` | `uint8_t` | `1` | `36` |
| `grapheme.width` | `uint8_t` | `1` | `37` |
| *(padding)* | — | `2` | `38..39` |
| `grapheme.base_cp` | `char32_t` | `4` | `40..43` |
| `ch` | `char32_t` | `4` | `44..47` |
| `fg` | `uint32_t` | `4` | `48..51` |
| `bg` | `uint32_t` | `4` | `52..55` |
| `width` | `uint8_t` | `1` | `56` |
| `continuation` | `bool` | `1` | `57` |
| `attrs` | `uint8_t` | `1` | `58` |
| `reserved` | `uint8_t[5]` | `5` | `59..63` |

Because `Grapheme` stores up to 35 UTF-8 bytes plus a null terminator inline (`char data[36]`), writing or diffing a `Cell` never allocates heap memory or chases an external string pointer.

<!-- src: include/koto/utf8.hpp:26-32 -->
<!-- src: include/koto/cell.hpp:29-75 -->

## Stack-allocated `LayoutResult` (Small Buffer Optimization)

`Layout::vertical` and `Layout::horizontal` return `koto::LayoutResult` (`include/koto/layout.hpp:109-183`), which holds `std::array<Area, 16> m_stack` (`256` bytes) alongside `std::vector<Area> m_heap`.
- When partitioning an `area` into `1` to `16` segments, `LayoutResult::push_back` writes exclusively into `m_stack` with zero heap allocations.
- Only when a 17th constraint is pushed (`m_size == InlineCapacity`) does `push_back` copy the 16 stack elements into `m_heap` (`include/koto/layout.hpp:124-129`).
- When you pass a C-array or braced initializer list of constraints (`auto [a, b] = Layout::vertical(area, {Constraint::fixed(3), Constraint::fill()})`), the template overload (`include/koto/layout.hpp:249-258`) copies the stack elements into `std::array<Area, N>`.

<!-- src: include/koto/layout.hpp:109-183, 247-294 -->

## Differential rendering and zero-byte clean frames

In `Frame::render(bool flush)` (`src/frame.cpp:331-440`):
1. **Clean-frame short-circuit**: Before emitting any escape sequences, `render()` scans `m_back_buffer` against `m_front_buffer` (skipping continuation cells) and checks if `m_cursor_visible`, `m_cursor_row`, or `m_cursor_col` changed. If `!any_dirty && !cursor_changed`, `render()` returns immediately without appending `[?2026h` or invoking `write_all()`.
2. **Pre-reserved output buffer**: When dirty cells exist, `m_output.reserve(static_cast<size_t>(m_rows * m_cols * 4))` reserves capacity upfront (`src/frame.cpp:360`), and all cursor moves, SGR attribute/color changes, and UTF-8 grapheme bytes are appended into `m_output` before a single `write_all(STDOUT_FILENO, m_output)` call.
3. **Sequential ASCII cursor elision**: `Frame::render()` tracks `cursor_row`, `cursor_col`, and `last_cell_was_ascii`. When consecutive dirty cells on the same row are single-width printable ASCII (`back.width == 1 && back.ch >= 0x20 && back.ch < 0x7F && back.grapheme.size == 1`), `move_cursor(r, c)` is skipped because the terminal's hardware cursor already advanced by 1 column (`src/frame.cpp:395-403`).

<!-- src: src/frame.cpp:331-440 -->

## Avoiding unnecessary allocations in application code

- **Pass `std::span<const uint64_t>` to `Sparkline::data`**: `Sparkline::data(const std::vector<uint64_t>&)` copies the vector into `m_owned_data`, whereas `Sparkline::data(std::span<const uint64_t>)` stores a non-owning view (`src/sparkline.cpp:14-26`).
- **Prefer `Layout::vertical` / `Layout::horizontal` over `Vbox` / `Hbox` in hot loops**: `Vbox::add<T>` and `Hbox::add<T>` allocate each child widget with `std::make_unique<T>` (`include/koto/vbox.hpp:18`, `include/koto/hbox.hpp:18`), whereas calling `Layout::vertical` / `Layout::horizontal` and drawing stack-constructed widgets (`Box().draw(...)`) performs no heap allocation for layout.

```cpp
std::array<uint64_t, 8> samples = {2, 4, 6, 8, 7, 5, 3, 9};
Sparkline()
    .data(std::span<const uint64_t>(samples))
    .draw(frame, chart_area);
```

## See also

- [Rendering Pipeline Architecture](/architecture/rendering-pipeline/)
- [Frames and Diff Renderer Concept](/concepts/frames-diff-renderer/)
- [Layout Concept](/concepts/layout/)
