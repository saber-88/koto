#include "koto/rule.hpp"
#include "koto/frame.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <string>

namespace koto
{
    Rule::Rule(std::string_view title)
        : m_title(title)
    {
    }

    Rule& Rule::title(std::string_view title)
    {
        m_title = std::string(title);
        return *this;
    }

    Rule& Rule::title_align(TitleAlign align)
    {
        m_title_align = align;
        return *this;
    }

    Rule& Rule::style(BorderStyle style)
    {
        m_style = style;
        return *this;
    }

    Rule& Rule::fg(std::string_view fg)
    {
        m_fg = parse_color(fg);
        return *this;
    }

    Rule& Rule::fg(uint32_t fg)
    {
        m_fg = fg;
        return *this;
    }

    Rule& Rule::bg(std::string_view bg)
    {
        m_bg = parse_color(bg);
        return *this;
    }

    Rule& Rule::bg(uint32_t bg)
    {
        m_bg = bg;
        return *this;
    }

    Rule& Rule::title_fg(std::string_view fg)
    {
        m_title_fg = parse_color(fg);
        return *this;
    }

    Rule& Rule::title_fg(uint32_t fg)
    {
        m_title_fg = fg;
        return *this;
    }

    Rule& Rule::title_bg(std::string_view bg)
    {
        m_title_bg = parse_color(bg);
        return *this;
    }

    Rule& Rule::title_bg(uint32_t bg)
    {
        m_title_bg = bg;
        return *this;
    }

    void Rule::draw(Frame& frame, Area area)
    {
        if (!area.is_valid() || area.width <= 0 || area.height <= 0)
        {
            return;
        }

        // Draw horizontal rule at vertical center of the area
        int row = area.row + (area.height - 1) / 2;

        char32_t line_ch = U'─';
        switch (m_style)
        {
            case BorderStyle::Double:
                line_ch = U'═';
                break;
            case BorderStyle::Thick:
                line_ch = U'━';
                break;
            case BorderStyle::None:
                line_ch = U' ';
                break;
            case BorderStyle::Single:
            case BorderStyle::Rounded:
            default:
                line_ch = U'─';
                break;
        }

        // Fill entire row with line character
        for (int c = area.col; c < area.right(); ++c)
        {
            frame.set_cell(row, c, {line_ch, m_fg, m_bg});
        }

        if (m_title.empty())
        {
            return;
        }

        // Add padding around title: " Title "
        std::string padded = " " + m_title + " ";
        int title_w = text_width(padded);

        if (title_w > area.width)
        {
            return; // Not enough space for title, keep solid line
        }

        int start_col = area.col;
        switch (m_title_align)
        {
            case TitleAlign::Left:
                start_col = area.col + 2;
                break;
            case TitleAlign::Right:
                start_col = area.right() - 2 - title_w;
                break;
            case TitleAlign::Center:
            default:
                start_col = area.col + (area.width - title_w) / 2;
                break;
        }

        start_col = std::max(area.col, std::min(start_col, area.right() - title_w));

        uint32_t t_fg = (m_title_fg != COLOR_DEFAULT) ? m_title_fg : m_fg;
        uint32_t t_bg = (m_title_bg != COLOR_DEFAULT) ? m_title_bg : m_bg;

        std::string_view remaining = padded;
        int col = start_col;
        while (!remaining.empty() && col < area.right())
        {
            char32_t cp = next_codepoint(remaining);
            int w = codepoint_width(cp);
            frame.set_cell(row, col, {cp, t_fg, t_bg});
            col += w;
        }
    }
}
