#include "eztui/box.hpp"
#include "eztui/cell.hpp"
#include "eztui/frame.hpp"
#include "eztui/layout.hpp"
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace eztui
{
    
    Box::Box(std::string_view fg, std::string_view bg, bool filled):m_filled(filled)
    {
        auto parse = [](std::string_view c) -> uint32_t
        {
            if (c.empty())
            {
                return COLOR_DEFAULT;
            }
            if (c[0] == '#')
            {
                c.remove_prefix(1);
            }

            char buf[7];
            c.copy(buf, c.size());

            return static_cast<uint32_t>(std::strtoul(buf,nullptr,16));
        };

        m_fg = parse(fg);
        m_bg = parse(bg);
    }

    void Box::draw(Frame& frame, Rect area)
    {
        uint32_t row = area.row;
        uint32_t col = area.col;
        uint32_t right = area.right();
        uint32_t bottom = area.bottom();

        for (uint32_t r = row; r < bottom; r++)
        {
            for (uint32_t c = col; c < right; c++)
            {
                if (r == row && c == col)
                {
                    frame.set_cell(r, c, {U'┌',m_fg,m_bg});
                }
                else if (r == row && c == right - 1)
                {
                    frame.set_cell(r, c, {U'┐',m_fg,m_bg});
                    
                }
                else if (r == bottom - 1 && c == col)
                {
                    frame.set_cell(r, c, {U'└',m_fg,m_bg});
                    
                }
                else if (r == bottom - 1 && c == right - 1)
                {
                    frame.set_cell(r, c, {U'┘',m_fg,m_bg});
                    
                }
                else if 
                (
                    (r == row || r == bottom - 1) && 
                    (c != col && c != right - 1)
                )
                {
                    frame.set_cell(r, c, {U'─',m_fg,m_bg});
                    
                }
                else if 
                (
                    (c == col || c == right - 1) && 
                    (r != row && r != bottom - 1)
                )
                {
                    frame.set_cell(r, c, {U'│',m_fg,m_bg});
                    
                }
                else {
                    if(m_filled){
                        
                        frame.set_cell(r, c, {U' ',COLOR_DEFAULT,m_bg});
                    }
                }

            }
        }

    }
}