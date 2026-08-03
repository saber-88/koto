#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <unistd.h>

namespace koto
{
    
    Box::Box(){}

    Box& Box::fg(std::string_view fg)
    {
        m_fg = parse_color(fg);
        return *this;
    }
    Box& Box::bg(std::string_view bg)
    {
        m_bg = parse_color(bg);
        return *this;
    }
    Box& Box::title_fg(std::string_view fg)
    {
        m_title_fg = parse_color(fg);
        return *this;
    }
    Box& Box::title_bg(std::string_view bg)
    {
        m_title_bg = parse_color(bg);
        return *this;
    }

    Box& Box::filled(bool fill)
    {
        m_filled = fill;
        return *this;
    }

    Box& Box::border_style(BorderStyle style)
    {
        m_style = style;
        return *this;
    }

    Box& Box::borders(Borders borders)
    {
        m_borders = borders;
        return *this;
    }

    Box& Box::title(std::string title)
    {
        m_title = title;
        return *this;
    }


    void Box::draw(Frame& frame, Area area)
    {
        if(!area.is_valid()) return;

        uint32_t row = area.row;
        uint32_t col = area.col;
        uint32_t right = area.right();
        uint32_t bottom = area.bottom();

        char32_t top_left;
        char32_t top_right;
        char32_t bottom_left;
        char32_t bottom_right;
        char32_t horizontal;
        char32_t vertical;


        switch (m_style)
        {
            case BorderStyle::None:
            {
                top_left = U' ';
                top_right = U' ';
                bottom_left = U' ';
                bottom_right = U' ';
                horizontal = U' ';
                vertical = U' ';
                break;
            }
            case BorderStyle::Single:
            {
                top_left = U'┌';
                top_right = U'┐';
                bottom_left = U'└';
                bottom_right = U'┘';
                horizontal = U'─';
                vertical = U'│';
                break;
            }
            case BorderStyle::Double:
            {
                top_left = U'╔';
                top_right = U'╗';
                bottom_left = U'╚';
                bottom_right = U'╝';
                horizontal = U'═';
                vertical = U'║';
                break;
            }
            case BorderStyle::Rounded:
            {
                top_left = U'╭';
                top_right = U'╮';
                bottom_left = U'╰';
                bottom_right = U'╯';
                horizontal = U'─';
                vertical = U'│';
                break;
            }
            case BorderStyle::Thick:
            {
                top_left = U'┏';
                top_right = U'┓';
                bottom_left = U'┗';
                bottom_right = U'┛';
                horizontal = U'━';
                vertical = U'┃';
                break;
            }
        }

        for (uint32_t r = row; r < bottom; r++)
        {
            for (uint32_t c = col; c < right; c++)
            {
                if (r == row && c == col)
                {
                    if (!m_borders.left) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {top_left,m_fg,m_bg});
                }
                else if (r == row && c == right - 1)
                {
                    if (!m_borders.right) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {top_right,m_fg,m_bg});
                    
                }
                else if (r == bottom - 1 && c == col)
                {
                    if (!m_borders.left) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {bottom_left,m_fg,m_bg});
                    
                }
                else if (r == bottom - 1 && c == right - 1)
                {
                    if (!m_borders.right) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {bottom_right,m_fg,m_bg});
                    
                }
                else if 
                (
                    (r == row) && 
                    (c != col && c != right - 1) &&
                    m_borders.top
                )
                {
                    frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                    
                }
                else if 
                (
                    (r == bottom - 1) && 
                    (c != col && c != right - 1) &&
                    m_borders.bottom
                )
                {
                    frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                    
                }
                else if 
                (
                    (c == col) && 
                    (r != row && r != bottom - 1) &&
                    m_borders.left
                )
                {
                    frame.set_cell(r, c, {vertical,m_fg,m_bg});
                    
                }
                else if 
                (
                    (c == right - 1) && 
                    (r != row && r != bottom - 1) &&
                    m_borders.right
                )
                {
                    frame.set_cell(r, c, {vertical,m_fg,m_bg});
                    
                }
                else {
                    if(m_filled){
                        
                        frame.set_cell(r, c, {U' ',m_fg,m_bg});
                    }
                }

            }
        }

        if ((!m_title.empty()) && (m_title.size() < area.width))
        {
            uint32_t center_col = area.col + (area.width - m_title.size()) / 2;

            frame.set_cell(row, center_col - 1, {U' ',m_title_fg,m_title_bg});
            for (int i = 0; i < m_title.size(); i++)
            {

                frame.set_cell(row, center_col + i, {(char32_t)m_title[i],m_title_fg,m_title_bg});
            }
            frame.set_cell(row, center_col + m_title.size(), {U' ',m_title_fg,m_title_bg});
            
        }

    }
}