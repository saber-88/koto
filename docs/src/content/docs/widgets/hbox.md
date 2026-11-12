---
title: Hbox
description: Horizontal container widget that owns child widgets and partitions its Area left-to-right using Constraints
sidebar:
  label: Hbox
  order: 5
---

`koto::Hbox` is a composite `Widget` container that owns child widgets on the heap via `std::unique_ptr<Widget>` and draws them left-to-right using `Layout::horizontal` (Header: `koto/hbox.hpp`).

## Quick example

```cpp title="Horizontal box container" render="34x3"
Hbox row;
row.add<Box>(Constraint::fixed(12)).title("Left").border_style(BorderStyle::Single);
row.add<Box>(Constraint::fill()).title("Right").border_style(BorderStyle::Rounded);
row.draw(frame, hbox_area);
```

```text frame="terminal" title="Output"
┌── Left ──┐╭────── Right ───────╮
│          ││                    │
└──────────┘╰────────────────────╯
```

## Behavior

- **Child ownership and fluent configuration**: `Hbox::add<T>(Constraint c, Args&&... args)` constructs a `std::unique_ptr<T>(std::forward<Args>(args)...)`, appends `{std::move(widget), c}` to `m_children`, and returns `T&` so you can chain widget setter methods directly (`include/koto/hbox.hpp:15-25`).
- **Zero-heap layout path for `<= 16` children**: When `m_children.size() <= LayoutResult::InlineCapacity` (`16`), `Hbox::draw()` copies child constraints into a stack `std::array<Constraint, 16>`, calls `Layout::horizontal(area, std::span<const Constraint>(...))`, and invokes `m_children[i].widget->draw(frame, rect[i])`. When `m_children.size() > 16`, it allocates a `std::vector<Constraint>` before calling `Layout::horizontal` (`src/hbox.cpp:16-42`).

<!-- src: include/koto/hbox.hpp:11-39 -->
<!-- src: src/hbox.cpp:12-43 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Hbox()` | Default-constructs an empty horizontal container (move-only due to `std::unique_ptr<Widget>`). | `m_children` empty |
| `add<T>(Constraint c, Args&&... args)` | Constructs child widget `T(args...)` with horizontal constraint `c`, stores it, and returns `T&`. | None |
| `draw(Frame& frame, Area area)` | Partitions `area` horizontally and draws each child into its corresponding sub-area. | None |

<!-- src: include/koto/hbox.hpp:11-28 -->

## Gotchas

- **Heap allocation per child**: Although `Hbox::draw()` avoids heap allocation for the `Constraint` array when `m_children.size() <= 16`, every call to `Hbox::add<T>(...)` allocates the child `Widget` on the heap via `std::make_unique<T>(...)`. In hot 60 FPS `on_render` callbacks, partitioning areas directly with `Layout::horizontal(area, {...})` and drawing stack-allocated widgets avoids per-frame `std::make_unique` allocations.
- **Move-only container**: Because `m_children` holds `std::unique_ptr<Widget>`, `Hbox` cannot be copied.

## See also

- [Vbox](/widgets/vbox/)
- [Layout](/api/layout/)
- [Layout Recipes Guide](/guides/layout-recipes/)
