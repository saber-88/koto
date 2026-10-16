#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include "koto/text.hpp"
#include "koto/utf8.hpp"
#include <cstddef>
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
    Box& Box::fg(uint32_t fg)
    {
        m_fg = fg;
        return *this;
    }
    Box& Box::bg(uint32_t bg)
    {
        m_bg = bg;
        return *this;
    }
    Box& Box::title_fg(uint32_t fg)
    {
        m_title_fg = fg;
        return *this;
    }
    Box& Box::title_bg(uint32_t bg)
    {
        m_title_bg = bg;
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

    Box& Box::title_align(TitleAlign align)
    {
        m_title_align = align;
        return *this;
    }


    void Box::draw(Frame& frame, Area area)
    {
        if(!area.is_valid()) return;

        int row = area.row;
        int col = area.col;
        int right = area.right();
        int bottom = area.bottom();

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

        for (int r = row; r < bottom; r++)
        {
            for (int c = col; c < right; c++)
            {
                bool on_top    = (r == row);
                bool on_bottom = (r == bottom - 1);
                bool on_left   = (c == col);
                bool on_right  = (c == right - 1);
                bool on_border = on_top || on_bottom || on_left || on_right;

                // skip interior cells when not filled
                if (!on_border && !m_filled) continue;

                if (on_top && on_left)
                {
                    if (!m_borders.left) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {top_left,m_fg,m_bg});
                }
                else if (on_top && on_right)
                {
                    if (!m_borders.right) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {top_right,m_fg,m_bg});
                    
                }
                else if (on_bottom && on_left)
                {
                    if (!m_borders.left) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {bottom_left,m_fg,m_bg});
                    
                }
                else if (on_bottom && on_right)
                {
                    if (!m_borders.right) {
                        frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                        continue;
                    }
                    frame.set_cell(r, c, {bottom_right,m_fg,m_bg});
                    
                }
                else if 
                (
                    on_top && 
                    !on_left && !on_right &&
                    m_borders.top
                )
                {
                    frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                    
                }
                else if 
                (
                    on_bottom && 
                    !on_left && !on_right &&
                    m_borders.bottom
                )
                {
                    frame.set_cell(r, c, {horizontal,m_fg,m_bg});
                    
                }
                else if 
                (
                    on_left && 
                    !on_top && !on_bottom &&
                    m_borders.left
                )
                {
                    frame.set_cell(r, c, {vertical,m_fg,m_bg});
                    
                }
                else if 
                (
                    on_right && 
                    !on_top && !on_bottom &&
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

        if ((!m_title.empty()) && (text_width(m_title) + 4 <= area.width))
        {
            int title_width = text_width(m_title);
            int title_col = area.col + 3;
            if (m_title_align == TitleAlign::Center)
            {
                title_col = area.col + (area.width - title_width) / 2;
            }
            else if (m_title_align == TitleAlign::Right)
            {
                title_col = area.right() - title_width - 2;
            }

            frame.set_cell(row, title_col - 1, {U' ', m_title_fg, m_title_bg});
            Area textArea = {
                row,
                title_col,
                title_width,
                1
            };
            Text(m_title).bg(m_title_bg).fg(m_title_fg).draw(frame, textArea);
            frame.set_cell(row, title_col + title_width, {U' ', m_title_fg, m_title_bg});
        }

    }
}