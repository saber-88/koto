#pragma once

#include "koto/widget.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include "koto/color.hpp"

namespace koto
{
    enum class TextWrap
    {
        None,
        Wrap,
    };

    enum class TextOverflow
    {
        Clip,
        Ellipsis,
    };

    class Text : public Widget
    {
        public:
            using TextWrap = koto::TextWrap;
            using TextOverflow = koto::TextOverflow;
            Text(std::string_view content = "");
            Text& fg(std::string_view fg);
            Text& bg(std::string_view bg);
            Text& fg(uint32_t fg);
            Text& bg(uint32_t bg);
            Text& wrap();
            Text& clip();
            Text& ellipsis();
            Text& overflow(TextOverflow o);
            void draw(Frame &frame, Area area) override;
            
        private:
            TextOverflow m_overflow = TextOverflow::Clip;
            TextWrap m_text_wrap = TextWrap::None;
            std::string m_content = "";
            uint32_t m_fg = COLOR_DEFAULT, m_bg = COLOR_DEFAULT;
        

    };
}