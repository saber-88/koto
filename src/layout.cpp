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
        if(!area.is_valid())
        {
            return {area.row,area.col,0,0};
        }
        int r = area.row + ( area.height - height) / 2;
        int c = area.col + (area.width - width) / 2;

        r = (r < 0) ? 0 : r;
        c = (c < 0) ? 0 : c;

        return {r,c,width,height};
    }
}