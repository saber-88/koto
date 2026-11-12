---
title: Text
description: Multi-line Unicode text widget supporting Left/Center/Right alignment, word wrapping, clipping, ellipsis, and SGR attributes
sidebar:
  label: Text
  order: 10
---

`koto::Text` renders single-line or multi-line UTF-8 text into an `area` with configurable horizontal alignment, soft word wrapping, overflow clipping or ellipsis truncation, and `TextAttr` styling (Header: `koto/text.hpp`).

## Quick example

```cpp title="Styled text" render="32x2"
Text("koto C++23 Terminal UI\nSecond line")
    .align(TextAlign::Center)
    .fg(Color::Cyan)
    .bold()
    .draw(frame, text_area);
```

```text frame="terminal" title="Output"
     koto C++23 Terminal UI
          Second line
```

## Behavior

- **Hard line breaks (`\n`)**: `Text::draw()` splits `m_content` on `\n` first (`src/text.cpp:104-106`) and ignores `\r` codepoints (`src/text.cpp:191, 238`).
- **Horizontal alignment**: For each paragraph line, when `m_align` is `TextAlign::Center` or `TextAlign::Right` and `text_width(line) < area.width`, `start_col` is offset to `area.col + (area.width - line_w) / 2` or `area.col + (area.width - line_w)` (`src/text.cpp:110-125`).
- **No-wrap mode (`TextWrap::None`, default)**:
  - Tabs (`\t`) advance `col` to the next 4-column tab stop relative to `area.col`, filling intermediate cells with spaces (`src/text.cpp:243-255`).
  - When a grapheme would exceed `area.right()` (`col + w > area.right()`), if `m_overflow == TextOverflow::Ellipsis` and `area.right() > area.col`, `Text` overwrites the last cell on the row (`area.right() - 1`) with `Cell{U'…', m_fg, m_bg}` and skips the rest of the line (`src/text.cpp:260-268`).
- **Word-wrap mode (`.wrap()` / `TextWrap::Wrap`)**:
  - Scans tokens of contiguous spaces/tabs (`' '` or `'\t'`) or contiguous non-whitespace graphemes (`src/text.cpp:142-168`).
  - Leading spaces/tabs at the start of a wrapped row (`col == area.col`) are skipped (`src/text.cpp:136-140`).
  - If a word token does not fit on the current line (`col + token_width > area.right() && col > area.col`), `Text` advances `row++` and resets `col = area.col` before drawing the word. If `row >= area.bottom()` and `m_overflow == TextOverflow::Ellipsis`, it writes `U'…'` at `(row - 1, area.right() - 1)` and returns (`src/text.cpp:171-183`).
  - Words longer than `area.width` wrap character-by-character at `area.right()`.

<!-- src: include/koto/text.hpp:11-61 -->
<!-- src: src/text.cpp:13-278 -->

## Methods

| Method | Description | Default |
| :--- | :--- | :--- |
| `Text(std::string_view content = "")` | Constructs a `Text` widget with `content` copied into `m_content`. | `content = ""` |
| `fg(Color fg)` | Sets the text foreground color. | `COLOR_DEFAULT` |
| `bg(Color bg)` | Sets the text background color (applied only to cells where glyphs/spaces are drawn). | `COLOR_DEFAULT` |
| `wrap()` | Enables soft word-wrapping (`m_text_wrap = TextWrap::Wrap`). | `TextWrap::None` |
| `clip()` | Sets overflow mode to `TextOverflow::Clip`. | `TextOverflow::Clip` |
| `ellipsis()` | Sets overflow mode to `TextOverflow::Ellipsis` (`U'…'`). | `TextOverflow::Clip` |
| `overflow(TextOverflow o)` | Explicitly sets `TextOverflow::Clip` or `TextOverflow::Ellipsis`. | `TextOverflow::Clip` |
| `align(TextAlign a)` | Sets horizontal alignment (`TextAlign::Left`, `Center`, `Right`). | `TextAlign::Left` |
| `bold(bool b = true)` | Sets or clears `TextAttr::Bold`. | `false` |
| `dim(bool d = true)` | Sets or clears `TextAttr::Dim`. | `false` |
| `italic(bool i = true)` | Sets or clears `TextAttr::Italic`. | `false` |
| `underline(bool u = true)` | Sets or clears `TextAttr::Underline`. | `false` |
| `reverse(bool r = true)` | Sets or clears `TextAttr::Reverse`. | `false` |
| `attr(TextAttr a)` | Bitwise-ORs `a` into `m_attrs` (e.g. `TextAttr::Strike`). | `0` |
| `draw(Frame& frame, Area area)` | Renders the text into `frame` within `area`. | None |

<!-- src: include/koto/text.hpp:30-61 -->

## Gotchas

- **Strikethrough requires `.attr(TextAttr::Strike)`**: `Text` provides convenience methods `.bold()`, `.dim()`, `.italic()`, `.underline()`, and `.reverse()`, while `TextAttr::Strike` is applied via `.attr(TextAttr::Strike)`.
- **`Text` does not fill trailing or leading blank cells**: `Text::draw()` only writes cells occupied by characters in `m_content`. If you want the entire `area` filled with a background color, draw a `Fill(bg)` or `Box().filled(true).bg(bg)` first.
- **`Text` copies `std::string_view` into `std::string m_content`**: Although the constructor takes `std::string_view`, `m_content` is a `std::string` member (`include/koto/text.hpp:56`).

## See also

- [TextInput](/widgets/text-input/)
- [TextArea](/widgets/text-area/)
- [UTF-8 and Grapheme](/api/utf8/)
