# `koto::Button`

Interactive trigger widget supporting mouse click dispatch, keyboard activation (`Enter` / `Space`), decorative borders, active focus styling, and automatic `FocusManager` integration.

---

## Visual Showcase & Layout Anatomy

```
Normal Unfocused Button:
┌───────────────────────┐
│     [ Submit ]        │
└───────────────────────┘

Active Focused Button:
╭───────────────────────╮  <-- Focused border color (e.g. Cyan / Yellow)
│    ▶  Submit  ◀       │  <-- Focused text color
╰───────────────────────╯
```

---

## Header & Class Synopsis

```cpp
#include <koto/button.hpp>
```

```cpp
namespace koto
{
    class Button : public Widget
    {
    public:
        Button() = default;
        explicit Button(std::string_view label);
        Button(std::string_view label, std::function<void()> on_click);

        Button& label(std::string_view label);
        Button& border_style(BorderStyle style);

        // Styling
        Button& fg(Color color);
        Button& bg(Color color);
        Button& label_fg(Color color);
        Button& focused(bool is_foc);
        Button& focused_border_fg(Color color);
        Button& focused_label_fg(Color color);

        // Integration
        Button& id(Id id);
        Button& focus(FocusManager& fm);
        Button& on_click(std::function<void()> callback);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`label()`** | `std::string_view` | `""` | Sets the button text label. |
| **`border_style()`** | `BorderStyle` | `BorderStyle::Single` | Configures the button border outline. |
| **`fg()`** | `Color` | `COLOR_DEFAULT` | Sets the unfocused border color. |
| **`bg()`** | `Color` | `COLOR_DEFAULT` | Sets the interior background fill color. |
| **`label_fg()`** | `Color` | `Color::White` | Sets the text color when unfocused. |
| **`focused()`** | `bool` | `false` | Explicitly overrides focus state. |
| **`focused_border_fg()`** | `Color` | `Color::Cyan` | Border color applied when the button is focused. |
| **`focused_label_fg()`** | `Color` | `Color::Yellow`| Text color applied when the button is focused. |
| **`id()`** | `Id` | `Id::none()` | Registers the button in `m_hit_buffer` for spatial mouse tracking. |
| **`focus()`** | `FocusManager&` | None | Automatically queries whether `this->id()` holds active focus in `fm`. |
| **`on_click()`** | `std::function<void()>` | None | Action callback executed when clicked or triggered by `Enter`/`Space`. |

---

## Usage Examples

### 1. Interactive Action Button with Focus Integration

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_action_button(Frame& frame, Area area, FocusManager& fm)
{
    Button("Deploy Cluster 🚀")
        .id("btn_deploy")
        .focus(fm)
        .border_style(BorderStyle::Rounded)
        .focused_border_fg(Color::Green)
        .on_click([&]() {
            // Trigger deployment routine
        })
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Automatic Hit-Buffer Registration**: Calling `.id("my_btn")` stamps the entire button area into `Frame::m_hit_buffer`, routing mouse clicks automatically.
2. **Dual-Trigger Equivalence**: Triggering via mouse click and triggering via `Enter` or `Space` both invoke the exact same `on_click` callback.
3. **Centered Label Alignment**: The label string is automatically centered vertically and horizontally within the button area.

---

## C++ Language Features Utilized

1. **`std::string_view` (C++17)**: High-performance, zero-allocation label passing.
2. **`std::function<void()>` Callbacks (C++11)**: Flexible lambda closure storage for click actions.
3. **Fluent Builder Pattern (`Button&`)**: Method chaining returning `*this` by reference.
4. **Explicit Constructors (C++11)**: Prevents accidental implicit conversions from string literals.
