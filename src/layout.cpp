#include "eztui/layout.hpp"
#include <cstdint>
#include <vector>

namespace eztui
{
    std::vector<Rect> Layout::vertical(Rect area, std::vector<Constraint> constraints)
    {
        std::vector<Rect> layout;
        uint32_t fill_count = 0;
        uint32_t used = 0;
        
        for (auto c: constraints)
        {
            if (c.type == Constraint::Type::Fixed)
            {
                used += c.value;
            }

            else if (c.type == Constraint::Type::Percent)
            {
                used += (area.height * c.value) / 100;
            }
            else if (c.type == Constraint::Type::Fill)
            {
                fill_count++;
            }
        }

        uint32_t fill_size = (fill_count > 0) ? (area.height - used) / fill_count : 0;

        uint32_t current_row = area.row;

        for (auto c : constraints)
        {
            Rect rect;
            rect.col = area.col;
            rect.width = area.width;
            rect.row = current_row;
            
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

    std::vector<Rect> Layout::horizontal(Rect area, std::vector<Constraint> constraints)
    {
        std::vector<Rect> layout;
        uint32_t fill_count = 0;
        uint32_t used = 0;
        
        for (auto c: constraints)
        {
            if (c.type == Constraint::Type::Fixed)
            {
                used += c.value;
            }

            else if (c.type == Constraint::Type::Percent)
            {
                used += (area.width * c.value) / 100;
            }
            else if (c.type == Constraint::Type::Fill)
            {
                fill_count++;
            }
        }

        uint32_t fill_size = (fill_count > 0) ? (area.width - used) / fill_count : 0;

        uint32_t current_col = area.col;

        for (auto c : constraints)
        {
            Rect rect;
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
}