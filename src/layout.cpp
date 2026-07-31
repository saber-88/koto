#include "koto/layout.hpp"
#include <cstdint>
#include <vector>

namespace koto
{
    auto Layout::vertical(Area area, std::vector<Constraint> constraints)
        -> std::vector<Area>
    {
        if(!area.is_valid())
        {
            return std::vector<Area>(constraints.size(),Area{area.row,area.col,0,0});
        }
        std::vector<Area> layout;
        uint32_t fill_count = 0;
        uint32_t used = 0;
        
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

        uint32_t fill_size = (fill_count > 0) ? (area.height - used) / fill_count : 0;

        uint32_t current_row = area.row;

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
                    rect.height = fill_size;
                    break;
                }
                case Constraint::Type::Percent :
                {
                    rect.height = ((area.height * c.value) / 100);
                    break;
                }
            }
            current_row += rect.height;
            layout.emplace_back(rect);

        }
        return layout;
    }

    auto Layout::horizontal(Area area, std::vector<Constraint> constraints)
        -> std::vector<Area>
    {
        if(!area.is_valid())
        {
            return std::vector<Area>(constraints.size(),Area{area.row,area.col,0,0});
        }
        std::vector<Area> layout;
        uint32_t fill_count = 0;
        uint32_t used = 0;
        
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

        uint32_t fill_size = (fill_count > 0) ? (area.width - used) / fill_count : 0;

        uint32_t current_col = area.col;

        for (auto c : constraints)
        {
            Area rect(area.row,current_col,area.width,area.height);
            rect.row = area.row;
            rect.height = area.height;
            rect.col = current_col;

            switch (c.type) {
                case Constraint::Type::Fixed :
                {
                    rect.width = c.value;
                    break;
                    
                }
                case Constraint::Type::Fill :
                {   
                    rect.width = fill_size;
                    break;
                }
                case Constraint::Type::Percent :
                {
                    rect.width = ((area.width * c.value) / 100);
                    break;
                }
            }
            current_col += rect.width;
            layout.emplace_back(rect);
        }
        return layout;
    }

    auto Layout::center(Area area, uint32_t width, uint32_t height) -> Area
    {
        if(!area.is_valid())
        {
            return {area.row,area.col,0,0};
        }
        uint32_t r = area.row + ( area.height - height) / 2;
        uint32_t c = area.col + (area.width - width) / 2;

        return {r,c,width,height};
    }
}