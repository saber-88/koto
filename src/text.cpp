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

    Text& Text::fg(Color fg)
    {
        m_fg = fg;
        return *this;
    }
    Text& Text::bg(Color bg)
    {
        m_bg = bg;
        return *this;
    }

    Text& Text::align(TextAlign a)
    {
        m_align = a;
        return *this;
    }

    Text& Text::bold(bool b)
    {
        if (b) m_attrs = static_cast<uint8_t>(m_attrs | static_cast<uint8_t>(TextAttr::Bold));
        else   m_attrs = static_cast<uint8_t>(m_attrs & static_cast<uint8_t>(~static_cast<unsigned int>(TextAttr::Bold)));
        return *this;
    }

    Text& Text::dim(bool d)
    {
        if (d) m_attrs = static_cast<uint8_t>(m_attrs | static_cast<uint8_t>(TextAttr::Dim));
        else   m_attrs = static_cast<uint8_t>(m_attrs & static_cast<uint8_t>(~static_cast<unsigned int>(TextAttr::Dim)));
        return *this;
    }

    Text& Text::italic(bool i)
    {
        if (i) m_attrs = static_cast<uint8_t>(m_attrs | static_cast<uint8_t>(TextAttr::Italic));
        else   m_attrs = static_cast<uint8_t>(m_attrs & static_cast<uint8_t>(~static_cast<unsigned int>(TextAttr::Italic)));
        return *this;
    }

    Text& Text::underline(bool u)
    {
        if (u) m_attrs = static_cast<uint8_t>(m_attrs | static_cast<uint8_t>(TextAttr::Underline));
        else   m_attrs = static_cast<uint8_t>(m_attrs & static_cast<uint8_t>(~static_cast<unsigned int>(TextAttr::Underline)));
        return *this;
    }

    Text& Text::reverse(bool r)
    {
        if (r) m_attrs = static_cast<uint8_t>(m_attrs | static_cast<uint8_t>(TextAttr::Reverse));
        else   m_attrs = static_cast<uint8_t>(m_attrs & static_cast<uint8_t>(~static_cast<unsigned int>(TextAttr::Reverse)));
        return *this;
    }

    Text& Text::attr(TextAttr a)
    {
        m_attrs |= static_cast<uint8_t>(a);
        return *this;
    }

    auto Text::draw(Frame& frame, Area area) -> void
    {
        if (!area.is_valid()) return;
        
        int row = area.row;
        std::string_view content = m_content;

        while (!content.empty() && row < area.bottom())
        {
            size_t newline_pos = content.find('\n');
            std::string_view line = (newline_pos != std::string_view::npos) ? content.substr(0, newline_pos) : content;
            std::string_view next_content = (newline_pos != std::string_view::npos) ? content.substr(newline_pos + 1) : std::string_view{};

            // Calculate visual width of this line if alignment is Center or Right
            int start_col = area.col;
            if (m_align != TextAlign::Left)
            {
                int line_w = 0;
                std::string_view scan = line;
                while (!scan.empty())
                {
                    char32_t cp = next_codepoint(scan);
                    if (cp == U'\r') continue;
                    if (cp == U'\t')
                    {
                        int tab_size = 4;
                        line_w = ((line_w / tab_size) + 1) * tab_size;
                    }
                    else
                    {
                        line_w += codepoint_width(cp);
                    }
                }

                if (line_w < area.width)
                {
                    if (m_align == TextAlign::Center)
                    {
                        start_col = area.col + (area.width - line_w) / 2;
                    }
                    else if (m_align == TextAlign::Right)
                    {
                        start_col = area.col + (area.width - line_w);
                    }
                }
            }

            int col = start_col;
            bool skipping = false;
            std::string_view line_rem = line;

            while (!line_rem.empty())
            {
                char32_t cp = next_codepoint(line_rem);
                if (cp == U'\r') continue;

                if (skipping) continue;

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
                        skipping = true;
                        continue;
                    }
                }

                frame.set_cell(row, col, Cell{cp, m_fg, m_bg, w, false, m_attrs});
                col += w;
            }

            content = next_content;
            row++;
        }
    }
}