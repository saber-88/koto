#pragma once

#include "koto/frame.hpp"
#include "koto/layout.hpp"

namespace koto
{
    class Widget {
        
        public:
            virtual ~Widget() = default;
            virtual void draw(Frame& frame, Area area) = 0;

    };
}