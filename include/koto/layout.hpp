#pragma once

#include <algorithm>
#include <array>
#include <span>
#include <vector>

namespace koto
{
    struct Area
    {
        int row = 0, col = 0;
        int width = 0, height = 0;

        Area() = default;
        Area(int r, int c, int w, int h)
        : row(r),col(c),width(w),height(h)
        {}

        auto right() const -> int { return col + width ;}
        auto bottom() const -> int { return row + height ;}

        auto inset(int top, int left, int bottom, int right) const -> Area
        {
            int n_row = row + top;
            int n_col = col + left;
            int n_width = width > left + right ? width - left - right : 0;
            int n_height = height > top + bottom ? height - top - bottom : 0;

            n_width = (n_width < 0) ? 0 : n_width;
            n_height = (n_height < 0) ? 0 : n_height;

            return {n_row,n_col,n_width,n_height};
        }

        auto is_valid() const -> bool
        {
            return width > 0 && height > 0;
        }

        auto contains(int r, int c) const -> bool
        {
            return r >= row && r < bottom() && c >= col && c < right();
        }

        auto center_row() const -> int
        {
            return row + height / 2;
        }

        auto center_col() const -> int
        {
            return col + width / 2;
        }

        auto union_with(Area other) const -> Area
        {
            if (!is_valid()) return other;
            if (!other.is_valid()) return *this;
            int min_r = std::min(row, other.row);
            int min_c = std::min(col, other.col);
            int max_r = std::max(bottom(), other.bottom());
            int max_c = std::max(right(), other.right());
            return {min_r, min_c, max_c - min_c, max_r - min_r};
        }

        bool operator==(const Area&) const = default;

        auto set_row(int r) const -> Area
        {
            Area copy = *this;
            copy.row = r;
            return copy;
        }
        auto set_col(int c) const -> Area
        {
            Area copy = *this;
            copy.col = c;
            return copy;
        }
        auto set_height(int h) const -> Area
        {
            Area copy = *this;
            copy.height = h;
            return copy;
        }
        auto set_width(int w) const -> Area
        {
            Area copy = *this;
            copy.width = w;
            return copy;
        }
    };

    struct Constraint
    {
        enum class Type {Fixed, Fill, Percent};
        Type type;
        int value = 0;

        static auto fixed(int n) -> Constraint { return {Type::Fixed, ((n < 0)?0:n)};}
        static auto fill() -> Constraint { return {Type::Fill, 0};}
        static auto percent(int n) -> Constraint { return {Type::Percent,((n < 0)?0:n) };}
    };

    // Small Buffer Optimized container for layout results.
    // Stores up to 16 Area elements inline on the stack with ZERO heap allocation (256 bytes).
    // Automatically spills to heap vector if layout partition exceeds 16 elements.
    class LayoutResult
    {
    public:
        static constexpr size_t InlineCapacity = 16;

        LayoutResult() = default;

        void push_back(Area a)
        {
            if (m_size < InlineCapacity)
            {
                m_stack[m_size++] = a;
            }
            else
            {
                if (m_size == InlineCapacity)
                {
                    m_heap.assign(m_stack.begin(), m_stack.end());
                }
                m_heap.push_back(a);
                m_size++;
            }
        }

        size_t size() const noexcept { return m_size; }
        bool empty() const noexcept { return m_size == 0; }

        const Area& operator[](size_t idx) const noexcept
        {
            return (m_size <= InlineCapacity) ? m_stack[idx] : m_heap[idx];
        }
        Area& operator[](size_t idx) noexcept
        {
            return (m_size <= InlineCapacity) ? m_stack[idx] : m_heap[idx];
        }

        const Area& at(size_t idx) const
        {
            if (idx >= m_size)
            {
                static const Area empty_area(0, 0, 0, 0);
                return empty_area;
            }
            return (*this)[idx];
        }

        const Area* begin() const noexcept
        {
            return (m_size <= InlineCapacity) ? m_stack.data() : m_heap.data();
        }
        const Area* end() const noexcept
        {
            return begin() + m_size;
        }
        const Area* cbegin() const noexcept { return begin(); }
        const Area* cend() const noexcept { return end(); }

        const Area& front() const noexcept { return (*this)[0]; }
        const Area& back() const noexcept { return (*this)[m_size > 0 ? m_size - 1 : 0]; }

        // Explicit conversion to std::vector if the caller specifically wants heap ownership
        std::vector<Area> to_vector() const
        {
            if (m_size <= InlineCapacity)
            {
                return std::vector<Area>(m_stack.begin(), m_stack.begin() + m_size);
            }
            return m_heap;
        }

    private:
        std::array<Area, InlineCapacity> m_stack{};
        std::vector<Area> m_heap{};
        size_t m_size = 0;
    };

    class Grid
    {
    public:
        Grid() = default;
        Grid(size_t r, size_t c, std::vector<Area> cells, Area bounds)
            : m_rows(r), m_cols(c), m_cells(std::move(cells)), m_bounds(bounds)
        {}

        size_t rows() const noexcept { return m_rows; }
        size_t cols() const noexcept { return m_cols; }
        size_t size() const noexcept { return m_cells.size(); }
        Area bounds() const noexcept { return m_bounds; }

        const Area& at(size_t r, size_t c) const
        {
            if (r >= m_rows || c >= m_cols)
            {
                static const Area empty(0, 0, 0, 0);
                return empty;
            }
            return m_cells[r * m_cols + c];
        }

        const Area& operator()(size_t r, size_t c) const { return at(r, c); }
        const Area& operator[](size_t r, size_t c) const { return at(r, c); }

        struct RowProxy
        {
            const Grid* grid;
            size_t row;
            const Area& operator[](size_t col) const { return grid->at(row, col); }
        };

        RowProxy operator[](size_t r) const { return RowProxy{this, r}; }

        template <typename F>
        void for_each(F&& func) const
        {
            for (size_t r = 0; r < m_rows; ++r)
            {
                for (size_t c = 0; c < m_cols; ++c)
                {
                    func(r, c, at(r, c));
                }
            }
        }

        const std::vector<Area>& cells() const noexcept { return m_cells; }

    private:
        size_t m_rows = 0;
        size_t m_cols = 0;
        std::vector<Area> m_cells;
        Area m_bounds{0, 0, 0, 0};
    };

    namespace Layout
    {
        // Dynamic layout split (std::span / std::vector / dynamic list) -> zero-heap LayoutResult (16-capacity inline)
        auto vertical(Area area, std::span<const Constraint> constraints) -> LayoutResult;
        auto horizontal(Area area, std::span<const Constraint> constraints) -> LayoutResult;

        // Compile-time array deduction overload returning std::array<Area, N> with structured bindings
        template <size_t N>
        inline std::array<Area, N> vertical(Area area, const Constraint (&constraints)[N])
        {
            LayoutResult lr = vertical(area, std::span<const Constraint>(constraints, N));
            std::array<Area, N> arr{};
            for (size_t i = 0; i < N; ++i)
            {
                arr[i] = lr[i];
            }
            return arr;
        }

        template <size_t N>
        inline std::array<Area, N> vertical(Area area, const std::array<Constraint, N>& constraints)
        {
            return vertical(area, constraints.data());
        }

        template <size_t N>
        inline std::array<Area, N> horizontal(Area area, const Constraint (&constraints)[N])
        {
            LayoutResult lr = horizontal(area, std::span<const Constraint>(constraints, N));
            std::array<Area, N> arr{};
            for (size_t i = 0; i < N; ++i)
            {
                arr[i] = lr[i];
            }
            return arr;
        }

        template <size_t N>
        inline std::array<Area, N> horizontal(Area area, const std::array<Constraint, N>& constraints)
        {
            return horizontal(area, constraints.data());
        }

        auto center(Area area, int width, int height) -> Area;

        auto grid(Area area, 
                  std::span<const Constraint> row_constraints, 
                  std::span<const Constraint> col_constraints) -> Grid;

        auto grid_uniform(Area area, size_t rows, size_t cols) -> Grid;

        auto grid_fixed(Area area, size_t rows, size_t cols, int cell_width, int cell_height) -> Grid;
    }
}
