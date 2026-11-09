# `koto::utf8`

High-speed RFC 3629 compliant UTF-8 stream decoder, Unicode 16.0 extended grapheme cluster segmenter, atomic cursor navigation engine, and visual column width evaluator.

---

## Header & Function Synopsis

```cpp
#include <koto/utf8.hpp>
```

```cpp
namespace koto
{
    struct Utf8Buffer
    {
        char data[5] = {};
        uint8_t size = 0;
        [[nodiscard]] constexpr std::string_view view() const noexcept;
        [[nodiscard]] constexpr const char* c_str() const noexcept;
    };

    // Modern C++23 bounds-safe stream decoder (advances 'in' by consumed bytes)
    char32_t next_codepoint(std::string_view& in);

    // Value-returning UTF-8 encoder
    Utf8Buffer encode_utf8(char32_t code);

    // Extended Grapheme Cluster Decoder (advances 'in' by consumed cluster)
    Grapheme next_grapheme(std::string_view& in);

    // Navigation & Offset Calculations
    [[nodiscard]] size_t prev_grapheme_offset(std::string_view text, size_t cursor) noexcept;
    [[nodiscard]] size_t next_grapheme_offset(std::string_view text, size_t cursor) noexcept;
    [[nodiscard]] size_t visual_col_to_byte_offset(std::string_view text, int target_visual_col) noexcept;

    // Width Queries
    [[nodiscard]] bool is_unicode_wide(char32_t cp) noexcept;
    [[nodiscard]] bool is_unicode_zero_width(char32_t cp) noexcept;
    [[nodiscard]] int codepoint_width(char32_t cp);
    [[nodiscard]] int text_width(std::string_view text);
}
```

---

## Functions Reference

| Function | Signature | Description |
|:---|:---|:---|
| **`next_codepoint()`** | `char32_t next_codepoint(std::string_view& in)` | Decodes the next valid UTF-32 codepoint and consumes its bytes from `in`. |
| **`next_grapheme()`** | `Grapheme next_grapheme(std::string_view& in)` | Consumes a complete user-perceived grapheme cluster (including ZWJ emoji, flags, skin tones). |
| **`encode_utf8()`** | `Utf8Buffer encode_utf8(char32_t code)` | Encodes a UTF-32 codepoint into an inline 5-byte buffer without dynamic heap allocation. |
| **`codepoint_width()`**| `int codepoint_width(char32_t cp)` | Queries display column width ($0$, $1$, or $2$) via Unicode 16.0 binary search tables. |
| **`text_width()`** | `int text_width(std::string_view text)` | Computes the total visual display column width of a multi-character string. |
| **`prev_grapheme_offset()`**| `size_t prev_grapheme_offset(std::string_view text, size_t cursor) noexcept` | Returns the starting byte offset of the preceding cluster for atomic backspace deletion. |
| **`next_grapheme_offset()`**| `size_t next_grapheme_offset(std::string_view text, size_t cursor) noexcept` | Returns the byte offset of the following cluster for caret navigation. |
| **`visual_col_to_byte_offset()`**| `size_t visual_col_to_byte_offset(std::string_view text, int target_visual_col) noexcept` | Translates a physical screen column coordinate to an internal UTF-8 byte index. |

---

## Usage Examples

### 1. Atomic Backspace Deletion

```cpp
#include <koto/koto.hpp>

using namespace koto;

void backspace_example()
{
    // Text ending with an 11-byte ZWJ sequence: "👩‍💻"
    std::string buffer = "Dev: 👩‍💻";
    size_t cursor = buffer.size();

    // Determine the start of the cluster preceding cursor
    size_t prev = prev_grapheme_offset(buffer, cursor);

    // Erases the entire emoji cluster cleanly without leaving stray bytes or combining marks
    buffer.erase(prev, cursor - prev);
}
```

### 2. Mouse Click Visual Column Translation

```cpp
#include <koto/koto.hpp>

using namespace koto;

void handle_click_on_line(std::string_view line_text, int mouse_x)
{
    // Convert physical screen column to UTF-8 byte position
    size_t byte_index = visual_col_to_byte_offset(line_text, mouse_x);

    // Position text editor caret at byte_index
}
```

---

## Invariants & Safety Guarantees

1. **RFC 3629 Security Sanitization**: Overlong sequences, UTF-16 surrogate halves (`0xD800`–`0xDFFF`), and codepoints $> \text{U+10FFFF}$ are safely rejected and replaced with `U+FFFD`.
2. **Infinite Loop Prevention**: On any invalid byte sequence, `next_codepoint` always consumes at least 1 byte (`in.remove_prefix(1)`).
3. **No-Throw Navigation**: All cursor navigation and visual column mapping functions are marked `noexcept`.

---

## C++ Language Features Utilized

1. **`std::string_view` Stream Slicing (C++17)**: Eliminates string copies during character decoding.
2. **`char32_t` Unicode Literals (C++11)**: Native representation of Unicode codepoints (`U'琴'`, `U'\uFFFD'`).
3. **`noexcept` Specification (C++11)**: Compile-time performance guarantees on hot text navigation routines.
4. **Compile-Time Table Search (C++11/C++14)**: Logarithmic $O(\log N)$ binary search over static interval tables.
