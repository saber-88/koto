#include "koto/fill.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"

namespace koto
{
    Fill::Fill(uint32_t bg)
        : m_bg(bg)
    {}

    Fill::Fill(std::string_view bg)
        : m_bg(parse_color(bg))
    {}

    Fill& Fill::bg(uint32_t color)
    {
        m_bg = color;
        return *this;
    }

    Fill& Fill::bg(std::string_view color)
    {
        m_bg = parse_color(color);
        return *this;
    }

    Fill& Fill::fg(uint32_t color)
    {
        m_fg = color;
        return *this;
    }

    Fill& Fill::fg(std::string_view color)
    {
        m_fg = parse_color(color);
        return *this;
    }

    Fill& Fill::char_code(char32_t ch)
    {
        m_ch = ch;
        return *this;
    }

    Fill& Fill::id(Id id)
    {
        m_tag_id = id;
        return *this;
    }

    void Fill::draw(Frame& frame, Area area)
    {
        frame.fill(area, m_ch, m_fg, m_bg, m_tag_id);
    }

    Clear& Clear::id(Id id)
    {
        m_tag_id = id;
        return *this;
    }

    void Clear::draw(Frame& frame, Area area)
    {
        frame.fill(area, U' ', COLOR_DEFAULT, COLOR_DEFAULT, m_tag_id);
    }
}
