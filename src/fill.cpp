#include "koto/fill.hpp"
#include "koto/color.hpp"
#include "koto/frame.hpp"

namespace koto
{
    Fill::Fill(Color bg)
        : m_bg(bg.value)
    {}

    Fill& Fill::bg(Color color)
    {
        m_bg = color.value;
        return *this;
    }

    Fill& Fill::fg(Color color)
    {
        m_fg = color.value;
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
