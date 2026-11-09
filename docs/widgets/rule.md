# `koto::Rule`

Visual divider widget rendering a styled horizontal divider rule across a rectangular bounding box with an optional embedded, padded text title.

---

## Visual Showcase & Layout Anatomy

```
TitleAlign::Left:
── Section Title ─────────────────────────────────────────────────────────────

TitleAlign::Center:
────────────────────────────── Section Title ─────────────────────────────────

TitleAlign::Right:
───────────────────────────────────────────── Section Title ──
```

---

## Header & Class Synopsis

```cpp
#include <koto/rule.hpp>
```

```cpp
namespace koto
{
    class Rule : public Widget
    {
    public:
        explicit Rule(std::string_view title = "");

        Rule& title(std::string_view title);
        Rule& title_align(TitleAlign align);
        Rule& style(BorderStyle style);
        Rule& fg(Color fg);
        Rule& bg(Color bg);
        Rule& title_fg(Color fg);
        Rule& title_bg(Color bg);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Border Styles Reference

| `BorderStyle` | Glyph | Codepoint | Visual Appearance |
|:---|:---:|:---:|:---|
| **`BorderStyle::Single`** | `─` | `U+2500` | Thin continuous line (default) |
| **`BorderStyle::Rounded`**| `─` | `U+2500` | Thin continuous line (matches rounded box edges) |
| **`BorderStyle::Double`** | `═` | `U+2550` | Double parallel horizontal line |
| **`BorderStyle::Thick`**  | `━` | `U+2501` | Heavy horizontal bar |
| **`BorderStyle::None`**   | ` ` | `U+0020` | Blank whitespace divider |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`title()`** | `std::string_view` | `""` | Sets the embedded text label. |
| **`title_align()`** | `TitleAlign` | `TitleAlign::Center` | Controls horizontal placement of the title (`Left`, `Center`, `Right`). |
| **`style()`** | `BorderStyle` | `BorderStyle::Single` | Controls the horizontal line drawing glyph. |
| **`fg()`** | `Color` | `COLOR_DEFAULT` | Sets the color of the horizontal rule. |
| **`bg()`** | `Color` | `COLOR_DEFAULT` | Sets the background color behind the rule. |
| **`title_fg()`** | `Color` | `COLOR_DEFAULT` | Sets the color of the title text (falls back to `fg`). |
| **`title_bg()`** | `Color` | `COLOR_DEFAULT` | Sets the background color of the title text (falls back to `bg`). |

---

## Usage Examples

### 1. Centered Section Title with Double Border

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_section_header(Frame& frame, Area area)
{
    Rule("Active Deployments")
        .style(BorderStyle::Double)
        .title_align(TitleAlign::Center)
        .fg(Color::Cyan)
        .title_fg(Color::White)
        .draw(frame, area);
}
```

### 2. Form Field Separation

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_form_separator(Frame& frame, Area area)
{
    Rule("Database Settings")
        .style(BorderStyle::Thick)
        .title_align(TitleAlign::Left)
        .fg(Color::Yellow)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Vertical Centering**: For an area of height $H \ge 1$, the rule is drawn at `area.row + (H - 1) / 2`.
2. **Overflow Fallback**: If the padded title exceeds `area.width`, the title is dropped automatically and a solid line is preserved.
3. **Grapheme Measurement**: Title width is evaluated using `text_width()`, accurately accounting for wide Unicode CJK and emoji characters.

---

## C++ Language Features Utilized

1. **`std::string_view` (C++17)**: High-performance string passing without heap allocation.
2. **Method Chaining (`Rule&`)**: Fluent builder pattern returning `*this` by reference.
3. **`char32_t` Unicode Codepoints (C++11)**: Native UTF-32 literals (`U'─'`, `U'═'`, `U'━'`).
4. **Strongly-Typed Scoped Enums (`enum class`) (C++11)**: `BorderStyle` and `TitleAlign`.
5. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
