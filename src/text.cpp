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

    auto Text::draw(Frame& frame, Area area) -> void
    {
        if (!area.is_valid()) return;
        
        uint32_t row = area.row;
        uint32_t col = area.col;

        const char* ptr = m_content.data();
        const char* end = ptr + m_content.size();

        while (ptr < end)
        {
            if (row >= area.bottom())
            {
                break;
            }

            char32_t cp = next_codepoint(ptr, end);
            frame.set_cell(row, col, {cp,m_fg,m_bg});

            col++;

            if (col >= area.right())
            {
                col = area.col;
                row++;
            }
        }
    }
}