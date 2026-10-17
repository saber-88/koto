#pragma once

#include "koto/widget.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include <cstdint>
#include <string>
#include <string_view>

namespace koto
{
    class Rule : public Widget
    {
    public:
        Rule(std::string_view title = "");

        Rule& title(std::string_view title);
        Rule& title_align(TitleAlign align);
        Rule& style(BorderStyle style);
        Rule& fg(std::string_view fg);
        Rule& fg(uint32_t fg);
        Rule& bg(std::string_view bg);
        Rule& bg(uint32_t bg);
        Rule& title_fg(std::string_view fg);
        Rule& title_fg(uint32_t fg);
        Rule& title_bg(std::string_view bg);
        Rule& title_bg(uint32_t bg);

        void draw(Frame& frame, Area area) override;

    private:
        std::string m_title;
        TitleAlign m_title_align = TitleAlign::Center;
        BorderStyle m_style = BorderStyle::Single;
        uint32_t m_fg = COLOR_DEFAULT;
        uint32_t m_bg = COLOR_DEFAULT;
        uint32_t m_title_fg = COLOR_DEFAULT;
        uint32_t m_title_bg = COLOR_DEFAULT;
    };
}
