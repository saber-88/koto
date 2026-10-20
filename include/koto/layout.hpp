#pragma once

#include <vector>

namespace koto
{
    struct Area
    {
        int row, col;
        int width ,height;

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
        auto vertical(Area area, const std::vector<Constraint>& constraints) -> std::vector<Area>;
        auto horizontal(Area area, const std::vector<Constraint>& constraints) -> std::vector<Area>;
        auto center(Area area, int width, int height) -> Area;

        auto grid(Area area, 
                  const std::vector<Constraint>& row_constraints, 
                  const std::vector<Constraint>& col_constraints) -> Grid;

        auto grid_uniform(Area area, size_t rows, size_t cols) -> Grid;

        auto grid_fixed(Area area, size_t rows, size_t cols, int cell_width, int cell_height) -> Grid;
    }
}