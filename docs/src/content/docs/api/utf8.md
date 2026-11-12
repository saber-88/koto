---
title: UTF-8 and Grapheme
description: Bounds-safe UTF-8 codec, 44-byte Grapheme cluster struct, atomic cursor offset calculators, and Unicode 16.0 width functions
sidebar:
  label: UTF-8
  order: 9
---

## Purpose and header

`koto/utf8.hpp` provides RFC 3629 UTF-8 encoding and decoding, the inline `Grapheme` cluster struct (`char data[36]`), extended grapheme cluster segmentation (`next_grapheme`), cursor offset helpers (`prev_grapheme_offset`, `next_grapheme_offset`, `visual_col_to_byte_offset`), and Unicode 16.0 display width queries (`is_unicode_wide`, `is_unicode_zero_width`, `codepoint_width`, `text_width`).
Header: `koto/utf8.hpp` (included by `koto/koto.hpp`).

<!-- src: include/koto/utf8.hpp:1-145 -->

## Types and members

| Symbol | Signature | Description | Default |
| :--- | :--- | :--- | :--- |
| `Utf8Buffer` | `struct Utf8Buffer { char data[5] = {}; uint8_t size = 0; ... }` | Value-returned 1–4 byte UTF-8 sequence with null terminator, `view()`, and `c_str()`. | `size = 0` |
| `next_codepoint(in)` | `char32_t next_codepoint(std::string_view& in)` | Decodes the next codepoint, advances `in`, and returns `U'\uFFFD'` on invalid/overlong/surrogate bytes (or `0` if empty). | None |
| `next_codepoint(ptr, end)` | `char32_t next_codepoint(const char*& ptr, const char* end)` | Pointer-advancing overload wrapping `next_codepoint(std::string_view&)`. | None |
| `encode_utf8(code)` | `Utf8Buffer encode_utf8(char32_t code)` | Encodes `code` into a `Utf8Buffer` by value. | None |
| `encode_utf8(code, buf)` | `int encode_utf8(char32_t code, char* utf8)` | Writes 1–4 UTF-8 bytes into `utf8` and returns the byte length. | None |
| `Grapheme` | `struct Grapheme` | Inline cluster holding `char data[36]`, `uint8_t size`, `uint8_t width`, and `char32_t base_cp` (`sizeof(Grapheme) == 44`). | `data = {' ', '\0'}`, `size = 1`, `width = 1`, `base_cp = U' '` |
| `Grapheme::view()` / `c_str()` / `empty()` | `constexpr` accessors | Returns `std::string_view(data, size)`, `data`, or `size == 0`. | None |
| `next_grapheme(in)` | `Grapheme next_grapheme(std::string_view& in)` | Consumes one extended grapheme cluster from `in` and computes its display width (`0`, `1`, or `2`). | None |
| `prev_grapheme_offset(text, cursor)` | `size_t prev_grapheme_offset(std::string_view text, size_t cursor) noexcept` | Returns the start byte index of the grapheme cluster immediately before `cursor` (or `0`). | None |
| `next_grapheme_offset(text, cursor)` | `size_t next_grapheme_offset(std::string_view text, size_t cursor) noexcept` | Returns the end byte index of the grapheme cluster starting at `cursor` (or `text.size()`). | None |
| `visual_col_to_byte_offset(text, col)` | `size_t visual_col_to_byte_offset(std::string_view text, int target_visual_col) noexcept` | Converts a 0-based visual column into the corresponding UTF-8 byte index in `text`. | None |
| `is_unicode_wide(cp)` | `bool is_unicode_wide(char32_t cp) noexcept` | Binary-searches `UNICODE_WIDE_RANGES` (`122` intervals in `src/unicode_data.inc`, returning `false` for `cp < 0x1100`). | None |
| `is_unicode_zero_width(cp)` | `bool is_unicode_zero_width(char32_t cp) noexcept` | Returns `true` for `0`, `0x200B`, `0xFEFF`, or codepoints in `UNICODE_ZERO_RANGES` (`cp >= 0x0300`). | None |
| `codepoint_width(cp)` | `int codepoint_width(char32_t cp)` | Returns `0` for control/zero-width, `2` for wide codepoints, and `1` for all others. | None |
| `text_width(text)` | `int text_width(std::string_view text)` | Sums grapheme widths across `text`, expanding `\t` to 4-column tab stops (`((total / 4) + 1) * 4`). | None |

<!-- src: include/koto/utf8.hpp:8-145 -->
<!-- src: src/utf8.cpp:9-425 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

int main()
{
    // U+7434 CJK ideograph (3 bytes, 2 columns) + space + ASCII
    std::string_view sample = "\xE7\x90\xB4 koto";
    assert(text_width(sample) == 7);

    std::string_view stream = sample;
    Grapheme first = next_grapheme(stream);
    assert(first.size == 3);
    assert(first.width == 2);
    assert(first.base_cp == 0x7434);

    assert(prev_grapheme_offset(sample, 3) == 0);
    assert(visual_col_to_byte_offset(sample, 2) == 3);
    return 0;
}
```

## Errors and edge cases

- **Inline `Grapheme` capacity (`35` UTF-8 bytes)**: `Grapheme::data` is `char data[36]`, holding up to `35` UTF-8 bytes plus a null terminator (`include/koto/utf8.hpp:28,90`). If a grapheme cluster exceeds 35 bytes, the constructor stops copying at the last complete codepoint boundary that fits in `35` bytes (`include/koto/utf8.hpp:103-108`) so it never slices mid-codepoint.
- **Malformed UTF-8 recovery**: `next_codepoint()` rejects stray continuation bytes (`0x80..0xBF`), overlong encodings (`0xC0..0xC1`, `0xE0` with `< 0xA0`, `0xF0` with `< 0x90`), UTF-16 surrogates (`0xED` with `0xA0..0xBF`), and values `> U+10FFFF`, advancing only the invalid prefix bytes and returning `U'�'` so subsequent valid ASCII bytes are not swallowed (`src/utf8.cpp:22-126`).
- **Tab width in `text_width` vs. `visual_col_to_byte_offset`**: `text_width()` aligns `	` to the next 4-column tab stop (`((total / 4) + 1) * 4`), whereas `visual_col_to_byte_offset()` treats `	` as a fixed width of `4` columns (`src/utf8.cpp:360-364`, `src/utf8.cpp:412`).

<!-- src: include/koto/utf8.hpp:77-120 -->
<!-- src: src/utf8.cpp:247-425 -->

## See also

- [Cell and Color](/api/cell-color/)
- [Unicode Guide](/guides/unicode/)
- [Unicode Deep Dive](/deep-dives/unicode/)
