#pragma once

#include "eztui/frame.hpp"
#include "eztui/layout.hpp"

namespace eztui
{
    class Widget {
        
        public:
            virtual ~Widget() = default;
            virtual void draw(Frame& frame, Rect area) = 0;

    };
}