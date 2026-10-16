#pragma once
#include <cstdint>
#include "koto/color.hpp"

namespace koto {

    struct Cell {
        char32_t ch = U' ';
        uint32_t fg = COLOR_DEFAULT;
        uint32_t bg = COLOR_DEFAULT;
        int width = 1;
        bool continuation = false;
        bool operator==(const Cell&) const = default;

    };

}