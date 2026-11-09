# `koto::Box`

Versatile container widget that renders styled decorative borders (Single, Double, Rounded, Thick), aligned embedded headers, background color fills, and active focus styling.

---

## Visual Showcase & Layout Anatomy

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ ╭── [Title Align: Left] ───────────────── [Title: Center] ───────────────╮  │
│ │                                                                        │  │
│ │                         Inner Area (inset(1, 1, 1, 1))                 │  │
│ │                                                                        │  │
│ ╰────────────────────────────────────────────────────────────────────────╯  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Header & Class Synopsis

```cpp
#include <koto/box.hpp>
```

```cpp
namespace koto
{
    enum class BorderStyle
    {
        None,
        Single,   // ─ │ ┌ ┐ └ ┘
        Double,   // ═ ║ ╔ ╗ ╚ ╝
        Rounded,  // ─ │ ╭ ╮ ╰ ╯
        Thick     // ━ ┃ ┏ ┓ ┗ ┛
    };

    enum class TitleAlign
    {
        Left,
        Center,
        Right
    };

    struct Borders
    {
        bool top = true, bottom = true, left = true, right = true;
    };

    class Box : public Widget
    {
    public:
        Box();

        Box& border_style(BorderStyle style);
        Box& title(std::string title);
        Box& title_align(TitleAlign align);
        Box& title_fg(Color fg);
        Box& title_bg(Color bg);
        Box& fg(Color fg);
        Box& bg(Color bg);
        Box& filled(bool fill);
        Box& borders(Borders borders);
        Box& focused(bool is_focused);
        Box& focused_border_fg(Color fg);
        Box& focused_border_style(BorderStyle style);
        Box& id(Id tag_id);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Border Styles Reference

| Style | Top-Left | Top-Right | Bottom-Left | Bottom-Right | Horizontal | Vertical | Visual Character |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|:---|
| **`Single`** | `┌` | `┐` | `└` | `┘` | `─` | `│` | Clean technical lines |
| **`Rounded`**| `╭` | `╮` | `╰` | `╯` | `─` | `│` | Modern smooth curves (default) |
| **`Double`** | `╔` | `╗` | `╚` | `╝` | `═` | `║` | Prominent modal emphasis |
| **`Thick`**  | `┏` | `┓` | `┗` | `┛` | `━` | `┃` | Heavy weight contrast |
| **`None`**   | ` ` | ` ` | ` ` | ` ` | ` ` | ` ` | Invisible bounding frame |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`border_style()`** | `BorderStyle` | `BorderStyle::Single` | Configures the glyphs used for corners and edges. |
| **`title()`** | `std::string` | `""` | Sets the embedded header text. |
| **`title_align()`** | `TitleAlign` | `TitleAlign::Left` | Controls header alignment (`Left`, `Center`, `Right`). |
| **`title_fg()`** | `Color` | `COLOR_DEFAULT` | Sets the foreground color of the title. |
| **`title_bg()`** | `Color` | `COLOR_DEFAULT` | Sets the background color behind the title. |
| **`fg()`** | `Color` | `COLOR_DEFAULT` | Sets the color of the border lines. |
| **`bg()`** | `Color` | `COLOR_DEFAULT` | Sets the canvas background color when `filled(true)`. |
| **`filled()`** | `bool` | `false` | When `true`, fills the interior with `bg` color. |
| **`borders()`** | `Borders` | All `true` | Allows selectively disabling individual edges (`top`, `bottom`, `left`, `right`). |
| **`focused()`** | `bool` | `false` | Toggles active focus visual mode. |
| **`focused_border_fg()`** | `Color` | `Color::Cyan` | Border color used when `focused == true`. |
| **`focused_border_style()`** | `BorderStyle` | Same as `style` | Border style used when `focused == true`. |
| **`id()`** | `Id` | `Id::none()` | Stamps the entire box area in `m_hit_buffer`. |

---

## Usage Examples

### 1. Rounded Card with Title

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_card(Frame& frame, Area area)
{
    Box()
        .title("System Status")
        .title_align(TitleAlign::Left)
        .border_style(BorderStyle::Rounded)
        .fg(Color::Cyan)
        .title_fg(Color::White)
        .draw(frame, area);
}
```

### 2. Filled Dialog with Focus Highlight

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_modal(Frame& frame, Area area, bool has_focus)
{
    Box()
        .title("Confirm Changes")
        .border_style(BorderStyle::Double)
        .filled(true)
        .bg(Color::Hex(0x181825))
        .focused(has_focus)
        .focused_border_fg(Color::Yellow)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Under-Size Protection**: If `area.width < 2` or `area.height < 2`, `draw()` returns immediately without corrupting adjacent grid cells.
2. **Title Clamping**: If the title width exceeds available horizontal space, it is truncated cleanly without line wrapping.
3. **Interior Inset Helper**: Child widgets placed inside `Box` should render inside `area.inset(1, 1, 1, 1)`.

---

## C++ Language Features Utilized

1. **Designated Initializers (C++20)**: `Box().borders({ .top = false, .right = true })` provides expressive border selection.
2. **Method Chaining (`Box&`)**: Fluent builder pattern returning `*this` by reference.
3. **Scoped Strongly-Typed Enums (`enum class`) (C++11)**: `BorderStyle` and `TitleAlign`.
4. **Virtual Method Override (`override`) (C++11)**: Strict compile-time interface verification.
