# `koto::Tabs` & `koto::TabsState`

Horizontal tab navigation bar supporting multiple visual styles (Bar, Pill, Underline), numeric hotkeys (`1`–`9`), Vim keys (`h`/`l`), and spatial mouse tab selection.

---

## Visual Showcase & Layout Anatomy

```
Style: Bar
 Overview │ Logs │ Settings 

Style: Pill
 [ Overview ]  [ Logs ]  [ Settings ] 

Style: Underline
 Overview    Logs    Settings 
 ════════
```

---

## Header & Class Synopsis

```cpp
#include <koto/tabs.hpp>
```

```cpp
namespace koto
{
    enum class TabsStyle
    {
        Bar,        // " Home │ Output │ Settings "
        Pill,       // " [ Home ]  [ Output ]  [ Settings ] "
        Underline   // Top line tabs, bottom line underline bar
    };

    struct TabsState
    {
        std::vector<std::string> titles;
        size_t selected = 0;
        bool focused = true;
        std::function<void(size_t idx, const std::string& title)> on_change;

        TabsState() = default;
        TabsState(std::vector<std::string> initial_titles, size_t initial_selected = 0);

        void select_next();
        void select_prev();
        void set_selected(size_t index);
        [[nodiscard]] std::optional<std::string_view> selected_title() const;

        bool handle_event(const KeyEvent& key);
        bool handle_mouse(const MouseEvent& mouse);
    };

    class Tabs : public Widget
    {
    public:
        explicit Tabs(TabsState& state);
        explicit Tabs(std::vector<std::string> titles);

        Tabs& style(TabsStyle s);
        Tabs& separator(std::string_view sep);
        Tabs& active_fg(Color color);
        Tabs& active_bg(Color color);
        Tabs& inactive_fg(Color color);
        Tabs& inactive_bg(Color color);
        Tabs& border_style(BorderStyle border);

        void draw(Frame& frame, Area area) override;
    };
}
```

---

## Navigation & Controls Reference

| Input Action | Result | Description |
|:---|:---|:---|
| **`Right Arrow` / `l`** | `select_next()` | Advances to next tab (wraps to first). |
| **`Left Arrow` / `h`** | `select_prev()` | Moves to previous tab (wraps to last). |
| **`1` .. `9`** | `set_selected(idx)` | Direct numerical jump to 1-indexed tab. |
| **Left Click on Tab** | `set_selected(idx)` | Direct mouse selection of clicked tab. |

---

## Configuration Methods Reference

| Method | Argument | Default | Description |
|:---|:---|:---|:---|
| **`style()`** | `TabsStyle` | `TabsStyle::Bar` | Configures visual header format (`Bar`, `Pill`, `Underline`). |
| **`separator()`** | `std::string_view` | `"│"` | Custom separator string between tabs in `Bar` style. |
| **`active_fg()`** | `Color` | `Color::Cyan` | Foreground color of the active tab. |
| **`active_bg()`** | `Color` | `COLOR_DEFAULT` | Background color behind the active tab. |
| **`inactive_fg()`**| `Color` | `Color::DarkGray` | Foreground color of unselected tabs. |
| **`inactive_bg()`**| `Color` | `COLOR_DEFAULT` | Background color behind unselected tabs. |

---

## Usage Examples

### 1. Underline Styled Tab Navigation

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_navigation_bar(Frame& frame, Area area)
{
    static TabsState tabs({"Overview 📊", "Logs 📜", "Metrics 📈", "Terminal 💻"});

    Tabs(tabs)
        .style(TabsStyle::Underline)
        .active_fg(Color::Cyan)
        .inactive_fg(Color::DarkGray)
        .draw(frame, area);
}
```

---

## Invariants & Safety Guarantees

1. **Sub-Item Hit-Testing**: Each tab title stamps an indexed `Id` into `Frame::m_hit_buffer`, allowing precise single-click tab selection.
2. **Circular Wrapping**: Calling `select_next()` on the final tab wraps cleanly to index 0.
3. **Empty Titles Safety**: If `titles` is empty, `draw()` renders cleanly without throwing or crashing.

---

## C++ Language Features Utilized

1. **`std::optional<std::string_view>` (C++17)**: Safe accessor for active tab title.
2. **`std::string_view` (C++17)**: Zero-copy parameter passing for separators and titles.
3. **Scoped Strongly-Typed Enums (`enum class`) (C++11)**: `TabsStyle`.
4. **Virtual Function Override (`override`) (C++11)**: Strict compile-time interface verification.
