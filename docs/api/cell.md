# `koto::Cell` & `koto::Grapheme`

Cache-line aligned 44-byte terminal grid cell owning an inline 24-byte extended grapheme cluster buffer, 24-bit TrueColor tags, text styling attributes, and continuation sentinels.

---

## Internal Memory Layout & Cache Alignment

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Total Footprint: 44 Bytes (Fits in a single 64-Byte CPU L1 Cache Line)      │
├──────────────────────────────────────────────────────────────┬──────────────┤
│ Offset  0..23 (24B) : Grapheme::data (Inline UTF-8 array)    │ Metadata:    │
│ Offset 24     ( 1B) : Grapheme::size                         │ attrs (1B)   │
│ Offset 25     ( 1B) : Grapheme::width                        │ cont  (1B)   │
│ Offset 26..27 ( 2B) : Padding                                │ padding (2B) │
│ Offset 28..31 ( 4B) : Grapheme::base_cp (char32_t)           │              │
│ Offset 32..35 ( 4B) : ch (char32_t primary codepoint)        │              │
│ Offset 36..39 ( 4B) : fg (24-bit TrueColor / default)        │              │
│ Offset 40..43 ( 4B) : bg (24-bit TrueColor / default)        │              │
└──────────────────────────────────────────────────────────────┴──────────────┘
```

---

## Header & Class Synopsis

```cpp
#include <koto/cell.hpp>
#include <koto/utf8.hpp>
```

```cpp
namespace koto
{
    enum class TextAttr : uint8_t
    {
        None      = 0,
        Bold      = 1 << 0,
        Dim       = 1 << 1,
        Italic    = 1 << 2,
        Underline = 1 << 3,
        Reverse   = 1 << 4,
        Strike    = 1 << 5
    };

    struct Grapheme
    {
        char data[24] = {' ', '\0'}; // Inline UTF-8 bytes (null-terminated)
        uint8_t size = 1;             // Byte length (1 to 23)
        uint8_t width = 1;            // Visual columns (0, 1, or 2)
        char32_t base_cp = U' ';      // Primary codepoint

        constexpr Grapheme() noexcept = default;
        constexpr Grapheme(char32_t cp, int w = 1) noexcept;
        constexpr Grapheme(std::string_view sv, char32_t cp = 0, int w = 1) noexcept;

        [[nodiscard]] constexpr std::string_view view() const noexcept;
        [[nodiscard]] constexpr const char* c_str() const noexcept;
        [[nodiscard]] constexpr bool empty() const noexcept;
        bool operator==(const Grapheme&) const = default;
    };

    struct Cell
    {
        Grapheme grapheme{};
        char32_t ch = U' ';
        uint32_t fg = COLOR_DEFAULT;
        uint32_t bg = COLOR_DEFAULT;
        int width = 1;
        bool continuation = false;
        uint8_t attrs = 0;

        constexpr Cell() noexcept = default;

        // Construct from single codepoint
        constexpr Cell(char32_t cp, uint32_t f = COLOR_DEFAULT, uint32_t b = COLOR_DEFAULT,
                       int w = 0, bool cont = false, uint8_t att = 0) noexcept;

        // Construct from Grapheme
        constexpr Cell(Grapheme g, uint32_t f = COLOR_DEFAULT, uint32_t b = COLOR_DEFAULT,
                       bool cont = false, uint8_t att = 0) noexcept;

        // Construct from Grapheme with explicit width
        constexpr Cell(Grapheme g, uint32_t f, uint32_t b, int w,
                       bool cont = false, uint8_t att = 0) noexcept;

        [[nodiscard]] constexpr std::string_view view() const noexcept;
        static constexpr Cell make_continuation() noexcept;

        bool operator==(const Cell&) const = default;
    };
}
```

---

## Member Variables Reference

| Field | Type | Description |
|:---|:---|:---|
| `grapheme` | `Grapheme` | Embedded 24-byte UTF-8 cluster with zero heap allocation. |
| `ch` | `char32_t` | Primary UTF-32 codepoint for backward compatibility. |
| `fg` | `uint32_t` | 24-bit TrueColor packed RGB foreground or `COLOR_DEFAULT`. |
| `bg` | `uint32_t` | 24-bit TrueColor packed RGB background or `COLOR_DEFAULT`. |
| `width` | `int` | Display column width ($0$ for combining/formatting, $1$ for Latin, $2$ for CJK/emoji). |
| `continuation` | `bool` | `true` if this cell is the reserved trailing companion cell of a 2-column wide glyph. |
| `attrs` | `uint8_t` | Bitmask of active `TextAttr` flags (`Bold`, `Underline`, `Reverse`, etc.). |

---

## Usage Examples

### 1. Constructing Styled Text Cells

```cpp
#include <koto/koto.hpp>

using namespace koto;

void style_demo()
{
    // Combined Bold + Underline attributes
    uint8_t styling = static_cast<uint8_t>(TextAttr::Bold) | static_cast<uint8_t>(TextAttr::Underline);

    Cell header_cell{U'#', Color::Cyan, Color::Default, 1, false, styling};
}
```

### 2. Rendering Emoji Graphemes Without Allocation

```cpp
#include <koto/koto.hpp>

using namespace koto;

void emoji_demo(Frame& frame)
{
    // Complex ZWJ emoji: 👩‍💻 (11 bytes UTF-8)
    std::string_view dev = "👩‍💻";
    Grapheme g = next_grapheme(dev);

    // Embed directly into Cell: width is 2, grapheme owns the 11 bytes inline
    Cell cell{g, Color::White, Color::DarkGray};
    frame.set_cell(0, 0, cell);
}
```

---

## Invariants & Safety Guarantees

1. **Zero Heap Allocation Guarantee**: Constructing and copying `Cell` or `Grapheme` never allocates heap memory (`new` or `malloc`).
2. **Safe Default Equality (`operator==`)**: Modern defaulted comparisons allow the diffing engine to check for identical cells with a single compiler-synthesized equality operator.
3. **Continuation Factory Invariant**: `Cell::make_continuation()` produces a cell with `continuation = true` and `width = 0` to ensure it is never rendered independently.

---

## C++ Language Features Utilized

1. **Defaulted Comparison (`operator== = default`) (C++20)**: Generates member-wise equality checks without hand-rolled comparison boilerplate.
2. **`constexpr` Constructors (C++14/C++20)**: Allows compile-time creation of cell and grapheme constants.
3. **Inline C-Style Array within Struct (`char data[24]`)**: Guarantees flat memory placement without pointer indirection.
4. **`std::string_view` (C++17)**: High-speed zero-copy access into the inline buffer via `view()`.
5. **Scoped Strong Enums with Underlying Type (`enum class : uint8_t`) (C++11)**: Bitmask-compatible styling flags with explicit 1-byte storage footprint.
