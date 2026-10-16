#pragma once

#include "koto/widget.hpp"
#include <cstdint>
#include <string>
#include "koto/color.hpp"

namespace koto
{   
    enum class BorderStyle
    {
        None,
        Single,   // ─ │ ┌ ┐ └ ┘
        Double,   // ═ ║ ╔ ╗ ╚ ╝
        Rounded,  // ─ │ ╭ ╮ ╰ ╯
        Thick,    // ━ ┃ ┏ ┓ ┗ ┛
    };

    enum class TitleAlign
    {
        Left,
        Center,
        Right,
    };

    struct Borders
    {
        bool top = true, bottom = true, left = true, right = true;
    };

    class Box : public Widget
    {   
        
        public:
         
            Box();
            Box& fg(std::string_view fg);
            Box& bg(std::string_view bg);
            Box& title_fg(std::string_view fg);
            Box& title_bg(std::string_view bg);
            Box& fg(uint32_t fg);
            Box& bg(uint32_t bg);
            Box& title_fg(uint32_t fg);
            Box& title_bg(uint32_t bg);
            Box& filled(bool fill);
            Box& border_style(BorderStyle style);
            Box& borders(Borders borders);
            Box& title(std::string title);
            Box& title_align(TitleAlign align);
            void draw(Frame& frame, Area area) override;

        private:

            uint32_t m_fg = COLOR_DEFAULT, m_bg = COLOR_DEFAULT;
            uint32_t m_title_fg = COLOR_DEFAULT, m_title_bg = COLOR_DEFAULT;
            bool m_filled = false;
            BorderStyle m_style = BorderStyle::Single;
            struct Borders m_borders;
            std::string m_title = "";
            TitleAlign m_title_align = TitleAlign::Center;
    };
}
