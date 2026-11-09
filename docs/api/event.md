# `koto::Event`

Strongly-typed, zero-overhead polymorphic event model encapsulating keyboard input (`KeyEvent`), SGR mouse tracking (`MouseEvent`), window resizing (`ResizeEvent`), and bracketed paste clipboard streams (`PasteEvent`).

---

## Header & Class Synopsis

```cpp
#include <koto/event.hpp>
```

```cpp
namespace koto
{
    enum class Key : uint32_t
    {
        None = 0,
        Up = 0x80000000, Down, Left, Right,
        Home, End, PageUp, PageDown,
        Enter, Backspace, Delete, Insert, Tab, BackTab,
        Escape,
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        ShiftUp, ShiftDown, ShiftLeft, ShiftRight,
        CtrlUp, CtrlDown, CtrlLeft, CtrlRight,
        AltUp, AltDown, AltLeft, AltRight
    };

    struct KeyEvent
    {
        uint32_t ch = 0;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool meta = false;

        [[nodiscard]] bool is(Key k) const;
        [[nodiscard]] bool is_char() const;
    };

    enum class MouseButton : uint8_t
    {
        Left = 0,
        Middle = 1,
        Right = 2,
        WheelUp = 64,
        WheelDown = 65,
        None = 255
    };

    enum class MouseAction : uint8_t
    {
        Press = 0,
        Release = 1,
        Move = 2
    };

    struct MouseEvent
    {
        MouseButton button = MouseButton::None;
        MouseAction action = MouseAction::Press;
        int row = 0;
        int col = 0;
        bool shift = false;
        bool ctrl = false;
        bool alt = false;
        Id target = Id::none();
    };

    struct ResizeEvent
    {
        uint32_t rows = 0;
        uint32_t cols = 0;
    };

    struct PasteEvent
    {
        std::string text;
    };

    class Event
    {
    public:
        [[nodiscard]] bool is_key() const noexcept;
        [[nodiscard]] bool is_mouse() const noexcept;
        [[nodiscard]] bool is_resize() const noexcept;
        [[nodiscard]] bool is_paste() const noexcept;

        [[nodiscard]] const KeyEvent* as_key() const noexcept;
        [[nodiscard]] const MouseEvent* as_mouse() const noexcept;
        [[nodiscard]] const ResizeEvent* as_resize() const noexcept;
        [[nodiscard]] const PasteEvent* as_paste() const noexcept;
    };
}
```

---

## Event Types Reference

| Event Struct | Primary Fields | Typical Use Case |
|:---|:---|:---|
| **`KeyEvent`** | `ch`, `ctrl`, `shift`, `alt`, `meta` | Text input, hotkeys, focus navigation (`Tab`, `Arrows`). |
| **`MouseEvent`**| `button`, `action`, `row`, `col`, `target` | Clicking buttons, dragging scrollbars, mouse wheel scrolling. |
| **`ResizeEvent`**| `rows`, `cols` | Recomputing responsive layout grids when terminal window resizes. |
| **`PasteEvent`** | `text` | Multi-line bracketed paste without triggering repetitive key events. |

---

## Usage Examples

### 1. Robust Keyboard Matching

```cpp
#include <koto/koto.hpp>

using namespace koto;

void handle_navigation(const KeyEvent& key)
{
    // Match functional keys directly
    if (key.is(Key::Escape)) {
        // Exit modal or cancel
    } else if (key.is(Key::Tab)) {
        // Advance focus forward
    } else if (key.is(Key::BackTab)) {
        // Advance focus backward (Shift+Tab)
    } else if (key.ctrl && (key.ch == 'c' || key.ch == 'C')) {
        // Interrupt / SIGINT shortcut
    }
}
```

### 2. Spatial Mouse Handling with Target Hit Tags

```cpp
#include <koto/koto.hpp>

using namespace koto;

void setup_mouse_handlers(App& app)
{
    app.on_mouse([&](const MouseEvent& mouse) {
        if (mouse.button == MouseButton::Left && mouse.action == MouseAction::Press) {
            // Check if user clicked a tagged widget directly
            if (mouse.target == "btn_submit") {
                // Submit form
            }
        } else if (mouse.button == MouseButton::WheelDown) {
            // Scroll down
        } else if (mouse.button == MouseButton::WheelUp) {
            // Scroll up
        }
    });
}
```

---

## Invariants & Safety Guarantees

1. **Non-Throwing Inspection**: All query functions (`is_key()`, `as_key()`, etc.) are marked `noexcept` and guarantee zero memory allocation.
2. **Terminal Mouse Protocol Neutrality**: Mouse events automatically convert both legacy X10 and modern SGR 1006 protocols into identical coordinate models.
3. **Bracketed Paste Sanitization**: Control characters within pasted text streams are preserved safely without executing arbitrary terminal commands.

---

## C++ Language Features Utilized

1. **`std::variant` Polymorphic Storage (C++17)**: High-speed, type-safe union eliminating virtual method table overhead and heap allocation.
2. **`[[nodiscard]]` (C++17)**: Enforces checking type query results.
3. **Scoped Strongly-Typed Enums (`enum class`) (C++11)**: `Key`, `MouseButton`, and `MouseAction`.
4. **`noexcept` Specifications (C++11)**: Strict compile-time performance guarantees on accessor paths.
