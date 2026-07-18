#pragma once
#include <cstdint>

namespace eztui {

    static constexpr uint32_t COLOR_DEFAULT = 0xFF000000;
    struct Cell {
        char32_t ch = U' ';
        uint32_t fg = COLOR_DEFAULT;
        uint32_t bg = COLOR_DEFAULT;
        
        bool operator==(const Cell&) const = default;
    };

}