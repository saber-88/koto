# Focus Management & Spatial Navigation

Deep architectural analysis of `libkoto`'s universal identity tokens (`koto::Id`), the `FocusManager` engine, 1D Tab rings, 2D spatial navigation heuristics, RAII modal scoping (`IdScope`), and $O(1)$ hardware-like hit-buffers.

---

## 1. Universal Identity Tokens (`koto::Id`)

In complex TUIs, widgets and focus targets require unique, lightweight identifiers. `koto::Id` provides a 32-bit universal token evaluated at compile time whenever possible using the **FNV-1a 32-bit hashing algorithm**:

```cpp
#include <koto/id.hpp>

// 1. From string literals (constexpr compile-time FNV-1a hash)
Id id1 = "username_input";

// 2. From raw memory addresses (pointer hashing for state structs)
Id id2 = &my_text_area_state;

// 3. From scoped enums
enum class Panel { Left, Right, Footer };
Id id3 = Panel::Left;

// 4. Multi-part / indexed tokens
Id row_item  = Id("row", index);
Id grid_cell = Id("matrix", row, col);
```

* **Footprint**: `sizeof(Id) == 4` (32 bits), fitting in a single CPU register.
* **Zero Runtime Cost**: String literals and enums are hashed at compile time via `constexpr`.

---

## 2. Navigation Policies

The `FocusManager` orchestrates keyboard focus and mouse click routing across interactive controls:

```cpp
FocusManager fm;
fm.navigation_policy(FocusNavPolicy::SpatialArrows);
```

| `FocusNavPolicy` | `Tab` / `Shift+Tab` | Arrow Keys (`Up`, `Down`, `Left`, `Right`) | Typical Use Case |
|:---|:---:|:---:|:---|
| **`TabOnly` (Default)** | Linear 1D cycle | Passed directly to focused widget | Text editors, forms, lists where arrows control caret/scroll. |
| **`SpatialArrows`** | Linear 1D cycle | 2D geographic navigation across screen | Dashboards, button grids, control panels. |
| **`AltArrows`** | Linear 1D cycle | 2D geographic navigation on Alt+Arrow | Hybrid apps where arrows scroll but Alt+arrows jump focus. |

---

## 3. 2D Spatial Navigation Engine

When navigating geographically, `FocusManager::move(Direction)` calculates directional distance vectors across bounding boxes auto-harvested by `koto::Frame`:

```
Visual 2D Target Layout:
        ┌──────────────┐
        │  [Target A]  │
        └──────┬───────┘
               │ (Direction::Down)
               ▼
 ┌─────────────┐   ┌─────────────┐
 │  [Target B] │   │  [Target C] │
 └─────────────┘   └─────────────┘
```

1. **Auto-Harvested Bounds**: As widgets call `frame.tag_hit_cell(row, col, id)`, `Frame` automatically records bounding boxes in `m_tag_bounds`.
2. **Manhattan Rectilinear Projection Heuristic**: Directional distance checks filter out targets behind the current focus, prioritizing targets directly along the primary axis with a penalty for perpendicular displacement ($\text{score} = \text{primary} + 2 \times \text{secondary}$).

---

## 4. Modal Scoping & Focus Trapping (`IdScope`)

When opening a modal dialog (e.g., confirmation popup or settings window), keyboard focus must be strictly confined to elements within the modal:

```cpp
{
    // RAII Modal Scope
    IdScope modal_scope(fm, "confirmation_modal", ScopeMode::Modal);

    // Any buttons or inputs rendered here belong exclusively to the modal scope
    Button().id("btn_ok").label("OK").draw(frame, ok_area);
    Button().id("btn_cancel").label("Cancel").draw(frame, cancel_area);

} // <-- Scope pops automatically upon exiting the block!
```

* While the modal is active, pressing `Tab` or arrows only cycles within `"confirmation_modal"`.
* When `IdScope` destructs, the previous focus target and scope hierarchy are seamlessly restored.

---

## 5. Sub-Item Mouse Routing & Hit-Testing

Every cell in `Frame` maintains an entry in `Frame::m_hit_buffer`:

```
Screen Col:     0   1   2   3   4   5   6   7   8   9
Row 0 Cells:   [ B ][ u ][ t ][ t ][ o ][ n ][   ][ O ][ K ]
Row 0 Hit-Buf: [ 0x4A1F (id: "btn_ok") .................... ]
```

When a user clicks at `(row, col)`:
1. `Frame::tag_at(row, col)` inspects `m_hit_buffer` in $O(1)$ time.
2. If `tag != -1`, `FocusManager::handle_mouse(m, frame)` matches the tag to registered callbacks, shifting visual focus and triggering `on_click` actions.

---

## C++ Language Features Utilized

1. **`constexpr` FNV-1a Hashing (C++14/C++17)**: Evaluates string literal hashes at compile time (`Id id = "save_btn"`).
2. **RAII Scope Guards (C++11)**: `IdScope` automatically restores previous focus state in its destructor upon exiting scope blocks.
3. **C++20 Concepts (`<concepts>`)**: Constrains state types for focus compatibility via `FocusableState`.
4. **`std::optional<Area>` (C++17)**: Safe, null-checked return values for bounding box queries.
5. **Strongly-Typed Scoped Enums (`enum class`)**: `FocusNavPolicy`, `Direction`, and `ScopeMode`.
