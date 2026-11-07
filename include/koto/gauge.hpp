#pragma once

#include "koto/widget.hpp"
#include "koto/color.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace koto
{
    class Gauge : public Widget
    {
    public:
        Gauge();

        Gauge& ratio(float r);
        Gauge& percent(int p);
        Gauge& label(std::string_view label);
        Gauge& filled_char(char32_t ch);
        Gauge& empty_char(char32_t ch);
        Gauge& use_fractional_blocks(bool enable);

        Gauge& fg(Color fg);
        Gauge& bg(Color bg);

        Gauge& filled_fg(Color fg);
        Gauge& filled_bg(Color bg);

        Gauge& empty_fg(Color fg);
        Gauge& empty_bg(Color bg);

        Gauge& label_fg(Color fg);

        void draw(Frame& frame, Area area) override;

    private:
        float m_ratio = 0.0f;
        std::string m_label;
        bool m_fractional = true;
        char32_t m_filled_ch = U'█';
        char32_t m_empty_ch = U' ';

        uint32_t m_filled_fg = Color::Emerald;
        uint32_t m_filled_bg = COLOR_DEFAULT;
        uint32_t m_empty_fg = Color::DarkGray;
        uint32_t m_empty_bg = COLOR_DEFAULT;
        uint32_t m_label_fg = Color::White;
    };
}
