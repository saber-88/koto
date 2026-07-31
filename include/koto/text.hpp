#pragma once

#include "koto/widget.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include "color.hpp"

namespace koto
{
    class Text : public Widget
    {
        private:
            std::string m_content = "";
            uint32_t m_fg = COLOR_DEFAULT, m_bg = COLOR_DEFAULT;
        
        public:

            Text(std::string_view content = "");
            Text& fg(std::string_view fg);
            Text& bg(std::string_view bg);
            void draw(Frame &frame, Area area) override;

    };
}