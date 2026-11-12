---
title: Layout Recipes
description: Practical patterns for headers, sidebars, centered modal boxes, and 2D grids using Layout and Constraint
sidebar:
  label: Layout Recipes
  order: 1
---

## Goal

Partition a terminal `frame` into a header, a 2-column sidebar-and-content body, a 2x2 dashboard `Grid`, a centered overlay `area`, and a 1-row footer using `Layout` and `Constraint`.

<!-- src: include/koto/layout.hpp:10-305 -->
<!-- src: src/layout.cpp:8-167 -->

## Complete working example

```cpp
#include <koto/koto.hpp>
#include <cstdio>

using namespace koto;

int main()
{
    App app;
    ListState nav({"Overview", "Nodes", "Alerts"});

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.is(Key::Escape)) {
            app.quit();
            return;
        }
        nav.handle_event(key);
    });

    app.on_render([&](Frame& frame) {
        auto [header, body, footer] = Layout::vertical(frame.area(), {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(1),
        });

        Box()
            .title("Cluster Monitor")
            .border_style(BorderStyle::Rounded)
            .fg(Color::Cyan)
            .draw(frame, header);

        auto [sidebar, workspace] = Layout::horizontal(body, {
            Constraint::percent(25),
            Constraint::fill(),
        });

        List(nav)
            .title("Menu")
            .border_style(BorderStyle::Single)
            .highlight_symbol("> ")
            .highlight_fg(Color::Emerald)
            .draw(frame, sidebar);

        Grid cards = Layout::grid_uniform(workspace, 2, 2);
        cards.for_each([&](size_t r, size_t c, const Area& cell_area) {
            Box()
                .title("Card")
                .border_style(BorderStyle::Single)
                .draw(frame, cell_area);
            Area inner = cell_area.inset(1, 1, 1, 1);
            if (r == 0 && c == 0) {
                Gauge().percent(75).label("75%").draw(frame, inner);
            } else {
                Text("Active").align(TextAlign::Center).draw(frame, inner);
            }
        });

        Text("Press j/k to navigate, q to quit")
            .fg(Color::DarkGray)
            .draw(frame, footer);
    });

    if (auto res = app.run(); !res) {
        std::fprintf(stderr, "Error: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return 1;
    }
    return 0;
}
```

## Step-by-step explanation

1. **Top-level vertical split**: `Layout::vertical(frame.area(), {Constraint::fixed(3), Constraint::fill(), Constraint::fixed(1)})` invokes the compile-time array template overload (`include/koto/layout.hpp:249`), returning `std::array<Area, 3>` unpacked via structured bindings into `header`, `body`, and `footer`.
2. **Sidebar and content horizontal split**: `Layout::horizontal(body, {Constraint::percent(25), Constraint::fill()})` allocates `(body.width * 25) / 100` columns to `sidebar` and the remaining columns to `workspace`.
3. **2D uniform grid**: `Layout::grid_uniform(workspace, 2, 2)` divides `workspace` into 4 equal cells (`include/koto/layout.hpp:302`). You can iterate cells with `cards.for_each(...)` or index them with C++23 2D subscripting `cards[r, c]`.
4. **Interior padding with `Area::inset`**: Because `Box` draws a 1-cell border on all four sides, `cell_area.inset(1, 1, 1, 1)` shrinks the rectangle by 1 row top/bottom and 1 column left/right before drawing child widgets.

<!-- src: include/koto/layout.hpp:23-34 -->
<!-- src: include/koto/layout.hpp:220-230 -->

## Common mistakes

- **Capitalizing `Constraint` factory methods**: The factory functions are lowercase `Constraint::fixed(n)`, `Constraint::percent(n)`, and `Constraint::fill()`. `Constraint::Type::Fixed` is the internal enum value, not a factory function (`include/koto/layout.hpp:97-103`).
- **Drawing child content directly into a bordered `Box` area**: If you pass `cell_area` instead of `cell_area.inset(1, 1, 1, 1)` to a child `Text` or `Gauge`, the child overwrites the top-left border of the `Box`.
- **Forgetting that `Percent` truncates via integer division**: `Constraint::percent(33)` computes `(total_size * 33) / 100`. Pair `Constraint::percent(...)` with `Constraint::fill()` on the last segment so rounding remainders never leave a gap at the right or bottom edge.

## See also

- [Layout API](/api/layout/)
- [Layout Concept](/concepts/layout/)
- [Vbox](/widgets/vbox/)
- [Hbox](/widgets/hbox/)
