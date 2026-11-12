---
title: Id
description: 32-bit FNV-1a identifier token for widgets, state structs, focus targets, and hit-buffer cells
sidebar:
  label: Id
  order: 7
---

## Purpose and header

`koto::Id` is a 4-byte identifier token used across `Frame`, `FocusManager`, and widgets to tag screen regions in the `hit buffer` and identify focus targets without heap allocation or string comparisons at runtime.
Header: `koto/id.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/id.hpp:1-109 -->

## Types and members

| Symbol | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `TAG_NONE` | `inline constexpr int TAG_NONE = -1` | Sentinel value (`-1` / `0xFFFFFFFFu`) representing an untagged cell or empty `Id`. | `-1` |
| `fnv1a_32(str, seed)` | `constexpr uint32_t fnv1a_32(std::string_view str, uint32_t seed = 0x811c9dc5u) noexcept` | Computes 32-bit FNV-1a hash of `str`, mapping `0xFFFFFFFFu` to `0xFFFFFFFEu` so a hash never equals `TAG_NONE`. | `seed = 0x811c9dc5u` |
| `fnv1a_32(val, seed)` | `constexpr uint32_t fnv1a_32(uint64_t val, uint32_t seed = 0x811c9dc5u) noexcept` | Computes 32-bit FNV-1a hash of 8 little-endian bytes of `val`, mapping `0xFFFFFFFFu` to `0xFFFFFFFEu`. | `seed = 0x811c9dc5u` |
| `Id()` | `constexpr Id() noexcept` | Constructs an empty `Id` equal to `Id::none()` (`TAG_NONE`). | `TAG_NONE` |
| `Id(uint32_t)` / `Id(int)` | `constexpr Id(uint32_t raw_val) noexcept` / `constexpr Id(int raw_val) noexcept` | Constructs an `Id` directly from a raw integer value. | None |
| `Id(std::string_view)` / `Id(const char*)` | `constexpr Id(std::string_view str) noexcept` / `constexpr Id(const char* str) noexcept` | Hashes a string literal or view via `fnv1a_32(str)`. | None |
| `Id(str, index)` | `constexpr Id(std::string_view str, size_t index) noexcept` | Hashes `str` and chains `index` (`fnv1a_32(static_cast<uint64_t>(index), fnv1a_32(str))`). | None |
| `Id(str, row, col)` | `constexpr Id(std::string_view str, size_t row, size_t col) noexcept` | Hashes `str`, `row`, and `col` for 2D grid cells. | None |
| `Id(const void* ptr)` | `Id(const void* ptr) noexcept` | Hashes a pointer address via `fnv1a_32(reinterpret_cast<uintptr_t>(ptr))`. | None |
| `Id(Enum e)` | `template<typename Enum> requires std::is_enum_v<Enum> constexpr Id(Enum e) noexcept` | Casts any enum or `enum class` value to `uint32_t`. | None |
| `combine(other)` | `constexpr Id combine(Id other) const noexcept` | Combines two `Id` tokens via `Id(fnv1a_32(static_cast<uint64_t>(other.m_value), m_value))`. | None |
| `value()` | `constexpr uint32_t value() const noexcept` | Returns the raw `uint32_t` token value. | `0xFFFFFFFFu` |
| `is_none()` | `constexpr bool is_none() const noexcept` | Returns `m_value == static_cast<uint32_t>(TAG_NONE)`. | `true` for `Id()` |
| `operator bool()` | `constexpr explicit operator bool() const noexcept` | Returns `!is_none()`. | `false` for `Id()` |
| `operator int()` | `constexpr explicit operator int() const noexcept` | Casts `m_value` to `int`. | `-1` for `Id()` |
| `operator==` | `constexpr bool operator==(const Id&) const noexcept = default` (plus `int` and `uint32_t` overloads) | Compares two `Id` values or an `Id` against an integer. | None |
| `Id::none()` | `static constexpr Id none() noexcept` | Returns `Id()`. | `TAG_NONE` |

<!-- src: include/koto/id.hpp:10-108 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

enum class Pane { Sidebar, Editor };

int main()
{
    constexpr Id by_name("submit_btn");
    constexpr Id by_index("row", 3);
    constexpr Id by_grid("cell", 1, 2);
    constexpr Id by_enum(Pane::Editor);

    ListState state({"a", "b"});
    Id by_ptr(&state);

    assert(!by_name.is_none());
    assert(by_name != by_index);
    assert(by_grid == Id("cell", 1, 2));
    assert(by_enum == Id(Pane::Editor));
    assert(by_ptr == Id(&state));
    return 0;
}
```

## Errors and edge cases

- **Passing string literals to `.id(...)` on widgets**: Because `Id` has both `Id(const char*)` and `Id(const void*)` constructors, passing a bare string literal to a method taking `Id` can trigger overload resolution ambiguity with pointer conversions in some contexts. Always wrap string literals explicitly: `.id(Id("name"))`.
- **Enum values and `-1`**: `Id(Enum e)` casts `e` directly to `uint32_t` without running `fnv1a_32`. Avoid defining an enum enumerator with value `-1` (`0xFFFFFFFFu`), as `static_cast<uint32_t>(-1)` equals `TAG_NONE` and `is_none()` will return `true`.
- **Pointer hashing collision check**: `Id(const void*)` folds a 64-bit pointer into a 32-bit FNV-1a hash. When two distinct `state struct` objects are registered in `FocusManager` without custom IDs, `FocusManager::register_target` asserts in debug builds (`assert(m_targets[i].state_ptr == target.state_ptr)`) that no 32-bit hash collision occurred.

<!-- src: include/koto/id.hpp:58-84 -->
<!-- src: src/focus.cpp:59-68 -->

## See also

- [Frame](/api/frame/)
- [FocusManager](/api/focus-manager/)
- [IDs and Hit Buffer Concept](/concepts/ids-hit-buffer/)
