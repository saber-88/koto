---
title: Cell and Color
description: Cache-line aligned 64-byte Cell struct, TextAttr flags, 24-bit Color representation, hex parsing, and ANSI quantization
sidebar:
  label: Cell and Color
  order: 4
---

## Purpose and header

`koto/cell.hpp` defines `TextAttr` styling flags and the 64-byte `Cell` struct stored in `Frame` buffers. `koto/color.hpp` defines `Color`, `ColorMode`, `ColorParseError`, named color constants, hex parsers, linear interpolation (`Color::lerp`), and ANSI 256/16 quantizers.
Headers: `koto/cell.hpp` and `koto/color.hpp` (both included by `koto/koto.hpp`).

<!-- src: include/koto/cell.hpp:1-76 -->
<!-- src: include/koto/color.hpp:1-83 -->

## Types and members

### `TextAttr` and `Cell` (`koto/cell.hpp`)

| Symbol | Kind | Description | Default |
| :--- | :--- | :--- | :--- |
| `TextAttr::None` | Enum value (`uint8_t`) | No SGR text attributes (`0`). | `0` |
| `TextAttr::Bold` | Enum value (`1 << 0`) | Bold intensity (`[1m`). | None |
| `TextAttr::Dim` | Enum value (`1 << 1`) | Dim / faint intensity (`[2m`). | None |
| `TextAttr::Italic` | Enum value (`1 << 2`) | Italic style (`[3m`). | None |
| `TextAttr::Underline` | Enum value (`1 << 3`) | Underlined text (`[4m`). | None |
| `TextAttr::Reverse` | Enum value (`1 << 4`) | Video inverse (`[7m`). | None |
| `TextAttr::Strike` | Enum value (`1 << 5`) | Strikethrough (`[9m`). | None |
| `operator\|` | `constexpr uint8_t operator\|(TextAttr a, TextAttr b) noexcept` | Combines `TextAttr` values into a `uint8_t` bitmask. | None |
| `Cell::grapheme` | Field (`Grapheme`) | Inline 44-byte UTF-8 grapheme cluster (`char data[36]` + metadata). | `" "` |
| `Cell::ch` | Field (`char32_t`) | Base Unicode codepoint. | `U' '` |
| `Cell::fg` / `Cell::bg` | Fields (`uint32_t`) | Foreground and background packed RGB colors. | `COLOR_DEFAULT` |
| `Cell::width` | Field (`uint8_t`) | Display column width (`0`, `1`, or `2`). | `1` |
| `Cell::continuation` | Field (`bool`) | Marks the trailing cell of a 2-column glyph. | `false` |
| `Cell::attrs` | Field (`uint8_t`) | Bitmask of `TextAttr` flags. | `0` |
| `Cell::reserved[5]` | Field (`uint8_t[5]`) | Padding ensuring `sizeof(Cell) == 64`. | `{}` |
| `Cell::view()` | `constexpr std::string_view view() const noexcept` | Returns `grapheme.view()`. | `" "` |
| `Cell::make_continuation()` | `static constexpr Cell make_continuation() noexcept` | Returns a `Cell` with `continuation = true`. | None |

<!-- src: include/koto/cell.hpp:9-75 -->

### `Color`, `ColorMode`, and `ColorParseError` (`koto/color.hpp`)

| Symbol | Signature / Value | Description | Default |
| :--- | :--- | :--- | :--- |
| `COLOR_DEFAULT` | `inline constexpr uint32_t COLOR_DEFAULT = 0xFF000000` | Terminal default foreground/background sentinel (`[39m` / `[49m`). | `0xFF000000` |
| `ColorMode` | `enum class ColorMode : uint8_t { NoColor = 0, Ansi16 = 1, Ansi256 = 2, TrueColor = 3 }` | Output color mode used by `Frame::render()`. | `TrueColor` |
| `ColorParseError` | `enum class ColorParseError { EmptyString, InvalidLength, InvalidHexDigit }` | Error code returned by `try_parse_color()` and `Color::try_from_hex()`. | None |
| `try_parse_color(color)` | `auto try_parse_color(std::string_view color) -> std::expected<uint32_t, ColorParseError>` | Parses case-insensitive color names or `#RGB`, `#RRGGBB`, `#RRGGBBAA` hex strings (with or without leading `#`). | None |
| `parse_color(color)` | `auto parse_color(std::string_view color) -> uint32_t` | Returns `try_parse_color(color).value_or(COLOR_DEFAULT)`. | `COLOR_DEFAULT` |
| `rgb_to_ansi256(r, g, b)` | `uint8_t rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b) noexcept` | Maps RGB to 256-color grayscale (`232..255`) or 6x6x6 cube (`16..231`). | None |
| `rgb_to_ansi16(r, g, b)` | `uint8_t rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b) noexcept` | Maps RGB to closest standard 16-color index (`0..15`). | None |
| `detect_system_color_mode()` | `ColorMode detect_system_color_mode() noexcept` | Checks `NO_COLOR`, `TERM=dumb`, `COLORTERM=truecolor\|24bit`, and `TERM` suffixes. | `ColorMode::TrueColor` |
| `Color::value` | `uint32_t value = COLOR_DEFAULT` | Packed `0x00RRGGBB` value or `COLOR_DEFAULT`. | `COLOR_DEFAULT` |
| `Color(uint32_t)` / `Color(string_view)` / `Color(const char*)` | Constructors | Implicitly constructs `Color` from packed integer, hex string, or named color string. | `COLOR_DEFAULT` |
| `Color::is_default()` | `constexpr bool is_default() const noexcept` | Returns `value == COLOR_DEFAULT`. | `true` on default `Color{}` |
| `Color::rgb(r, g, b)` / `Color::Rgb(r, g, b)` | `static constexpr uint32_t rgb(uint8_t r, uint8_t g, uint8_t b) noexcept` | Packs 8-bit `r`, `g`, `b` channels into `0x00RRGGBB`. | None |
| `Color::try_from_hex(hex)` / `Color::from_hex(hex)` | Static methods | Delegates to `try_parse_color(hex)` and `parse_color(hex)`. | None |
| `Color::lerp(c1, c2, t)` | `static uint32_t lerp(uint32_t c1, uint32_t c2, float t) noexcept` | Linearly interpolates RGB channels between `c1` and `c2` (or `hex1` and `hex2`). | None |

<!-- src: include/koto/color.hpp:10-81 -->
<!-- src: src/color.cpp:11-281 -->

### Named color constants (`Color::*`)

| Constant | Hex value | Recognized string names in `try_parse_color` (case-insensitive) |
| :--- | :--- | :--- |
| `Color::Default` / `Color::Reset` | `0xFF000000` (`COLOR_DEFAULT`) | `"default"`, `"reset"` |
| `Color::Black` | `0x00000000` | `"black"` |
| `Color::White` | `0x00FFFFFF` | `"white"` |
| `Color::Red` | `0x00FF0000` | `"red"` |
| `Color::Green` | `0x0000FF00` | `"green"` |
| `Color::Blue` | `0x000000FF` | `"blue"` |
| `Color::Yellow` | `0x00FFFF00` | `"yellow"` |
| `Color::Cyan` | `0x0000FFFF` | `"cyan"` |
| `Color::Magenta` | `0x00FF00FF` | `"magenta"` |
| `Color::Gray` | `0x00808080` | `"gray"`, `"grey"` |
| `Color::DarkGray` | `0x00404040` | `"darkgray"`, `"darkgrey"`, `"dark_gray"` |
| `Color::LightGray` | `0x00C0C0C0` | `"lightgray"`, `"lightgrey"`, `"light_gray"` |
| `Color::Orange` | `0x00FFA500` | `"orange"` |
| `Color::Emerald` | `0x0050C878` | `"emerald"` |
| `Color::Teal` | `0x00008080` | `"teal"` |
| `Color::Amber` | `0x00FFBF00` | `"amber"` |

<!-- src: include/koto/color.hpp:47-63 -->
<!-- src: src/color.cpp:29-45 -->

## Examples

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

int main()
{
    Color c_named = "emerald";
    Color c_hex3  = "#F00";
    Color c_hex8  = "#FF000080"; // Alpha byte is ignored
    assert(c_named.value == Color::Emerald);
    assert(c_hex3.value == Color::Red);
    assert(c_hex8.value == Color::Red);

    uint8_t attrs = TextAttr::Bold | TextAttr::Underline;
    Cell cell{U'A', Color::lerp(Color::Emerald, Color::Amber, 0.5f), Color::Black, 1, false, attrs};
    assert(sizeof(cell) == 64);
    return 0;
}
```

## Errors and edge cases

- **8-digit hex alpha stripping**: `try_parse_color("#RRGGBBAA")` extracts the top 3 bytes (`R`, `G`, `B`) and discards the trailing alpha byte so an 8-digit hex string with `0xFF` in its highest byte never collides with `COLOR_DEFAULT` (`0xFF000000`).
- **Invalid color strings**: `parse_color()` and `Color(std::string_view)` fall back silently to `COLOR_DEFAULT` on invalid input. Use `try_parse_color()` or `Color::try_from_hex()` when you need `ColorParseError` diagnostics.
- **`Color::lerp` with `COLOR_DEFAULT`**: Since `COLOR_DEFAULT` has no RGB coordinates, `Color::lerp(COLOR_DEFAULT, c2, t)` steps discretely at `t = 0.5f` (`t < 0.5f` returns `COLOR_DEFAULT`; `t >= 0.5f` returns `c2`).

<!-- src: src/color.cpp:86-128 -->

## See also

- [Frame](/api/frame/)
- [UTF-8 and Grapheme](/api/utf8/)
- [Colors Guide](/guides/colors/)
