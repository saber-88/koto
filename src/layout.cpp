#include "koto/layout.hpp"
#include <algorithm>
#include <span>
#include <vector>

namespace
{
    auto split_1d(int origin, int total_size, int cross_origin, int cross_size, bool is_vertical,
                  std::span<const koto::Constraint> constraints) -> koto::LayoutResult
    {
        koto::LayoutResult layout;
        int fill_count = 0;
        int used = 0;

        for (const auto& c : constraints)
        {
            switch (c.type)
            {
                case koto::Constraint::Type::Fixed:
                    used += c.value;
                    break;
                case koto::Constraint::Type::Fill:
                    fill_count++;
                    break;
                case koto::Constraint::Type::Percent:
                    used += (total_size * c.value) / 100;
                    break;
            }
        }

        int remaining = std::max(0, total_size - used);
        int fill_size = (fill_count > 0) ? remaining / fill_count : 0;
        int fill_remainder = (fill_count > 0) ? remaining % fill_count : 0;

        int current_pos = origin;
        int fill_idx = 0;

        for (const auto& c : constraints)
        {
            int size = 0;
            switch (c.type)
            {
                case koto::Constraint::Type::Fixed:
                    size = c.value;
                    break;
                case koto::Constraint::Type::Fill:
                    size = fill_size + (fill_idx < fill_remainder ? 1 : 0);
                    fill_idx++;
                    break;
                case koto::Constraint::Type::Percent:
                    size = (total_size * c.value) / 100;
                    break;
            }

            int max_possible = std::max(0, (origin + total_size) - current_pos);
            size = std::min(size, max_possible);

            if (is_vertical)
            {
                layout.push_back(koto::Area{current_pos, cross_origin, cross_size, size});
            }
            else
            {
                layout.push_back(koto::Area{cross_origin, current_pos, size, cross_size});
            }
            current_pos += size;
        }

        return layout;
    }
}

namespace koto
{
    auto Layout::vertical(Area area, std::span<const Constraint> constraints)
        -> LayoutResult
    {
        if(!area.is_valid())
        {
            LayoutResult empty;
            for (size_t i = 0; i < constraints.size(); ++i)
            {
                empty.push_back(Area{area.row, area.col, 0, 0});
            }
            return empty;
        }
        return split_1d(area.row, area.height, area.col, area.width, true, constraints);
    }

    auto Layout::horizontal(Area area, std::span<const Constraint> constraints)
        -> LayoutResult
    {
        if(!area.is_valid())
        {
            LayoutResult empty;
            for (size_t i = 0; i < constraints.size(); ++i)
            {
                empty.push_back(Area{area.row, area.col, 0, 0});
            }
            return empty;
        }
        return split_1d(area.col, area.width, area.row, area.height, false, constraints);
    }

    auto Layout::center(Area area, int width, int height) -> Area
    {
        if (!area.is_valid() || width <= 0 || height <= 0)
        {
            return {area.row, area.col, 0, 0};
        }
        int w = std::min(width, area.width);
        int h = std::min(height, area.height);
        int r = area.row + (area.height - h) / 2;
        int c = area.col + (area.width - w) / 2;

        return {r, c, w, h};
    }

    auto Layout::grid(Area area, 
                      std::span<const Constraint> row_constraints, 
                      std::span<const Constraint> col_constraints) -> Grid
    {
        if (row_constraints.empty() || col_constraints.empty() || !area.is_valid())
        {
            return Grid(0, 0, {}, area);
        }

        size_t num_rows = row_constraints.size();
        size_t num_cols = col_constraints.size();

        auto row_areas = Layout::vertical(area, row_constraints);
        std::vector<Area> cells;
        cells.reserve(num_rows * num_cols);

        for (size_t r = 0; r < num_rows; ++r)
        {
            auto col_areas = Layout::horizontal(row_areas[r], col_constraints);
            for (size_t c = 0; c < num_cols; ++c)
            {
                cells.push_back(col_areas[c]);
            }
        }

        return Grid(num_rows, num_cols, std::move(cells), area);
    }

    auto Layout::grid_uniform(Area area, size_t rows, size_t cols) -> Grid
    {
        if (rows == 0 || cols == 0)
        {
            return Grid(0, 0, {}, area);
        }
        std::vector<Constraint> row_c(rows, Constraint::fill());
        std::vector<Constraint> col_c(cols, Constraint::fill());
        return grid(area, row_c, col_c);
    }

    auto Layout::grid_fixed(Area area, size_t rows, size_t cols, int cell_width, int cell_height) -> Grid
    {
        if (rows == 0 || cols == 0)
        {
            return Grid(0, 0, {}, area);
        }
        std::vector<Constraint> row_c(rows, Constraint::fixed(cell_height));
        std::vector<Constraint> col_c(cols, Constraint::fixed(cell_width));
        return grid(area, row_c, col_c);
    }
}