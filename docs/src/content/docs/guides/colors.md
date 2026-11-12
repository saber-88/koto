---
title: Colors, Gradients, and Palette Quantization
description: How to use named colors, 24-bit RGB, hex strings, Color::lerp gradients, and ColorMode downscaling
sidebar:
  label: Colors
  order: 5
---

## Goal

Style widgets using `Color` constants, 24-bit RGB values, hex strings (`#RGB`, `#RRGGBB`, `#RRGGBBAA`), smooth `Color::lerp` interpolation, and terminal `ColorMode` overrides.

<!-- src: include/koto/color.hpp:10-81 -->
<!-- src: src/color.cpp:11-281 -->

## Complete working example

```cpp
#include <koto/koto.hpp>
#include <cstdio>
#include <string>

using namespace koto;

int main()
{
    App app;
    float progress = 0.35f;

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.is(Key::Escape)) {
            app.quit();
        } else if (key.is(Key::Right)) {
            progress = (progress + 0.05f > 1.0f) ? 1.0f : progress + 0.05f;
        } else if (key.is(Key::Left)) {
            progress = (progress - 0.05f < 0.0f) ? 0.0f : progress - 0.05f;
        }
    });

    app.on_render([&](Frame& frame) {
        auto [header_area, gauge_area, swatch_area] = Layout::vertical(frame.area(), {
            Constraint::fixed(3),
            Constraint::fixed(3),
            Constraint::fill(),
        });

        Box()
            .title("Color & Gradient Demo (Left/Right to adjust, q to quit)")
            .border_style(BorderStyle::Rounded)
            .fg("#00E5FF")
            .draw(frame, header_area);

        uint32_t dynamic_fg = Color::lerp(Color::Emerald, Color::Red, progress);
        int pct = static_cast<int>(progress * 100.0f);

        Box().title("Load").border_style(BorderStyle::Single).draw(frame, gauge_area);
        Gauge()
            .ratio(progress)
            .label(std::to_string(pct) + "%")
            .filled_fg(dynamic_fg)
            .empty_fg(Color::DarkGray)
            .draw(frame, gauge_area.inset(1, 1, 1, 1));

        Box()
            .title("RGB Swatch")
            .filled(true)
            .bg(Color::rgb(24, 24, 37))
            .fg("amber")
            .draw(frame, swatch_area);
    });

    if (auto res = app.run(); !res) {
        std::fprintf(stderr, "Error: %.*s\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data());
        return 1;
    }
    return 0;
}
```

## Step-by-step explanation

1. **Implicit `Color` construction**: Every widget color setter (`fg`, `bg`, `title_fg`, `highlight_fg`, etc.) accepts `koto::Color` by value. Because `Color` has implicit constructors from `uint32_t`, `std::string_view`, and `const char*` (`include/koto/color.hpp:39-41`), you can pass `Color::Emerald`, `Color::rgb(24, 24, 37)`, `"#00E5FF"`, or `"amber"` interchangeably.
2. **Validating user-supplied hex strings**: When parsing untrusted config files, call `try_parse_color(str)` or `Color::try_from_hex(str)` to receive `std::expected<uint32_t, ColorParseError>` (`EmptyString`, `InvalidLength`, `InvalidHexDigit`) instead of falling back silently to `COLOR_DEFAULT`.
3. **Interpolating gradients**: `Color::lerp(c1, c2, t)` linearly interpolates red, green, and blue channels for `t` in `[0.0f, 1.0f]` using `std::lround`.
4. **Automatic terminal downscaling**: At startup, `Terminal` initializes `m_color_mode = detect_system_color_mode()`. During `Frame::paint_cell()`, 24-bit colors are automatically emitted as `[38;2;R;G;Bm` (`TrueColor`), quantized via `rgb_to_ansi256` to `[38;5;Nm` (`Ansi256`), quantized via `rgb_to_ansi16` to `[30..37;90..97m` (`Ansi16`), or suppressed (`NoColor`) (`src/frame.cpp:243-324`).

<!-- src: src/color.cpp:11-273 -->
<!-- src: src/frame.cpp:243-324 -->

## Common mistakes

- **Calling non-existent `Color::Hex(0x...)` or `Color::from_rgb(...)`**: To construct from a packed integer literal, pass the `uint32_t` directly (`Color(0x00E5FF)` or `0x00E5FF`). For RGB channels, call `Color::rgb(r, g, b)` or `Color::Rgb(r, g, b)`. For hex strings, pass `"#00E5FF"` or call `Color::from_hex("#00E5FF")`.
- **Testing TrueColor output when `NO_COLOR` is set in the environment**: If `NO_COLOR` is non-empty in the environment, `detect_system_color_mode()` returns `ColorMode::NoColor` (`src/color.cpp:229-233`). In unit tests that inspect `Frame::raw_output()` for SGR color sequences, either use a headless `Frame(rows, cols)` (which defaults to `ColorMode::TrueColor` because `m_term == nullptr`) or explicitly call `term.color_mode(ColorMode::TrueColor)`.

## See also

- [Cell and Color API](/api/cell-color/)
- [Terminal API](/api/terminal/)
- [Troubleshooting](/deep-dives/troubleshooting/)
