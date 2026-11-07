#pragma once
#include <cstdint>
#include "koto/color.hpp"

namespace koto {

    enum class TextAttr : uint8_t
    {
        None        = 0,
        Bold        = 1 << 0,
        Dim         = 1 << 1,
        Italic      = 1 << 2,
        Underline   = 1 << 3,
        Reverse     = 1 << 4,
        Strike      = 1 << 5
    };

    inline constexpr uint8_t operator|(TextAttr a, TextAttr b) noexcept
    {
        return static_cast<uint8_t>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    inline constexpr uint8_t operator|(uint8_t a, TextAttr b) noexcept
    {
        return static_cast<uint8_t>(a | static_cast<uint8_t>(b));
    }

    struct Cell {
        char32_t ch = U' ';
        uint32_t fg = COLOR_DEFAULT;
        uint32_t bg = COLOR_DEFAULT;
        int width = 1;
        bool continuation = false;
        uint8_t attrs = 0;
        bool operator==(const Cell&) const = default;

    };

}