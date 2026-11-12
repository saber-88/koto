---
title: Focus Management, Scopes, and Modal Trapping
description: How FocusManager routes keyboard and mouse events across a 1D focus ring, 2D spatial layout, and modal scopes
sidebar:
  label: Focus Management
  order: 5
---

## The idea in plain words

In a multi-widget terminal interface, only one interactive control should receive typed characters at a time, and users expect `Tab`, `Shift+Tab`, directional arrows, or mouse clicks to move focus predictably. `koto::FocusManager` centralizes focus state, syncs the `.focused` boolean on your `state struct` instances, and isolates focus inside modal dialogs when opened.

## How koto implements it

1. **Type-erased `focus ring` (`std::vector<FocusTarget>`)**: Calling `fm.add(state)` constrains `state` with the C++20 `FocusableState` concept (`s.focused` and `s.handle_event(k)`) and uses `if constexpr` to detect `HasMouseHandler` (`s.handle_mouse(m)`) and `HasPasteHandler` (`s.insert(str)`), storing thin lambda closures inside a `FocusTarget` struct (`include/koto/focus.hpp:146-187`). The first target added to an empty `FocusManager` receives focus automatically (`m_active_index = 0`) (`src/focus.cpp:117-124`).
2. **1D and 2D navigation policies (`FocusNavPolicy`)**:
   - `Tab` (`Key::Tab`) and `Shift+Tab` (`Key::BackTab`) always call `fm.next()` and `fm.prev()`, cycling across targets in `m_targets` that match `active_modal_scope()` and satisfy `is_focusable(has_geometry())` (`src/focus.cpp:257-313, 550-558`).
   - In `FocusNavPolicy::TabOnly` (default), arrow keys are passed only to the focused widget and never move focus between widgets.
   - In `FocusNavPolicy::SpatialArrows`, arrow keys not consumed by the focused widget invoke `fm.move(Direction)`, which compares harvested `bounds` centers (`center_row()`, `center_col()`) and edge distances in `active_modal_scope()` to pick the target minimizing `primary_dist + 2 * secondary_dist` (`src/focus.cpp:408-498, 580-586`).
   - In `FocusNavPolicy::AltArrows`, `Alt` + arrow keys invoke `fm.move(Direction)` before the focused widget sees the key (`src/focus.cpp:561-567`).
3. **Normal `scope` vs. `modal scope`**:
   - Pushing a normal `scope` (`fm.scope(Id("panel"))` or `fm.push_scope(id, ScopeMode::Normal)`) combines `scope_id` into `fm.scoped_id(base_id)` so reusable components can register local IDs without colliding (`src/focus.cpp:779-791`).
   - Pushing a `modal scope` (`fm.modal_scope(Id("dialog"))` or `fm.push_scope(id, ScopeMode::Modal)`) saves the current focused `Id` onto `m_saved_focus_stack`, tags all subsequently registered targets with `modal_scope = scope_id`, immediately focuses the first target registered in that modal, restricts `next()`, `prev()`, `focus()`, and `move()` to targets with `modal_scope == scope_id`, and absorbs mouse clicks outside the modal (`src/focus.cpp:89-115, 636-700, 722-742`).
   - Popping a `modal scope` (`fm.pop_scope()` or `~IdScope()`) erases all targets with `t.modal_scope == popped_id` from `m_targets` and restores focus to the saved `Id` (`src/focus.cpp:744-777`).

<!-- src: include/koto/focus.hpp:27-313 -->
<!-- src: src/focus.cpp:50-803 -->

```cpp
TextInputState first("a");
TextInputState second("b");
fm.add(first).add(second);
fm.next();
```

## Consequences for your code

- **Register persistent background widgets once before `app.run()`**: Because `register_target` updates existing targets in place by `Id` (`src/focus.cpp:55-99`), you can register background `state struct` instances once during setup and only call `fm.push_scope(..., ScopeMode::Modal)` + `fm.add(...)` when opening a modal dialog (and `fm.pop_scope()` when closing it).
- **Hidden widgets on inactive tabs are skipped automatically**: Once `sync_geometry(frame)` has harvested at least one valid bounding box (`fm.has_geometry() == true`), any registered widget that was not drawn during the most recent `on_render` has `bounds == Area{0, 0, 0, 0}` and is skipped by `next()` and `prev()` (`src/focus.cpp:265-277, 382-406`).

## See also

- [FocusManager](/api/focus-manager/)
- [Focus System Architecture](/architecture/focus-system/)
- [Modal Dialogs Guide](/guides/modal-dialogs/)
