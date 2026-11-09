# `koto::Frame`

Dual-buffered drawing grid managing virtual cell buffers (`m_front_buffer` and `m_back_buffer`), wide continuation cell sentinels, hardware-like 32-bit spatial hit-testing, and automated headless snapshot serialization.

---

## Header & Class Synopsis

```cpp
#include <koto/frame.hpp>
```

```cpp
namespace koto
{
    class Frame
    {
    public:
        Frame(int rows, int cols, Terminal* term = nullptr);

        // Core Cell Operations
        void set_cell(int row, int col, Cell c, Id tag_id = Id::none());
        [[nodiscard]] Cell get_cell(int row, int col) const;
        Cell& operator[](int row, int col);
        const Cell& operator[](int row, int col) const;

        // Area Operations
        void fill(Area area, Cell c, Id tag_id = Id::none());
        void clear(Area area, Color bg = Color::Default);
        void clear_back_buffer();

        // Hit-Testing & Tags
        void tag_area(Area area, Id tag_id);
        [[nodiscard]] Id tag_at(int row, int col) const;
        [[nodiscard]] Area bounds_of(Id tag_id) const;

        // Hardware Cursor Positioning
        void set_cursor(int row, int col);
        void hide_cursor();

        // Snapshotting & Verification
        [[nodiscard]] std::string to_plain_text_row(int row) const;
        [[nodiscard]] std::string to_plain_text() const;

        // Dimensions
        [[nodiscard]] Area area() const noexcept;
        [[nodiscard]] int rows() const noexcept;
        [[nodiscard]] int cols() const noexcept;

        // Flush & Diff Render
        void render(bool flush = true);
    };
}
```

---

## Member Functions Reference

| Method | Signature | Description |
|:---|:---|:---|
| **`set_cell()`** | `void set_cell(int row, int col, Cell c, Id tag_id)` | Writes a cell to `m_back_buffer`, manages continuation cells, and stamps the hit-buffer. |
| **`operator[]`** | `Cell& operator[](int row, int col)` | Modern C++23 multi-dimensional index operator for concise 2D cell mutation. |
| **`fill()`** | `void fill(Area area, Cell c, Id tag_id)` | Fills a rectangular region with repeated cells and optional spatial tag. |
| **`clear()`** | `void clear(Area area, Color bg)` | Clears a rectangular region to whitespace with the specified background color. |
| **`tag_area()`** | `void tag_area(Area area, Id tag_id)` | Marks a region in `m_hit_buffer` without modifying visible text. |
| **`tag_at()`** | `Id tag_at(int row, int col) const` | Queries the spatial ID token residing at screen coordinate `(row, col)`. |
| **`bounds_of()`**| `Area bounds_of(Id tag_id) const` | Returns the minimum bounding rectangle encompassing all cells tagged with `tag_id`. |
| **`set_cursor()`**| `void set_cursor(int row, int col)` | Configures the terminal hardware cursor location for text inputs and carets. |
| **`to_plain_text_row()`** | `std::string to_plain_text_row(int row) const` | Renders a row to a UTF-8 string with continuation cells omitted for headless CI assertions. |
| **`render()`** | `void render(bool flush = true)` | Diffs `m_back_buffer` against `m_front_buffer` and emits ANSI sequences to the terminal. |

---

## Usage Examples

### 1. Drawing Direct Cells with C++23 Multidimensional Subscripting

```cpp
#include <koto/koto.hpp>

using namespace koto;

void draw_crosshair(Frame& frame, int center_r, int center_c)
{
    // C++23 native 2D indexing operator[row, col]
    frame[center_r, center_c] = Cell{U'┼', Color::Red, Color::Default};
    frame[center_r - 1, center_c] = Cell{U'│', Color::Red, Color::Default};
    frame[center_r + 1, center_c] = Cell{U'│', Color::Red, Color::Default};
    frame[center_r, center_c - 1] = Cell{U'─', Color::Red, Color::Default};
    frame[center_r, center_c + 1] = Cell{U'─', Color::Red, Color::Default};
}
```

### 2. Headless Unit Testing Without Live TTY

`Frame` can be constructed directly in memory without an active TTY:

```cpp
#include <koto/koto.hpp>
#include <cassert>

using namespace koto;

void test_box_rendering()
{
    Frame frame(5, 20); // 5 rows, 20 columns virtual grid

    Box().title("Test").border_style(BorderStyle::Rounded).draw(frame, frame.area());

    // Verify rendered snapshot
    std::string top_row = frame.to_plain_text_row(0);
    assert(top_row.find("Test") != std::string::npos);
    assert(top_row.starts_with("╭"));
    assert(top_row.ends_with("╮"));
}
```

---

## Invariants & Safety Guarantees

1. **Automatic Wide Glyph Overwrite Repair**: Overwriting a continuation cell automatically replaces its owner cell on the left with a space `U' '`.
2. **Terminal Boundary Clamping**: Wide characters written to `col == width - 1` are converted to single spaces, preventing unexpected physical cursor wrapping in terminals.
3. **Out-of-Bounds Protection**: Coordinates outside `[0, rows)` or `[0, cols)` are safely ignored by `set_cell()`, `tag_area()`, and `fill()`.

---

## C++ Language Features Utilized

1. **Multi-Dimensional Subscripting (`operator[r, c]`) (C++23)**: Clean syntax for 2D grid manipulations (`frame[r, c]`).
2. **`[[nodiscard]]` Attribute (C++17)**: Prevents silently ignoring snapshot returns and dimension queries.
3. **`noexcept` Member Functions (C++11)**: Guarantees no-throw geometry and dimension accessors (`area()`, `rows()`, `cols()`).
4. **Move Semantics (`std::move`) (C++11)**: Fast buffer pointer swaps between `m_front_buffer` and `m_back_buffer`.
