---
title: Frames and the Differential Renderer
description: How Frame diffs front and back Cell buffers, manages 2-column continuation cells, and emits synchronized ANSI updates
sidebar:
  label: Frames and Diff Renderer
  order: 2
---

## The idea in plain words

Writing the entire terminal screen to `STDOUT_FILENO` on every frame causes visible flicker and wastes bandwidth over SSH or terminal multiplexers. `koto::Frame` keeps two cell grids in memory—`m_back_buffer` (what your widgets draw this frame) and `m_front_buffer` (what is currently on the terminal screen)—and emits ANSI escape sequences only for cells that changed.

## How koto implements it

1. **Initialization and resize invalidation**: `Frame` initializes `m_front_buffer` with sentinel `Cell{U'ÿ'}` and `m_back_buffer` with default `Cell{}` (`U' '`) (`src/frame.cpp:23-24, 452-456`). Because `U'ÿ'` differs from a blank space, the first `render()` call (and the first `render()` after `handle_resize()`) paints every cell on the screen.
2. **Wide-character continuation cells**: When `Frame::set_cell(row, col, c, tag_id)` writes a width-2 character (such as a CJK ideograph or emoji), it places `Cell::make_continuation()` at `(row, col + 1)`. If `col + 1 >= m_cols`, `set_cell` replaces `c` with `Cell{U' '}` of width `1` so the terminal never wraps mid-glyph. Overwriting either half of a width-2 character clears the companion cell to a width-1 space (`src/frame.cpp:73-125`).
3. **Dirty scan and early return**: `Frame::render(bool flush)` scans `m_back_buffer` against `m_front_buffer` (ignoring cells where `back.continuation == true`) and checks whether cursor visibility or position changed. If neither changed, `render()` returns immediately with `m_output` empty (`src/frame.cpp:335-353`).
4. **Synchronized output and SGR caching**: When dirty cells exist, `render()` appends `[?2026h` (DEC Mode 2026 begin synchronized update), iterates rows and columns, emits `[r;cH` cursor moves (skipping cursor moves only when advancing sequentially across single-width ASCII `0x20..0x7E` cells), emits SGR attribute/color diffs against `m_cur_attrs`, `m_cur_fg`, and `m_cur_bg`, appends `c.grapheme.view()`, updates `m_front_buffer`, positions or hides the hardware cursor (`[?25h` / `[?25l`), appends `[?2026l`, and flushes `m_output` in a single `write_all(STDOUT_FILENO, m_output)` call (`src/frame.cpp:363-439`).

<!-- src: src/frame.cpp:67-141 -->
<!-- src: src/frame.cpp:200-440 -->

```cpp
Frame local_frame(2, 10);
local_frame.set_cell(0, 0, Cell{U'A', Color::Cyan, Color::Default});
local_frame.render(false);
// First render emits dirty cell diff; second identical render emits 0 bytes
local_frame.render(false);
```

## Consequences for your code

- **Use `set_cell` or `fill` when writing wide characters**: Indexing `frame[row, col]` directly bypasses continuation-cell maintenance in `set_cell()`.
- **Headless testing requires no TTY**: Constructing `Frame(rows, cols)` or calling `frame.render(false)` populates `frame.raw_output()` and `frame.to_plain_text()` in memory without touching `STDOUT_FILENO`.

## See also

- [Frame](/api/frame/)
- [Cell and Color](/api/cell-color/)
- [Rendering Pipeline Architecture](/architecture/rendering-pipeline/)
