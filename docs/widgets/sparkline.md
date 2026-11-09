# `koto::Sparkline`

High-density data visualization widget rendering time-series metrics and telemetry trends into single-row graphs using 8 vertical Unicode sub-block characters (`U+2581` through `U+2588`).

---

## Visual Showcase & Layout Anatomy

```
Telemetry Visualization Sample:
[  ▂▃▄▅▆▇██▇▆▅▄▃▂  ]

Right-Aligned (Newest data hugs the right edge):
[          ▂▃▄▅▆▇██]

Left-Aligned:
[ ▂▃▄▅▆▇██          ]
```

---

## Header & Class Synopsis

```cpp
#include <koto/sparkline.hpp>
```

```cpp
namespace koto
{
    class Sparkline : public Widget
    {
    public:
        Sparkline();

        Sparkline& data(std::span<const uint64_t> values);
        Sparkline& data(const std::vector<uint64_t>& values);
        Sparkline& max(uint64_t max_value);
        Sparkline& align_right(bool right = true);

        Sparkline& fg(Color fg);
        Sparkline& bg(Color bg);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Vertical Block Quantization Reference

| Level | Value Range | Glyph | Codepoint | Description |
|:---:|:---:|:---:|:---|
| `0` | $\text{val} = 0$ | ` ` | `U+0020` | Empty space |
| `1` | $0 < \text{val} \le \frac{1}{8} \text{max}$ | ` ` | `U+2581` | Lower one-eighth block |
| `2` | $\le \frac{2}{8} \text{max}$ | `▂` | `U+2582` | Lower one-quarter block |
| `3` | $\le \frac{3}{8} \text{max}$ | `▃` | `U+2583` | Lower three-eighths block |
| `4` | $\le \frac{4}{8} \text{max}$ | `▄` | `U+2584` | Lower half block |
| `5` | $\le \frac{5}{8} \text{max}$ | `▅` | `U+2585` | Lower five-eighths block |
| `6` | $\le \frac{6}{8} \text{max}$ | `▆` | `U+2586` | Lower three-quarters block |
| `7` | $\le \frac{7}{8} \text{max}$ | `▇` | `U+2587` | Lower seven-eighths block |
| `8` | $\approx \text{max}$ | `█` | `U+2588` | Full block |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`data()`** | `std::span<const uint64_t>` | Empty | Zero-allocation non-owning view over contiguous metrics. |
| **`data()`** | `const std::vector<uint64_t>&` | Empty | Owning data copy from a dynamic vector. |
| **`max()`** | `uint64_t` | `0` (auto) | Fixed scaling ceiling. If `0`, dynamically scales to maximum value in series. |
| **`align_right()`**| `bool` | `true` | When `true`, newest values hug the right margin. |
| **`fg()`** | `Color` | `Color::Cyan` | Sets the color of the sparkline bars. |
| **`bg()`** | `Color` | `COLOR_DEFAULT` | Sets the background behind the sparkline. |

---

## Usage Examples

### 1. Telemetry Ring Buffer with `std::span` (Zero Allocation)

```cpp
#include <koto/koto.hpp>
#include <array>
#include <span>

using namespace koto;

void render_latency_graph(Frame& frame, Area area)
{
    static std::array<uint64_t, 64> latency_history{};
    static size_t count = 0;

    // Pass non-owning view directly without copying or allocating memory
    std::span<const uint64_t> slice(latency_history.data(), count);

    Sparkline()
        .data(slice)
        .max(100) // Fixed 100ms ceiling
        .fg(Color::Yellow)
        .align_right(true)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Automatic History Slicing**: If `data.size() > area.width`, `Sparkline` automatically renders the latest $W$ points, dropping older data points without reallocating buffers.
2. **Bottom-Row Rendering**: When placed in an area of height $> 1$, `Sparkline` draws on the bottom row (`area.bottom() - 1`) and clears upper rows.
3. **Division by Zero Protection**: If all data points are $0$ and `max == 0`, `effective_max` defaults safely to $1$.

---

## C++ Language Features Utilized

1. **`std::span` (C++20)**: Non-owning, bounds-safe contiguous memory slicing for telemetry and data arrays.
2. **Function Overloading for Data Ingestion**: Clean support for both `std::span` and `std::vector`.
3. **`constexpr std::array` (C++17)**: 8-element compile-time sub-block character table (`SPARKLINE_BARS`).
4. **Explicit Narrowing Casts (`static_cast`) (C++11)**: Strict compliance with compiler sign and narrowing diagnostics.
5. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
