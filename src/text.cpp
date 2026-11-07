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

            if (m_text_wrap == TextWrap::Wrap)
            {
                // Word-wrapping implementation: breaks between words at whitespace
                std::string_view line_rem = line;
                while (!line_rem.empty() && row < area.bottom())
                {
                    // Skip leading whitespace on a freshly wrapped line
                    if (col == area.col && (line_rem.front() == ' ' || line_rem.front() == '\t'))
                    {
                        line_rem.remove_prefix(1);
                        continue;
                    }

                    // Peek the next token (either a whitespace run or a word run)
                    bool is_space = (line_rem.front() == ' ' || line_rem.front() == '\t');
                    size_t token_len = 0;
                    int token_width = 0;
                    std::string_view scan = line_rem;

                    while (!scan.empty())
                    {
                        char cp = scan.front();
                        bool cp_is_space = (cp == ' ' || cp == '\t');
                        if (cp_is_space != is_space) break;

                        std::string_view prev_scan = scan;
                        char32_t full_cp = next_codepoint(scan);
                        token_len += (prev_scan.size() - scan.size());
                        if (full_cp == '\t')
                        {
                            token_width += 4;
                        }
                        else
                        {
                            token_width += codepoint_width(full_cp);
                        }
                    }

                    std::string_view token = line_rem.substr(0, token_len);

                    // If it's a word and doesn't fit on the current line (and we're not at the line start):
                    if (!is_space && col + token_width > area.right() && col > area.col)
                    {
                        row++;
                        col = area.col;
                        if (row >= area.bottom())
                        {
                            if (m_overflow == TextOverflow::Ellipsis && area.right() > area.col)
                            {
                                frame.set_cell(row - 1, area.right() - 1, {U'…', m_fg, m_bg});
                            }
                            return;
                        }
                    }

                    // Render token characters
                    std::string_view token_rem = token;
                    while (!token_rem.empty())
                    {
                        char32_t cp = next_codepoint(token_rem);
                        if (cp == U'\r') continue;
                        int w = (cp == U'\t') ? 4 : codepoint_width(cp);

                        if (col + w > area.right())
                        {
                            row++;
                            col = area.col;
                            if (row >= area.bottom())
                            {
                                if (m_overflow == TextOverflow::Ellipsis && area.right() > area.col)
                                {
                                    frame.set_cell(row - 1, area.right() - 1, {U'…', m_fg, m_bg});
                                }
                                return;
                            }
                            if (cp == U' ' || cp == U'\t') continue;
                        }

                        if (cp == U'\t')
                        {
                            for (int i = 0; i < 4 && col < area.right(); ++i)
                            {
                                frame.set_cell(row, col++, {U' ', m_fg, m_bg});
                            }
                        }
                        else
                        {
                            frame.set_cell(row, col, Cell{cp, m_fg, m_bg, w, false, m_attrs});
                            col += w;
                        }
                    }

                    line_rem.remove_prefix(token_len);
                }
            }
            else
            {
                // NoWrap mode with optional clipping and ellipsis
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
                        if (m_overflow == TextOverflow::Ellipsis && area.right() > area.col)
                        {
                            frame.set_cell(row, area.right() - 1, {U'…', m_fg, m_bg});
                        }
                        skipping = true;
                        continue;
                    }

                    frame.set_cell(row, col, Cell{cp, m_fg, m_bg, w, false, m_attrs});
                    col += w;
                }
            }

            content = next_content;
            row++;
        }
    }
}