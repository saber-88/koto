---
title: Vbox
description: Vertical container widget that owns child widgets and partitions its Area top-to-bottom using Constraints
sidebar:
  label: Vbox
  order: 13
---

`koto::Vbox` is a composite `Widget` container that owns child widgets via `std::unique_ptr<Widget>` and draws them top-to-bottom using `Layout::vertical` (Header: `koto/vbox.hpp`).

## Quick example

```cpp title="Vertical box container" render="30x6"
Vbox col;
col.add<Box>(Constraint::fixed(3)).title("Top").border_style(BorderStyle::Rounded);
col.add<Box>(Constraint::fill()).title("Bottom").border_style(BorderStyle::Single);
col.draw(frame, vbox_area);
```

```text frame="terminal" title="Output"
╭─────────── Top ────────────╮
│                            │
╰────────────────────────────╯
┌────────── Bottom ──────────┐
│                            │
└────────────────────────────┘
```

## Behavior

- **Child ownership and fluent chaining**: `Vbox::add<T>(Constraint c, Args&&... args)` allocates a `std::unique_ptr<T>(std::forward<Args>(args)...)`, appends `{std::move(widget), c}` to `m_children`, and returns a `T&` reference so you can chain configuration calls on the newly added child (`include/koto/vbox.hpp:15-25`).
- **Nesting `Hbox` and `Vbox`**: Because both `Vbox` and `Hbox` derive from `koto::Widget`, you can nest an `Hbox` inside a `Vbox` via `auto& row = vbox.add<Hbox>(Constraint::fill());` and populate `row.add<...>(...)`.
- **Stack constraint buffer for `<= 16` children**: When `m_children.size() <= LayoutResult::InlineCapacity` (`16`), `Vbox::draw()` copies the child constraints into a stack `std::array<Constraint, 16>`, calls `Layout::vertical(area, std::span<const Constraint>(...))`, and draws each child into `rect[i]` (`src/vbox.cpp:16-28`).

<!-- src: include/koto/vbox.hpp:11-39 -->
<!-- src: src/vbox.cpp:12-43 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Vbox()` | Default-constructs an empty vertical container (move-only due to `std::unique_ptr<Widget>`). | `m_children` empty |
| `add<T>(Constraint c, Args&&... args)` | Constructs child widget `T(args...)` with vertical constraint `c`, stores it, and returns `T&`. | None |
| `draw(Frame& frame, Area area)` | Partitions `area` vertically and draws each child into its corresponding sub-area. | None |

<!-- src: include/koto/vbox.hpp:11-28 -->

## Gotchas

- **Per-child heap allocation in `add<T>`**: Calling `vbox.add<T>(...)` invokes `std::make_unique<T>(...)`. To avoid heap allocations inside `on_render`, either construct a static `Vbox` when children do not depend on per-frame state or use `Layout::vertical(area, {...})` with stack-allocated widgets.

## See also

- [Hbox](/widgets/hbox/)
- [Layout](/api/layout/)
- [Layout Recipes Guide](/guides/layout-recipes/)
