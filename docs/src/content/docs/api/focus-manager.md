---
title: FocusManager
description: Concept-constrained 1D focus ring, 2D spatial navigator, hit-buffer mouse router, and RAII modal scope manager
sidebar:
  label: FocusManager
  order: 6
---

## Purpose and header

`koto::FocusManager` coordinates keyboard focus, 1D `Tab` / `BackTab` cycling, 2D spatial arrow navigation across harvested `Frame` bounding boxes, mouse click and wheel routing, and normal or modal `IdScope` guards.
Header: `koto/focus.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/focus.hpp:1-314 -->

## Types and members

### Concepts, enums, and callback aliases

| Symbol | Kind | Description | Default |
| :--- | :--- | :--- | :--- |
| `FocusableState<T>` | Concept | Satisfied when `s.focused` converts to `bool` and `s.handle_event(const KeyEvent&)` converts to `bool`. | Built-ins: `ListState`, `TabsState`, `TextInputState`, `TextAreaState` |
| `HasMouseHandler<T>` | Concept | Satisfied when `s.handle_mouse(const MouseEvent&)` converts to `bool`. | Built-ins: `ListState`, `TabsState`, `TextAreaState` |
| `HasPasteHandler<T>` | Concept | Satisfied when `s.insert(const std::string&)` is valid. | Built-ins: `TextInputState`, `TextAreaState` |
| `Direction` | Enum class (`uint8_t`) | `Up`, `Down`, `Left`, `Right`. | None |
| `ScopeMode` | Enum class (`uint8_t`) | `Normal` (ID namespacing only) or `Modal` (ID namespacing + focus trapping and click shielding). | `Normal` |
| `FocusNavPolicy` | Enum class (`uint8_t`) | `TabOnly` (default), `SpatialArrows`, `AltArrows`, `Custom`. | `TabOnly` |
| `FocusKeyInterceptor` | Type alias | `std::function<bool(FocusManager& fm, const KeyEvent& key)>`. | None |
| `FocusChangeValidator` | Type alias | `std::function<bool(FocusManager& fm, Id from_id, Id to_id)>`. | None |
| `FocusChangeObserver` | Type alias | `std::function<void(Id from_id, Id to_id)>`. | None |

<!-- src: include/koto/focus.hpp:27-71 -->

### `IdScope` and `FocusTarget`

| Symbol | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `IdScope(fm, scope_id, mode)` | `IdScope(FocusManager& fm, Id scope_id, ScopeMode mode = ScopeMode::Normal)` | Calls `fm.push_scope(scope_id, mode)` on construction and `fm.pop_scope()` on destruction. Move-only. | `mode = ScopeMode::Normal` |
| `FocusTarget` | Struct | Stores `id`, `bounds`, `modal_scope`, `enabled`, `visible`, `set_focus`, `handle_key`, `handle_mouse`, `handle_paste`, `on_focus`, `on_blur`, `state_ptr`, and `has_custom_id`. | `enabled = true`, `visible = true` |
| `FocusTarget::is_focusable()` | `bool is_focusable(bool require_geometry = true) const noexcept` | Returns `enabled && visible` and (when `require_geometry` is `true`) `bounds.is_valid()`. | `require_geometry = true` |

<!-- src: include/koto/focus.hpp:79-127 -->

### `FocusManager` methods

| Member | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `add(state, custom_id)` | `template<FocusableState T> FocusManager& add(T& state, Id custom_id = Id::none())` | Registers `state` (using `custom_id` or `Id(&state)` combined with active scopes) and auto-wires `handle_mouse` and `handle_paste` if supported. | `custom_id = Id::none()` |
| `add(id, on_key, on_mouse)` | `FocusManager& add(Id id, std::function<bool(const KeyEvent&)> on_key = nullptr, std::function<bool(const MouseEvent&)> on_mouse = nullptr)` | Registers a stateless target by `Id`. If `on_mouse == nullptr` and `on_key != nullptr`, left clicks synthesize `Key::Enter` to `on_key`. | `nullptr`, `nullptr` |
| `register_target(target)` | `void register_target(FocusTarget target)` | Inserts or updates `target` in `m_targets`; focuses index `0` on first target or focuses immediately if registered inside an active modal scope. | None |
| `remove(id)` / `remove(state)` | `bool remove(Id id)` / `bool remove(const T& state)` | Removes target from `m_targets` and clamps `m_active_index`. | None |
| `set_visible()` / `is_visible()` | `FocusManager& set_visible(Id id, bool visible)` / `bool is_visible(Id id) const` | Sets or queries target visibility (`false` skips target in `next()`/`prev()`). | `true` |
| `set_enabled()` / `is_enabled()` | `FocusManager& set_enabled(Id id, bool enabled)` / `bool is_enabled(Id id) const` | Sets or queries target enabled state (`false` skips target in `next()`/`prev()`). | `true` |
| `has_geometry()` | `bool has_geometry() const noexcept` | Returns `true` if at least one target has valid non-zero `bounds`. | `false` before `sync_geometry` |
| `next()` / `prev()` | `bool next()` / `bool prev()` | Cycles forward (`Tab`) or backward (`BackTab`) through focusable targets in the active modal scope. | None |
| `focus(...)` / `focus_id(...)` | `bool focus(size_t index)` / `bool focus_id(Id id)` / `bool focus(Id id)` / `bool focus(const T& state)` | Focuses the specified target if it belongs to the active modal scope and `can_focus` returns `true`. | None |
| `sync_geometry(frame)` | `void sync_geometry(const Frame& frame)` | Copies `frame.bounds_of(target.id)` into each target and calls `next()` if the active target is no longer focusable on screen. | None |
| `move(dir)` / `move_up()` / `down()` / `left()` / `right()` | `bool move(Direction dir)` | Moves focus in 2D to the target in the active modal scope minimizing `primary_dist + 2 * secondary_dist`. | None |
| `is_focused(...)` | `bool is_focused(Id id) const` / `bool is_focused(const T& state) const` | Returns `true` if `id` or `state` is the currently focused target. | None |
| `current_index()` / `current_id()` | `size_t current_index() const noexcept` / `Id current_id() const noexcept` | Returns active index or active `Id` (`Id::none()` if empty). | `0` / `Id::none()` |
| `size()` / `empty()` / `clear()` | `size_t size() const noexcept` / `bool empty() const noexcept` / `void clear()` | Queries target count or clears all targets, scopes, and saved focus stack. | None |
| `handle_key(key)` | `bool handle_key(const KeyEvent& key)` | Dispatches `key` through interceptor, `Tab`/`BackTab`, `AltArrows`, active target, and `SpatialArrows`. | None |
| `handle_paste(text)` | `bool handle_paste(const std::string& text)` | Routes `text` to the active target's `handle_paste` closure. | None |
| `navigation_policy(...)` | `FocusManager& navigation_policy(FocusNavPolicy policy) noexcept` / `FocusNavPolicy navigation_policy() const noexcept` | Sets or gets the active `FocusNavPolicy`. | `FocusNavPolicy::TabOnly` |
| `enable_spatial_arrows()` | `FocusManager& enable_spatial_arrows(bool enable = true) noexcept` | Sets policy to `SpatialArrows` when `true` or `TabOnly` when `false`. | `enable = true` |
| `interceptor()` / `can_focus()` / `on_focus_change()` | Hook setters | Registers `FocusKeyInterceptor`, `FocusChangeValidator`, or `FocusChangeObserver`. | `nullptr` |
| `handle_mouse()` / `handle_mouse_click()` | `bool handle_mouse(const MouseEvent& mouse, const Frame& frame)` / `bool handle_mouse_click(int row, int col, const Frame& frame)` | Hit-tests `(row, col)` in `frame`, shifts focus on left press, routes mouse wheel/press to target, and shields background targets during a modal scope. | None |
| `scope(id)` / `modal_scope(id)` | `IdScope scope(Id scope_id)` / `IdScope modal_scope(Id scope_id)` | Returns an RAII `IdScope` with `ScopeMode::Normal` or `ScopeMode::Modal`. | None |
| `push_scope()` / `pop_scope()` | `void push_scope(Id scope_id, ScopeMode mode = ScopeMode::Normal)` / `void pop_scope()` | Manually pushes or pops a scope entry (`Modal` pop erases modal targets and restores saved focus). | `mode = ScopeMode::Normal` |
| `scoped_id()` / `id()` / `active_modal_scope()` | `Id scoped_id(Id base_id) const` / `Id id(Id base_id) const` / `Id id(const T& state) const` / `Id active_modal_scope() const` | Computes scope-prefixed `Id` or returns the innermost active modal scope `Id`. | `Id::none()` |

<!-- src: include/koto/focus.hpp:139-313 -->
<!-- src: src/focus.cpp:50-803 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

int main()
{
    FocusManager fm;
    TextInputState email("user@example.com");
    TextInputState password("");

    fm.add(email).add(password);
    assert(fm.is_focused(email));

    fm.handle_key(KeyEvent{static_cast<uint32_t>(Key::Tab)});
    assert(fm.is_focused(password));
    return 0;
}
```

## Errors and edge cases

- **Non-owning state references**: `FocusManager::add(T& state)` captures `&state` by reference inside lambda closures. Every registered `state struct` must outlive `FocusManager`, be unregistered via `fm.remove(state)` before destruction, or be registered inside a `modal scope` (which automatically prunes its targets on `pop_scope()`).
- **Vector reallocation with custom `Id`**: If `state struct` instances are stored in a `std::vector` that reallocates, pass an explicit `custom_id` to `fm.add(vec[i], Id("item", i))` when re-registering so `register_target` updates the target in place without triggering the debug pointer-hash collision `assert`.
- **Tab in `TextArea`**: Because `FocusManager::handle_key` checks `Key::Tab` and `Key::BackTab` in step 2 before calling the focused target's `handle_key` in step 4 (`src/focus.cpp:550-576`), pressing `Tab` while a `TextArea` is managed by `FocusManager` cycles focus instead of inserting 4 spaces unless you intercept `Key::Tab` in `fm.interceptor(...)`.

<!-- src: include/koto/focus.hpp:132-138 -->
<!-- src: src/focus.cpp:59-68 -->
<!-- src: src/focus.cpp:537-589 -->

## See also

- [Id](/api/id/)
- [Focus Management Concept](/concepts/focus-management/)
- [Focus System Architecture](/architecture/focus-system/)
- [Modal Dialogs Guide](/guides/modal-dialogs/)
