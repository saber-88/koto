---
title: Unicode 16.0 and Grapheme Segmentation Internals
description: Deep dive into RFC 3629 validation, Unicode 16.0 interval tables, next_grapheme clustering rules, and continuation cells
sidebar:
  label: Unicode Internals
  order: 3
---

## Stage 1: RFC 3629 UTF-8 stream decoding (`next_codepoint`)

`koto::next_codepoint(std::string_view& in)` (`src/utf8.cpp:9-127`) decodes one codepoint from `in` and advances `in` past the consumed bytes:
- **1-byte ASCII (`0x00..0x7F`)**: Consumes 1 byte and returns it directly.
- **Invalid lead bytes (`0x80..0xC1` or `>= 0xF5`)**: Consumes 1 byte and returns `U'\uFFFD'`.
- **2-byte sequences (`0xC2..0xDF`)**: Requires `(b1 & 0xC0) == 0x80`; if missing or truncated, consumes only `1` byte (`in.remove_prefix(1)`) and returns `U'\uFFFD'` so a following valid ASCII byte is not lost.
- **3-byte sequences (`0xE0..0xEF`)**: Rejects overlong sequences (`byte == 0xE0 && b1 < 0xA0`) and UTF-16 surrogates (`byte == 0xED && b1 >= 0xA0`), consuming only the invalid prefix bytes (`1` or `2` bytes) and returning `U'\uFFFD'`.
- **4-byte sequences (`0xF0..0xF4`)**: Rejects overlong sequences (`byte == 0xF0 && b1 < 0x90`) and codepoints above `U+10FFFF` (`byte == 0xF4 && b1 > 0x8F`), returning `U'\uFFFD'`.

<!-- src: src/utf8.cpp:9-127 -->

## Stage 2: Unicode 16.0 interval tables (`codepoint_width`)

`src/unicode_data.inc` (included inside `namespace koto` at `src/utf8.cpp:180`) defines two sorted `constexpr` arrays of `struct UnicodeInterval { char32_t first; char32_t last; }`:
- `UNICODE_WIDE_RANGES[122]`: East Asian Wide (`W`) and Fullwidth (`F`) ranges from UAX #11 plus default emoji presentation ranges from UTS #51.
- `UNICODE_ZERO_RANGES`: Combining marks, variation selectors, and zero-width formatting characters.

`codepoint_width(char32_t cp)` (`src/utf8.cpp:218-245`) evaluates in order:
1. Printable ASCII (`cp >= 0x20 && cp < 0x7F`) returns `1`.
2. C0/C1 control characters (`cp < 0x20 || (cp >= 0x7F && cp < 0xA0)`) return `0`.
3. `is_unicode_zero_width(cp)` (`cp == 0 || cp == 0x200B || cp == 0xFEFF` or binary search in `UNICODE_ZERO_RANGES` for `cp >= 0x0300`) returns `0`.
4. `is_unicode_wide(cp)` (binary search in `UNICODE_WIDE_RANGES` for `cp >= 0x1100`) returns `2`.
5. All other codepoints return `1`.

<!-- src: src/unicode_data.inc:1-10 -->
<!-- src: src/utf8.cpp:180-245 -->

## Stage 3: Extended grapheme cluster segmentation (`next_grapheme`)

`next_grapheme(std::string_view& in)` (`src/utf8.cpp:247-350`) groups multi-codepoint sequences into a single `koto::Grapheme` (`char data[36]`, up to `35` UTF-8 bytes):
1. **ASCII fast path and keycaps (`base_cp < 0x80`)**: If `base_cp` is `'0'..'9'`, `'#'`, or `'*'` followed immediately by `0xFE0F` and `0x20E3` (keycap sequence `'1'` + `U+FE0F` + `U+20E3`), consumes all 3 codepoints (`7` bytes) with `width = 2`. Otherwise consumes 1 ASCII byte with `width = (base_cp < 0x20) ? 0 : 1`.
2. **Regional indicator flag pairs (`0x1F1E6..0x1F1FF`)**: If followed by a second regional indicator in `0x1F1E6..0x1F1FF`, consumes both codepoints (`8` bytes) as a single flag cluster with `width = 2` and returns immediately (`src/utf8.cpp:280-292`).
3. **Trailing modifiers and ZWJ loop (`src/utf8.cpp:297-347`)**: Starting from `cluster_w = codepoint_width(base_cp)`, loops over subsequent codepoints:
   - `0xFE0F` (Variation Selector-16): consumes and sets `cluster_w = 2`.
   - `0xFE0E` (Variation Selector-15): consumes and sets `cluster_w = 1`.
   - `0x1F3FB..0x1F3FF` (Emoji skin-tone modifiers): consumes and sets `cluster_w = 2`.
   - `0x200D` (Zero-Width Joiner): consumes `0x200D`, consumes the joined codepoint, optionally consumes a trailing `0xFE0F` or skin-tone modifier `0x1F3FB..0x1F3FF`, and sets `cluster_w = 2`.
   - Any other codepoint where `is_unicode_zero_width(next_cp)` is `true`: consumes without changing `cluster_w`.
   - Stops at the first codepoint that matches none of the above.

```cpp
std::string_view family = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6";
Grapheme g = next_grapheme(family);
// 25 UTF-8 bytes stored inline inside Grapheme::data[36], width == 2
static_cast<void>(g);
```

<!-- src: include/koto/utf8.hpp:26-127 -->
<!-- src: src/utf8.cpp:247-350 -->

## Stage 4: `Grapheme` truncation guard

Inside `Grapheme(std::string_view sv, char32_t cp, int w)` (`include/koto/utf8.hpp:77-120`), `max_bytes` is `sizeof(data) - 1` (`35` bytes). Before copying bytes from `sv` into `data`, the constructor scans UTF-8 lead bytes (`1`, `2`, `3`, or `4` bytes per codepoint) and stops accumulating `valid_len` as soon as `valid_len + cp_len > 35`. This guarantees that even an artificially long cluster with dozens of combining marks is truncated cleanly on a codepoint boundary and null-terminated.

<!-- src: include/koto/utf8.hpp:89-120 -->

## See also

- [UTF-8 and Grapheme API](/api/utf8/)
- [Unicode Guide](/guides/unicode/)
- [Rendering Pipeline Architecture](/architecture/rendering-pipeline/)
