#pragma once

#include "eztui/cell.hpp"
#include "eztui/widget.hpp"
#include <cstdint>
#include <string_view>

namespace eztui
{
    class Box : public Widget
    {   
        private:

            uint32_t m_fg , m_bg;
            bool m_filled;

        public:

            Box(std::string_view fg = "", std::string_view bg = "", bool filled = false);
            void draw(Frame& frame, Rect are) override;
    };
}