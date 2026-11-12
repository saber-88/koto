---
title: IDs and the Spatial Hit Buffer
description: How 32-bit FNV-1a Id tokens tag Frame cells for O(1) mouse hit-testing and automatic bounding-box harvesting
sidebar:
  label: IDs and Hit Buffer
  order: 3
---

## The idea in plain words

Terminal emulators report mouse clicks as `(row, col)` coordinates, not widget references. Instead of maintaining a separate scene graph to intersect mouse coordinates against widget rectangles, `koto::Frame` stores a parallel `hit buffer` (`std::vector<int> m_hit_buffer`) of the exact same dimensions as the cell grid and records each widget's `id` as it draws.

## How koto implements it

1. **Universal `Id` tokens**: `koto::Id` (`include/koto/id.hpp:52`) hashes strings (`Id("save")`), indexed strings (`Id("item", i)`), 2D coordinates (`Id("cell", r, c)`), or object pointers (`Id(&state)`) into a 32-bit unsigned integer using `fnv1a_32`. If FNV-1a produces `0xFFFFFFFFu`, `fnv1a_32` remaps it to `0xFFFFFFFEu` so no valid hash collides with `TAG_NONE` (`-1`).
2. **Stamping the `hit buffer` and `m_tag_bounds`**:
   - At the start of each frame, `Frame::clear_back_buffer()` fills `m_hit_buffer` with `TAG_NONE` (`-1`) and clears `m_tag_bounds` (`src/frame.cpp:183-184`).
   - Whenever a widget calls `frame.set_cell(r, c, cell, tag_id)`, `frame.fill(area, cell, tag_id)`, `frame.tag_cell(r, c, tag_id)`, or `frame.tag_area(area, tag_id)` with `!tag_id.is_none()`, `Frame` writes `static_cast<int>(tag_id.value())` into `m_hit_buffer[r * m_cols + c]` and unions the cell or `area` into `m_tag_bounds[tag_id.value()]` (`src/frame.cpp:127-140, 464-507`).
   - Later draws overwrite earlier draws in `m_hit_buffer` (painter's algorithm), so a modal dialog drawn last automatically claims the hit buffer cells underneath it.
3. **`O(1)` lookup and geometry harvesting**:
   - `Frame::hit_at(row, col)` indexes `m_hit_buffer[row * m_cols + col]` in `O(1)` time (`src/frame.cpp:509-513`).
   - After `on_render` finishes, `App::draw_frame()` calls `FocusManager::sync_geometry(m_frame)`, which queries `frame.bounds_of(target.id)` (`O(1)` hash map lookup in `m_tag_bounds`) to populate each `FocusTarget::bounds` (`src/focus.cpp:382-395`).

<!-- src: include/koto/id.hpp:10-108 -->
<!-- src: src/frame.cpp:464-535 -->

```cpp
Box()
    .title("Sidebar")
    .id(Id("sidebar"))
    .draw(frame, sidebar_area);

Id hit = frame.hit_at(sidebar_area.row, sidebar_area.col);
static_cast<void>(hit);
```

## Consequences for your code

- **Stateful widgets tag themselves automatically**: `List`, `Tabs`, `TextInput`, and `TextArea` automatically tag their drawn `area` with `Id(&m_state)` (unless overridden via `.id(Id(...))`). You do not need to call `.id(...)` on them to use mouse clicking or 2D spatial navigation with `FocusManager`.
- **Stateless `Box`, `Button`, `Fill`, and `Clear` require `.id(Id(...))`**: To hit-test or focus a `Box` or `Button`, pass `.id(Id("my_id"))` when drawing it.
- **When using `FocusManager::scope`, tag with `fm.id(...)`**: Inside a normal or modal `scope`, `FocusManager` prefixes target IDs with the active scope hash. Pass `.id(fm.id("btn"))` when drawing a stateless `Box` so the `hit buffer` tag matches the scoped `FocusTarget::id`.

<!-- src: src/list.cpp:313-314 -->
<!-- src: src/focus.cpp:779-791 -->

## See also

- [Id](/api/id/)
- [Frame](/api/frame/)
- [Mouse Guide](/guides/mouse/)
