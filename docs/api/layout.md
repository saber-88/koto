# `koto::Layout`, `Area`, `Constraint` & `Grid`

Declarative geometry engine supporting fixed, percentage, and dynamic fill constraints with 16-capacity stack-allocated Small Size Optimization (SSO), structured bindings, and multi-dimensional 2D grid matrix indexing.

---

## Header & Class Synopsis

```cpp
#include <koto/layout.hpp>
```

```cpp
namespace koto
{
    struct Area
    {
        int row = 0, col = 0;
        int width = 0, height = 0;

        Area() = default;
        Area(int r, int c, int w, int h);

        [[nodiscard]] int right() const noexcept;
        [[nodiscard]] int bottom() const noexcept;
        [[nodiscard]] bool is_valid() const noexcept;
        [[nodiscard]] bool contains(int r, int c) const noexcept;

        [[nodiscard]] Area inset(int top, int left, int bottom, int right) const;
        [[nodiscard]] Area union_with(Area other) const;

        bool operator==(const Area&) const = default;
    };

    struct Constraint
    {
        enum class Type { Fixed, Fill, Percent };
        Type type;
        int value = 0;

        static Constraint fixed(int n);
        static Constraint fill();
        static Constraint percent(int n);
    };

    class LayoutResult
    {
    public:
        static constexpr size_t InlineCapacity = 16;
        size_t size() const noexcept;
        const Area& operator[](size_t idx) const noexcept;
    };

    namespace Layout
    {
        // Dynamic partition (using std::span)
        LayoutResult vertical(Area area, std::span<const Constraint> constraints);
        LayoutResult horizontal(Area area, std::span<const Constraint> constraints);

        // Compile-time array deduction overload returning std::array<Area, N>
        template <size_t N>
        std::array<Area, N> vertical(Area area, const Constraint (&constraints)[N]);

        template <size_t N>
        std::array<Area, N> horizontal(Area area, const Constraint (&constraints)[N]);

        // 2D Grid Helpers
        Grid grid(Area area, std::span<const Constraint> rows, std::span<const Constraint> cols);
        Grid grid_uniform(Area area, size_t num_rows, size_t num_cols);
        Area center(Area parent, int width, int height);
    }
}
```

---

## Constraint Types Reference

| Type | Factory Method | Behavior | Description |
|:---|:---|:---|:---|
| **Fixed** | `Constraint::fixed(n)` | Reserves exactly $n$ rows or columns | Used for borders, titles, headers, status lines. |
| **Percent** | `Constraint::percent(p)`| Reserves $\lfloor \frac{p \times \text{total}}{100} \rfloor$ space | Proportionate division (e.g. 30% sidebar, 70% content). |
| **Fill** | `Constraint::fill()` | Consumes remaining space equally | Dynamic expansion filling available viewport. |

---

## Usage Examples

### 1. Partitioning with Structured Bindings

```cpp
#include <koto/koto.hpp>

using namespace koto;

void layout_example(Area screen)
{
    // Decomposes into 3 distinct Area values on the stack
    auto [header, body, footer] = Layout::vertical(screen, {
        Constraint::fixed(3),
        Constraint::fill(),
        Constraint::fixed(1)
    });

    // Subdivide body horizontally into 20% sidebar and 80% main content
    auto [sidebar, content] = Layout::horizontal(body, {
        Constraint::percent(20),
        Constraint::percent(80)
    });
}
```

### 2. Multi-Dimensional Grid Layout (`koto::Grid`)

```cpp
#include <koto/koto.hpp>

using namespace koto;

void render_dashboard_grid(Frame& frame, Area area)
{
    // Uniform 3x3 layout
    Grid matrix = Layout::grid_uniform(area, 3, 3);

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            // C++23 2D multidimensional index matrix[r, c]
            Area cell_area = matrix[r, c];
            Box().title("Card").draw(frame, cell_area);
        }
    }
}
```

---

## Invariants & Safety Guarantees

1. **Zero-Allocation Stack SSO Guarantee**: Any layout query specifying up to 16 constraints executes completely on the stack with zero dynamic memory allocation.
2. **Under-Size Protection**: If an allocated area has insufficient width or height to fulfill constraints, subsequent areas clamp gracefully to `width = 0` or `height = 0` rather than wrapping or overflowing coordinates.
3. **Structured Binding Tuple Protocol**: `LayoutResult` implements `std::tuple_size` and `std::tuple_element` hooks to seamlessly unpack up to 16 variables in C++17 structured bindings.

---

## C++ Language Features Utilized

1. **Structured Bindings (C++17)**: `auto [a, b, c] = Layout::vertical(...)` eliminates index-based extraction boilerplate.
2. **C++23 Multi-Dimensional Indexing (`Grid::operator[r, c]`)**: Intuitive coordinate access on 2D layout matrices.
3. **Small Size Optimization (SSO)**: `LayoutResult` uses an inline `std::array<Area, 16>` buffer to prevent heap allocations.
4. **Compile-Time Array Template Overloads (C++11/C++14)**: `template <size_t N> auto vertical(Area, const Constraint (&)[N])` returns `std::array<Area, N>` for compile-time size deduction.
5. **Defaulted Equality (`operator== = default`) (C++20)**: Synthesizes member-wise equality for `Area`.
