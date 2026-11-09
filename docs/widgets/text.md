# `koto::Text`

Versatile text rendering widget supporting Unicode 16.0 extended grapheme clusters, multi-line hard breaks, horizontal alignments (Left, Center, Right), word wrapping, and ellipsis truncation.

---

## Visual Showcase & Layout Anatomy

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ Left Aligned:     [Text begins at left boundary and pads right            ] │
│ Center Aligned:   [             Symmetrically Centered Text               ] │
│ Right Aligned:    [                             Text hugs the right border] │
│ Ellipsis Mode:    [Text that exceeds the allotted column width is cut…    ] │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Header & Class Synopsis

```cpp
#include <koto/text.hpp>
```

```cpp
namespace koto
{
    enum class TextAlign
    {
        Left,
        Center,
        Right
    };

    enum class TextOverflow
    {
        Clip,
        Ellipsis
    };

    enum class TextWrap
    {
        None,
        Wrap
    };

    class Text : public Widget
    {
    public:
        Text(std::string_view content = "");

        Text& align(TextAlign a);
        Text& wrap();
        Text& clip();
        Text& ellipsis();
        Text& overflow(TextOverflow o);

        // Colors & Attributes
        Text& fg(Color fg);
        Text& bg(Color bg);
        Text& bold(bool b = true);
        Text& dim(bool d = true);
        Text& italic(bool i = true);
        Text& underline(bool u = true);
        Text& reverse(bool r = true);
        Text& attr(TextAttr a);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`align()`** | `TextAlign` | `TextAlign::Left` | Controls horizontal text alignment (`Left`, `Center`, `Right`). |
| **`wrap()`** | (None) | None | Enables automatic soft word-wrapping at whitespace. |
| **`clip()`** | (None) | Default | Hard-clips overflow at the right boundary without ellipsis. |
| **`ellipsis()`** | (None) | None | Truncates overflowing lines with `…`. |
| **`overflow()`** | `TextOverflow` | `TextOverflow::Clip` | Explicitly configures overflow mode (`Clip` or `Ellipsis`). |
| **`fg()`** | `Color` | `COLOR_DEFAULT` | Sets text foreground color. |
| **`bg()`** | `Color` | `COLOR_DEFAULT` | Sets text background color. |
| **`bold()`** | `bool` | `true` | Toggles bold font attribute. |
| **`dim()`** | `bool` | `true` | Toggles dim / faint text attribute. |
| **`italic()`** | `bool` | `true` | Toggles italic text attribute. |
| **`underline()`**| `bool` | `true` | Toggles underline attribute. |
| **`reverse()`** | `bool` | `true` | Swaps foreground and background colors. |

---

## Usage Examples

### 1. Centered Header with Styling

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_header(Frame& frame, Area area)
{
    Text("Dashboard Overview 🚀")
        .align(TextAlign::Center)
        .bold()
        .underline()
        .fg(Color::Yellow)
        .draw(frame, area);
}
```

### 2. Auto-Wrapped Paragraph with Ellipsis Fallback

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_paragraph(Frame& frame, Area area, std::string_view description)
{
    Text(description)
        .wrap()
        .fg(Color::White)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Zero Heap Allocation on Literals**: The constructor accepts `std::string_view`, allowing string literals to be rendered without string allocation.
2. **Grapheme Atomic Preservation**: Multi-byte emojis and CJK ideographs are never sliced in half; continuation cells and display widths are honored automatically.
3. **Hard Newlines Respected**: Embedded newline characters (`\n`) advance the cursor to the next row at `area.col`.

---

## C++ Language Features Utilized

1. **`std::string_view` (C++17)**: Eliminates deep string copies across widget drawing passes.
2. **Fluent Builder Pattern (`Text&`)**: Method chaining returning `*this` by reference.
3. **Bitwise Attribute Composition (C++11)**: Combines styling flags using `TextAttr`.
4. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
