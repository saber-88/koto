---
title: Unicode, CJK, and Emoji Handling
description: How to measure display widths, iterate extended grapheme clusters, and handle wide characters in koto
sidebar:
  label: Unicode and CJK
  order: 6
---

## Goal

Measure visual column widths with `text_width()`, segment multi-codepoint emoji and CJK strings with `next_grapheme()`, and navigate UTF-8 buffers on grapheme boundaries with `prev_grapheme_offset()` and `next_grapheme_offset()`.

<!-- src: include/koto/utf8.hpp:1-145 -->
<!-- src: src/utf8.cpp:218-425 -->

## Complete working example

```cpp
#include <koto/koto.hpp>
#include <cassert>
#include <string>
#include <string_view>

using namespace koto;

int main()
{
    // U+7434 CJK ideograph (3 UTF-8 bytes, 2 columns) + space + ZWJ sequence (U+1F469 U+200D U+1F4BB, 11 UTF-8 bytes, 2 columns)
    std::string sample = "\xE7\x90\xB4 " "\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x92\xBB";

    // 1. Visual column width is 5 (2 + 1 + 2), whereas byte length is 15
    assert(sample.size() == 15);
    assert(text_width(sample) == 5);

    // 2. Iterate extended grapheme clusters without heap allocation
    std::string_view stream = sample;
    Grapheme g1 = next_grapheme(stream);
    Grapheme g2 = next_grapheme(stream);
    Grapheme g3 = next_grapheme(stream);
    assert(g1.width == 2 && g1.size == 3);
    assert(g2.width == 1 && g2.size == 1);
    assert(g3.width == 2 && g3.size == 11);
    assert(stream.empty());

    // 3. Atomic backspace from end of string erases the entire 11-byte ZWJ cluster
    size_t prev = prev_grapheme_offset(sample, sample.size());
    assert(prev == 4);
    sample.erase(prev);
    assert(sample == "\xE7\x90\xB4 ");

    // 4. Render into a Frame and verify continuation cell at column 1
    Frame frame(2, 10);
    Text(sample).draw(frame, frame.area());
    assert(frame.get_cell(0, 0).width == 2);
    assert(frame.get_cell(0, 1).continuation);
    return 0;
}
```

## Step-by-step explanation

1. **Measuring display columns (`text_width`)**: Never use `std::string::size()` to compute terminal column offsets. `text_width(std::string_view)` segments the string with `next_grapheme()` and sums each cluster's display width (`0`, `1`, or `2`), expanding `\t` to 4-column tab stops (`src/utf8.cpp:352-371`).
2. **Grapheme cluster segmentation (`next_grapheme`)**: `next_grapheme(std::string_view& in)` consumes a base codepoint and any trailing variation selectors (`U+FE0F` forces width `2`; `U+FE0E` forces width `1`), skin-tone modifiers (`U+1F3FB..U+1F3FF`), keycap sequences (`'0'..'9'`, `'#'`, `'*'` + `U+FE0F` + `U+20E3`), regional indicator flag pairs (`U+1F1E6..U+1F1FF`), Zero-Width Joiner (`U+200D`) sequences, and zero-width combining marks (`src/utf8.cpp:247-350`).
3. **Grapheme-safe cursor movement**: `prev_grapheme_offset(text, cursor)` and `next_grapheme_offset(text, cursor)` return the exact byte boundaries of the adjacent grapheme cluster (`src/utf8.cpp:373-399`), which `TextInputState` and `TextAreaState` use for `move_left()`, `move_right()`, `backspace()`, and `delete_forward()`.

<!-- src: src/utf8.cpp:247-425 -->

## Common mistakes

- **Ambiguous Dingbats and Miscellaneous Symbols without `U+FE0F`**: Per Unicode 16.0 UAX #11 and UTS #51, codepoints such as `U+2714` (heavy check mark), `U+2605` (black star), `U+1F5D1` (wastebasket), and `U+2139` (information source) have default text presentation (`width == 1`), whereas appending Variation Selector-16 (`U+FE0F`) promotes them to emoji presentation (`width == 2`) (`tests/test8_text_cjk_emoji.cpp:119-140`).
- **Writing a width-2 character at the last column (`col == cols - 1`)**: Because a 2-column character cannot fit in 1 remaining column without wrapping the hardware terminal cursor, `Frame::set_cell` replaces any width-2 cell at `col + 1 >= m_cols` with a single space `Cell{U' '}` of width `1` (`src/frame.cpp:101-105`).

## See also

- [UTF-8 and Grapheme API](/api/utf8/)
- [Unicode Deep Dive](/deep-dives/unicode/)
- [Frames and Diff Renderer Concept](/concepts/frames-diff-renderer/)
