# `koto::Color`

Value type representing 24-bit TrueColor RGB values, semantic theme constants, compile-time linear interpolation (`lerp`), and automatic ANSI 256/16 palette downscaling.

---

## Internal Memory Representation

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 32-Bit Packed Unsigned Integer (0x00RRGGBB)                                 │
├──────────────┬──────────────────┬──────────────────┬────────────────────────┤
│ Bits 31..24  │ Bits 23..16      │ Bits 15..8       │ Bits 7..0              │
│ Alpha / Tag  │ Red Channel (R)  │ Green Channel (G)│ Blue Channel (B)       │
└──────────────┴──────────────────┴──────────────────┴────────────────────────┘
```

---

## Header & Class Synopsis

```cpp
#include <koto/color.hpp>
```

```cpp
namespace koto
{
    class Color
    {
    public:
        // Standard Semantic Palette
        static const Color Default;
        static const Color Reset;
        static const Color Black;
        static const Color Red;
        static const Color Green;
        static const Color Yellow;
        static const Color Blue;
        static const Color Magenta;
        static const Color Cyan;
        static const Color White;
        static const Color DarkGray;
        static const Color LightGray;
        static const Color Emerald;

        // Compile-Time Factories
        static constexpr Color Rgb(uint8_t r, uint8_t g, uint8_t b) noexcept;
        static constexpr Color Hex(uint32_t hex) noexcept;

        // Gradients & Math
        static constexpr Color lerp(Color a, Color b, float t) noexcept;

        // Raw Value & Implicit Conversion
        [[nodiscard]] constexpr uint32_t value() const noexcept;
        constexpr operator uint32_t() const noexcept;

        bool operator==(const Color&) const = default;
    };

    // Color Quantization Helpers
    uint8_t rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b) noexcept;
    uint8_t rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b) noexcept;
}
```

---

## Member Functions Reference

| Method | Signature | Description |
|:---|:---|:---|
| **`Rgb()`** | `constexpr Color Rgb(uint8_t r, uint8_t g, uint8_t b) noexcept` | Constructs a color from discrete red, green, and blue 8-bit channels. |
| **`Hex()`** | `constexpr Color Hex(uint32_t hex) noexcept` | Constructs a color from a hexadecimal literal (e.g. `0xFF5733`). |
| **`lerp()`**| `constexpr Color lerp(Color a, Color b, float t) noexcept` | Linearly interpolates between colors $a$ and $b$ for smooth animation gradients. |
| **`value()`**| `constexpr uint32_t value() const noexcept` | Returns the raw packed 32-bit integer representation. |

---

## Usage Examples

### 1. 24-Bit TrueColor RGB Construction

```cpp
#include <koto/koto.hpp>

using namespace koto;

void color_samples()
{
    Color vibrant_orange = Color::Rgb(255, 120, 0);
    Color electric_blue  = Color::Hex(0x00E5FF);
}
```

### 2. Smooth Linear Gradient Animations (`Color::lerp`)

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_dynamic_bar(Frame& frame, Area area, float progress)
{
    // Transition smoothly from Emerald Green (healthy) to Red (critical)
    Color bar_color = Color::lerp(Color::Emerald, Color::Red, progress);

    Gauge()
        .ratio(progress)
        .filled_fg(bar_color)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Zero Runtime Overhead (`constexpr`)**: Color construction, hex decoding, and linear interpolation (`lerp`) are fully evaluable at compile time.
2. **Transparent Palette Downscaling**: When applications specify RGB TrueColor, `libkoto` automatically detects host terminal capability and selects the closest 256-color or 16-color ANSI match without code modifications.
3. **Register-Sized Footprint**: At 4 bytes, `Color` passes directly in a single CPU register with zero heap or pointer indirection.

---

## C++ Language Features Utilized

1. **`constexpr` Mathematics (C++14/C++20)**: `Color::lerp()` executes at compile time for constant gradients.
2. **Implicit Conversion Operator (`constexpr operator uint32_t()`)**: Allows seamless interoperation with low-level cell buffers.
3. **Defaulted Equality (`operator== = default`) (C++20)**: Synthesizes bitwise equality checks.
4. **`noexcept` Guarantees (C++11)**: All color manipulation routines are non-throwing.
