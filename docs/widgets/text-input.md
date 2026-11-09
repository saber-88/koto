# `koto::TextInput` & `koto::TextInputState`

Single-line editable text input widget featuring atomic Unicode grapheme deletion, input filtering, automatic horizontal scrolling, placeholder styling, and hardware cursor synchronization.

---

## Visual Showcase & Layout Anatomy

```
┌───────────────────────────────────────────┐
│ ╭── Username ───────────────────────────╮ │
│ │ admin_user_88█                        │ │ <-- Hardware cursor synced over caret
│ ╰───────────────────────────────────────╯ │
│                                           │
│ ╭── Port ───────────────────────────────╮ │
│ │ 8080                                  │ │ <-- Input filter rejects non-digits
│ ╰───────────────────────────────────────╯ │
└───────────────────────────────────────────┘
```

---

## Header & Class Synopsis

```cpp
#include <koto/text_input.hpp>
```

```cpp
namespace koto
{
    struct TextInputState
    {
        using KeyInterceptor = std::function<bool(TextInputState&, const KeyEvent&)>;
        using InputFilter = std::function<bool(std::string_view current_text, char32_t new_char)>;

        std::string value;
        size_t cursor = 0;
        int scroll_offset = 0;
        bool focused = true;

        TextInputState() = default;
        TextInputState(std::string_view initial_text);

        bool handle_event(const KeyEvent& key);
        TextInputState& on_key(KeyInterceptor handler);
        TextInputState& set_filter(InputFilter filter);

        bool insert(char32_t cp);
        bool insert(std::string_view text);
        bool backspace();
        bool delete_forward();
        bool delete_word();
        bool move_left();
        bool move_right();
        bool move_home();
        bool move_end();
        void clear();
    };

    class TextInput : public Widget
    {
    public:
        explicit TextInput(TextInputState& state);

        TextInput& placeholder(std::string_view placeholder);
        TextInput& title(std::string_view title);
        TextInput& border_style(BorderStyle style);
        TextInput& fg(Color color);
        TextInput& bg(Color color);
        TextInput& placeholder_fg(Color color);
        TextInput& focused_border_fg(Color color);
        TextInput& focused(bool is_focused);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Editing Actions Reference

| Action | Function | Keybinding | Description |
|:---|:---|:---|:---|
| **Character Insertion** | `insert(cp)` | Direct typing | Appends UTF-8 bytes at `cursor`. |
| **Atomic Backspace** | `backspace()` | `Backspace` | Deletes preceding grapheme cluster as an atomic unit. |
| **Forward Delete** | `delete_forward()` | `Delete` | Deletes succeeding grapheme cluster. |
| **Word Deletion** | `delete_word()` | `Ctrl+W` / `Alt+Backspace` | Deletes back to preceding whitespace. |
| **Move Caret Left** | `move_left()` | `Left Arrow` | Moves backward by one grapheme cluster. |
| **Move Caret Right**| `move_right()` | `Right Arrow` | Moves forward by one grapheme cluster. |
| **Jump to Start** | `move_home()` | `Home` / `Ctrl+A` | Jumps to beginning of string (`cursor: 0`). |
| **Jump to End** | `move_end()` | `End` / `Ctrl+E` | Jumps to end of string (`cursor: value.size()`). |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`placeholder()`** | `std::string_view` | `""` | Dim hint text displayed when `value` is empty. |
| **`title()`** | `std::string_view` | `""` | Header label on the surrounding border. |
| **`border_style()`** | `BorderStyle` | `BorderStyle::Single` | Configures the border outline. |
| **`placeholder_fg()`**| `Color` | `Color::DarkGray` | Text color of the placeholder string. |
| **`focused()`** | `bool` | `state.focused` | Toggles border focus highlighting and hardware cursor visibility. |

---

## Usage Examples

### 1. Numeric Port Input with Validation Filter

```cpp
#include <koto/koto.hpp>

using namespace koto;

void setup_port_field(Frame& frame, Area area)
{
    static TextInputState port_state("8080");

    // Strictly restrict input to ASCII digits
    port_state.set_filter([](std::string_view current, char32_t cp) {
        return cp >= '0' && cp <= '9';
    });

    TextInput(port_state)
        .title("Port")
        .placeholder("e.g. 3000")
        .border_style(BorderStyle::Rounded)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Atomic Grapheme Safety**: Pressing backspace or delete never splits multi-byte UTF-8 sequences, zero-width joiners, skin-tone modifiers, or emoji variation selectors.
2. **Hardware Cursor Alignment**: When `focused == true`, `TextInput` positions the terminal hardware cursor precisely over the active column via `frame.set_cursor()`.
3. **Viewport Scroll Clamping**: If typing exceeds the width of the input box, `scroll_offset` shifts automatically so the typing caret remains visible.

---

## C++ Language Features Utilized

1. **Type Aliases (`using KeyInterceptor = ...`) (C++11)**: Clear, expressive closure signatures.
2. **`std::string_view` (C++17)**: High-performance string passing across filters and builders.
3. **Lambda Filter Callbacks (C++11)**: Pluggable input validation algorithms.
4. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
