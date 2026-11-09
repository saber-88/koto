# `koto::Gauge`

High-precision progress bar widget supporting smooth 1/8th sub-block fractional rendering, custom fill/empty glyphs, and centered overlay labels that preserve underlying bar colors.

---

## Visual Showcase & Layout Anatomy

```
Standard Full-Cell (20 cols, 5% steps):
[████████████        ]  60%

koto Sub-Block (20 cols * 8 = 160 steps, 0.625% precision):
[████████████▌       ]  62.5%

Centered Label with Background Color Preservation:
[██████████  72%     ]
```

---

## Header & Class Synopsis

```cpp
#include <koto/gauge.hpp>
```

```cpp
namespace koto
{
    class Gauge : public Widget
    {
    public:
        Gauge();

        Gauge& ratio(float r);
        Gauge& percent(int p);
        Gauge& label(std::string_view label);
        Gauge& filled_char(char32_t ch);
        Gauge& empty_char(char32_t ch);
        Gauge& use_fractional_blocks(bool enable);

        Gauge& fg(Color fg);
        Gauge& bg(Color bg);

        Gauge& filled_fg(Color fg);
        Gauge& filled_bg(Color bg);
        Gauge& empty_fg(Color fg);
        Gauge& empty_bg(Color bg);
        Gauge& label_fg(Color fg);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Fractional 1/8th Sub-Blocks Reference

| Remainder | Glyph | Codepoint | Description |
|:---:|:---:|:---:|:---|
| `0/8` | ` ` | `U+0020` | Empty whitespace |
| `1/8` | `▏` | `U+258F` | Left one-eighth block |
| `2/8` | `▎` | `U+258E` | Left one-quarter block |
| `3/8` | `▍` | `U+258D` | Left three-eighths block |
| `4/8` | `▌` | `U+258C` | Left half block |
| `5/8` | `▋` | `U+258B` | Left five-eighths block |
| `6/8` | `▊` | `U+258A` | Left three-quarters block |
| `7/8` | `▉` | `U+2589` | Left seven-eighths block |
| `8/8` | `█` | `U+2588` | Full block |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`ratio()`** | `float` | `0.0f` | Sets progress ratio clamped to $[0.0, 1.0]$. |
| **`percent()`** | `int` | `0` | Convenience helper for `ratio(p / 100.0f)`. |
| **`label()`** | `std::string_view` | `""` | Centered text overlay. |
| **`filled_char()`** | `char32_t` | `U'█'` | Custom fill character (disables fractional blocks). |
| **`empty_char()`** | `char32_t` | `U' '` | Background character for unfilled portion. |
| **`use_fractional_blocks()`**| `bool` | `true` | Toggles 1/8th sub-block precision. |
| **`filled_fg()`** | `Color` | `Color::Emerald` | Foreground color of filled portion. |
| **`filled_bg()`** | `Color` | `COLOR_DEFAULT` | Background color behind filled blocks. |
| **`empty_fg()`** | `Color` | `Color::DarkGray` | Foreground color of unfilled cells. |
| **`empty_bg()`** | `Color` | `COLOR_DEFAULT` | Background color behind unfilled cells. |
| **`label_fg()`** | `Color` | `Color::White` | Text color of the centered label. |

---

## Usage Examples

### 1. Progress Bar with Percentage Label

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_progress(Frame& frame, Area area, float progress)
{
    int pct = static_cast<int>(progress * 100.0f);
    std::string text = std::format("{}%", pct);

    Gauge()
        .ratio(progress)
        .label(text)
        .filled_fg(Color::Emerald)
        .empty_fg(Color::DarkGray)
        .label_fg(Color::White)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Background Color Preservation Under Label**: When drawing the overlay label, `frame.get_cell(row, col).bg` is preserved so text floating across the progress threshold does not distort the bar.
2. **Sub-Cell Precision**: Multiplies resolution by $8\times$, delivering smooth animations even in narrow progress bars.
3. **Ratio Clamping**: Inputs are strictly bounded into $[0.0f, 1.0f]$ via `std::clamp`.

---

## C++ Language Features Utilized

1. **`std::clamp` (C++17)**: Bounds input ratios to $[0.0f, 1.0f]$.
2. **`std::lround` (C++11)**: Deterministic rounding to nearest sub-cell integer without truncation bias.
3. **`constexpr std::array` (C++17)**: Compile-time 9-element sub-block character table (`FRACTIONAL_BLOCKS`).
4. **`std::string_view` (C++17)**: Zero-copy label passing without heap allocation.
5. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
