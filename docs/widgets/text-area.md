# `koto::TextArea` & `koto::TextAreaState`

Full-featured multi-line text editor widget with gutter line numbers, sticky visual column navigation physics, margin-aware 2D scrolling, and atomic grapheme cluster editing.

---

## Visual Showcase & Layout Anatomy

```
┌─────────────────────────────────────────────────────────────┐
│ ╭── editor.cpp ───────────────────────────────────────────╮ │
│ │  1 │ #include <iostream>                                │ │
│ │  2 │                                                    │ │
│ │  3 │ int main() {                                       │ │
│ │  4 │     std::cout << "Status: 🚀\n";█                  │ │ <-- Sticky visual caret
│ │  5 │     return 0;                                      │ │
│ │  6 │ }                                                  │ │
│ ╰─────────────────────────────────────────────────────────╯ │
└─────────────────────────────────────────────────────────────┘
  ▲  ▲ ▲
  │  │ └── Code editing viewport
  │  └──── Vertical divider bar (U+2502)
  └─────── Dynamic line number gutter
```

---

## Header & Class Synopsis

```cpp
#include <koto/text_area.hpp>
```

```cpp
namespace koto
{
    enum class CursorColumnMode
    {
        Clamp,
        Sticky
    };

    enum class ScrollMode
    {
        Clamp,
        Margin
    };

    struct TextAreaState
    {
        std::vector<std::string> text{""};
        size_t cursor_row = 0;
        size_t cursor_col = 0;
        size_t preferred_col = 0; // Visual display column
        size_t scroll_row = 0;
        size_t scroll_col = 0;
        bool show_line_number = true;
        bool focused = true;

        TextAreaState();
        TextAreaState(std::string_view initial_text);

        void insert(char32_t cp);
        void insert(std::string_view str);
        void newline();
        void backspace();
        void delete_forward();

        void move_left();
        void move_right();
        void move_up();
        void move_down();
        void move_home();
        void move_end();

        bool handle_event(const KeyEvent& key);
        bool handle_mouse(const MouseEvent& mouse);
    };

    class TextArea : public Widget
    {
    public:
        explicit TextArea(TextAreaState& state);

        TextArea& line_numbers(bool show);
        TextArea& title(std::string_view title);
        TextArea& border_style(BorderStyle style);
        TextArea& fg(Color fg);
        TextArea& bg(Color bg);
        TextArea& line_number_fg(Color fg);
        TextArea& focused(bool f);
        TextArea& cursor_mode(CursorColumnMode mode);
        TextArea& scroll_mode(ScrollMode mode, size_t margin = 1);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`line_numbers()`** | `bool` | `true` | Toggles dynamic gutter line numbers. |
| **`title()`** | `std::string_view` | `""` | Sets header text on the enclosing border. |
| **`border_style()`** | `BorderStyle` | `BorderStyle::Single` | Configures the border outline. |
| **`line_number_fg()`**| `Color` | `Color::DarkGray` | Color of the gutter line numbers. |
| **`cursor_mode()`** | `CursorColumnMode` | `CursorColumnMode::Sticky` | Toggles sticky visual column memory vs clamping. |
| **`scroll_mode()`** | `ScrollMode, size_t` | `ScrollMode::Clamp, 1` | Configures margin scrolling threshold. |

---

## Usage Examples

### 1. Code Editor with Margin Scrolling

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_editor(Frame& frame, Area area)
{
    static TextAreaState editor(
        "#include <iostream>\n\n"
        "int main() {\n"
        "    std::cout << \"Hello from libkoto 🚀\\n\";\n"
        "    return 0;\n"
        "}\n"
    );

    TextArea(editor)
        .title("main.cpp")
        .border_style(BorderStyle::Rounded)
        .line_numbers(true)
        .line_number_fg(Color::DarkGray)
        .scroll_mode(ScrollMode::Margin, 2)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Sticky Visual Column Preservation**: In `Sticky` mode, moving vertically across a short line does not destroy `preferred_col`. Returning to a subsequent long line restores the original visual column.
2. **Dynamic Gutter Sizing**: Gutter column width scales logarithmically with the total line count ($\lfloor \log_{10}(N) \rfloor + 3$ cols).
3. **Atomic Multi-Line Splits & Merges**: Pressing `Enter` splits the current row into two; pressing `Backspace` at column 0 merges the row into the preceding line without corruption.

---

## C++ Language Features Utilized

1. **`std::string_view` (C++17)**: Eliminates string copies during initialization and title rendering.
2. **`std::vector<std::string>` Memory Model**: Contiguous storage for individual document lines.
3. **Scoped Strongly-Typed Enums (`enum class`) (C++11)**: `CursorColumnMode` and `ScrollMode`.
4. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
