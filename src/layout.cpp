#include "koto/layout.hpp"
#include <algorithm>
#include <vector>

namespace koto
{
    auto Layout::vertical(Area area, const std::vector<Constraint>& constraints)
        -> std::vector<Area>
    {
        if(!area.is_valid())
        {
            return std::vector<Area>(constraints.size(),Area{area.row,area.col,0,0});
        }
        std::vector<Area> layout;
        int fill_count = 0;
        int used = 0;
        
        for (auto c: constraints)
        {
            switch (c.type) {
                case Constraint::Type::Fixed :
                {
                    used += c.value;
                    break;
                    
                }
                case Constraint::Type::Fill :
                {
                    fill_count++;
                    break;
                }
                case Constraint::Type::Percent :
                {
                    used += (area.height * c.value) / 100;
                    break;
                }
            }

        }
        int remaining = std::max(0,area.height - used);
        int fill_size = (fill_count > 0) ? remaining / fill_count : 0;
        int fill_remainder = (fill_count > 0) ? remaining % fill_count : 0;

        int current_row = area.row;
        int fill_idx = 0;

        for (auto c : constraints)
        {
            Area rect(current_row,area.col,area.width,area.height);
            
            switch (c.type) {
                case Constraint::Type::Fixed :
                {
                    rect.height = c.value;
                    break;
                    
                }
                case Constraint::Type::Fill :
                {
                    rect.height = fill_size + (fill_idx < fill_remainder ? 1 : 0);
                    fill_idx++;
                    break;
                }
                case Constraint::Type::Percent :
                {
                    rect.height = ((area.height * c.value) / 100);
                    break;
                }
            }
            int max_possible_h = std::max(0, area.bottom() - current_row);
            rect.height = std::min(rect.height, max_possible_h);
            current_row += rect.height;
            layout.emplace_back(rect);
        }
        return layout;
    }

    auto Layout::horizontal(Area area, const std::vector<Constraint>& constraints)
        -> std::vector<Area>
    {
        if(!area.is_valid())
        {
            return std::vector<Area>(constraints.size(),Area{area.row,area.col,0,0});
        }
        std::vector<Area> layout;
        int fill_count = 0;
        int used = 0;
        
        for (auto c: constraints)
        {
            switch (c.type) {
                case Constraint::Type::Fixed :
                {
                    used += c.value;
                    break;
                    
                }
                case Constraint::Type::Fill :
                {
                    fill_count++;
                    break;
                }
                case Constraint::Type::Percent :
                {
                    used += (area.width * c.value) / 100;
                    break;
                }
            }

        }

        int remaining = std::max(0,area.width - used);
        int fill_size = (fill_count > 0) ? remaining / fill_count : 0;
        int fill_remainder = (fill_count > 0) ? remaining % fill_count : 0;

        int current_col = area.col;
        int fill_idx = 0;

        for (auto c : constraints)
        {
            Area rect(area.row,current_col,area.width,area.height);

            switch (c.type) {
                case Constraint::Type::Fixed :
                {
                    rect.width = c.value;
                    break;
                    
                }
                case Constraint::Type::Fill :
                {   
                    rect.width = fill_size + (fill_idx < fill_remainder ? 1 : 0);
                    fill_idx++;
                    break;
                }
                case Constraint::Type::Percent :
                {
                    rect.width = ((area.width * c.value) / 100);
                    break;
                }
            }
            int max_possible_w = std::max(0, area.right() - current_col);
            rect.width = std::min(rect.width, max_possible_w);
            current_col += rect.width;
            layout.emplace_back(rect);
        }
        return layout;
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
                      const std::vector<Constraint>& row_constraints, 
                      const std::vector<Constraint>& col_constraints) -> Grid
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