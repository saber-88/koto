# `koto::List` & `koto::ListState`

Scrollable item list widget featuring customizable highlight symbols, full-width row backgrounds, keyboard navigation (Vim `j`/`k` and arrows), and direct mouse row selection and wheel scrolling.

---

## Visual Showcase & Layout Anatomy

```
┌───────────────────────────────────────────┐
│ ╭── Select Host ────────────────────────╮ │
│ │   alpha.internal.net                  │ │
│ │ ▶ beta.internal.net                   │ │ <-- Highlight symbol + selected row bg
│ │   gamma.internal.net                  │ │
│ │   delta.internal.net                  │ │
│ ╰───────────────────────────────────────╯ │
└───────────────────────────────────────────┘
```

---

## Header & Class Synopsis

```cpp
#include <koto/list.hpp>
```

```cpp
namespace koto
{
    enum class ListNavigation
    {
        Clamp,
        Wrap
    };

    struct ListState
    {
        std::vector<std::string> items;
        int selected = 0;
        int scroll = 0;
        bool focused = true;
        ListNavigation navigation = ListNavigation::Clamp;
        std::function<void(int idx, const std::string& val)> on_change;

        ListState() = default;
        ListState(std::vector<std::string> initial_items, ListNavigation nav = ListNavigation::Clamp);

        void select_next();
        void select_prev();
        void select_first();
        void select_last();
        void set_selected(int index);

        [[nodiscard]] std::optional<std::string_view> selected_item() const;

        bool handle_event(const KeyEvent& key);
        bool handle_mouse(const MouseEvent& mouse);
    };

    class List : public Widget
    {
    public:
        explicit List(ListState& state);

        List& title(std::string title);
        List& border_style(BorderStyle border);
        List& highlight_symbol(std::string symbol);
        List& highlight_fg(Color color);
        List& highlight_bg(Color color);
        List& fg(Color color);
        List& bg(Color color);
        List& overflow(TextOverflow o);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Keyboard & Mouse Controls Reference

| Input Action | Result | Description |
|:---|:---|:---|
| **`j` / `Down Arrow`** | `select_next()` | Advances selection downward. Clamps or wraps based on `navigation`. |
| **`k` / `Up Arrow`** | `select_prev()` | Moves selection upward. Clamps or wraps based on `navigation`. |
| **`g` / `Home`** | `select_first()` | Jumps immediately to the top of the list (`index: 0`). |
| **`G` / `End`** | `select_last()` | Jumps immediately to the bottom of the list. |
| **Left Click on Row** | `set_selected(row)` | Direct spatial selection of the clicked list element. |
| **Mouse Wheel Up/Down**| Viewport Scroll | Scrolls the visible window by one line. |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`title()`** | `std::string` | `""` | Sets the header text on the enclosing border. |
| **`border_style()`** | `BorderStyle` | `BorderStyle::Single` | Configures the surrounding border. |
| **`highlight_symbol()`**| `std::string` | `"> "` | Prefix marker prepended to the active row. |
| **`highlight_fg()`** | `Color` | `Color::Yellow` | Foreground color of the selected row. |
| **`highlight_bg()`** | `Color` | `COLOR_DEFAULT` | Full-width background color behind the selected row. |
| **`overflow()`** | `TextOverflow` | `TextOverflow::Clip` | Configures clipping or ellipsis truncation for long lines. |

---

## Usage Examples

### 1. Interactive Menu with Change Callback

```cpp
#include <koto/koto.hpp>

using namespace koto;

void setup_menu(Frame& frame, Area area)
{
    static ListState menu({"Dashboard", "Deployments", "Analytics", "Settings"});

    menu.on_change = [](int idx, const std::string& item) {
        // State update when selection changes
    };

    List(menu)
        .title("Main Menu")
        .border_style(BorderStyle::Rounded)
        .highlight_symbol("▶ ")
        .highlight_fg(Color::Yellow)
        .highlight_bg(Color::Hex(0x313244))
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Auto-Adjusting Viewport Scroll**: Whenever `selected` advances outside the visible bounds, `scroll` adjusts automatically so the selected item is always displayed.
2. **Bounds Protection**: `selected` is strictly clamped within $[0, \text{items.size()} - 1]$. If the list is empty, `selected_item()` returns `std::nullopt`.
3. **Deterministic Separation of Concerns (`UI = f(state)`)**: All navigation logic resides in mutable `ListState`, while `List` is a pure, immutable draw pass.

---

## C++ Language Features Utilized

1. **`std::optional<std::string_view>` (C++17)**: Null-safe accessor for the active selection.
2. **`std::function` State Callbacks (C++11)**: Event notification on selection transition (`on_change`).
3. **Scoped Strongly-Typed Enums (`enum class`) (C++11)**: `ListNavigation` (`Clamp`, `Wrap`).
4. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
