#include "koto/text.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include "koto/utf8.hpp"
#include <cstdint>
#include <string_view>

namespace koto
{


    Text::Text(std::string_view content):m_content(content)
    {}

    Text& Text::overflow(TextOverflow o)
    {
        m_overflow = o;
        return *this;
    }
    Text& Text::wrap()
    {
        m_text_wrap = TextWrap::Wrap;
        return *this;
    }
    Text& Text::clip()
    {
        m_overflow = TextOverflow::Clip;
        return *this;
    }
    Text& Text::ellipsis()
    {
        m_overflow = TextOverflow::Ellipsis;
        return *this;
    }

    Text& Text::fg(std::string_view fg)
    {
        m_fg = parse_color(fg);
        return *this;
    }
    Text& Text::bg(std::string_view bg)
    {
        m_bg = parse_color(bg);
        return *this;
    }
    Text& Text::fg(uint32_t fg)
    {
        m_fg = fg;
        return *this;
    }
    Text& Text::bg(uint32_t bg)
    {
        m_bg = bg;
        return *this;
    }

    auto Text::draw(Frame& frame, Area area) -> void
    {
        if (!area.is_valid()) return;
        
        int row = area.row;
        int col = area.col;
        bool skipping_current_line = false;

        const char* ptr = m_content.data();
        const char* end = ptr + m_content.size();

        while (ptr < end)
        {
            char32_t cp = next_codepoint(ptr, end);

            // Hard newline: always advance to next row and reset column
            if (cp == U'\n')
            {
                col = area.col;
                row++;
                skipping_current_line = false;
                if (row >= area.bottom()) return;
                continue;
            }

            if (cp == U'\r')
            {
                continue;
            }

            // If current line already exceeded width, skip characters until next newline
            if (skipping_current_line)
            {
                continue;
            }

            // Tab stop alignment (4 spaces)
            if (cp == U'\t')
            {
                int tab_size = 4;
                int rel = col - area.col;
                int next_stop = ((rel / tab_size) + 1) * tab_size;
                int target_col = area.col + next_stop;
                while (col < target_col && col < area.right())
                {
                    frame.set_cell(row, col, {U' ', m_fg, m_bg});
                    col++;
                }
                continue;
            }

            int w = codepoint_width(cp);
            
            if (col + w > area.right())
            {
                if (m_text_wrap == TextWrap::Wrap)
                {
                    col = area.col;
                    row++;

                    if (row >= area.bottom())
                    {
                        if (m_overflow == TextOverflow::Ellipsis && area.right() > area.col)
                        {
                            frame.set_cell(row - 1, area.right() - 1, {U'…', m_fg, m_bg});
                        }
                        return;
                    }
                }
                else
                {
                    if (m_overflow == TextOverflow::Ellipsis && area.right() > area.col)
                    {
                        frame.set_cell(row, area.right() - 1, {U'…', m_fg, m_bg});
                    }
                    skipping_current_line = true;
                    continue;
                }
            }

            frame.set_cell(row, col, {cp, m_fg, m_bg});
            col += w;
        }
    }
}