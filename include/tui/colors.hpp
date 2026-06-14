#pragma once
#include <string>
#include <fmt/color.h>

namespace Tui {

    // --------------------------------------------
    // ENUMS
    // --------------------------------------------

    enum class FG {
        Default,
        Black, Red, Green, Yellow,
        Blue, Magenta, Cyan, White,

        BrightGreen,
        BrightYellow,
        BrightBlue,
        BrightCyan
    };

    enum class BG {
        Default,
        Black, Red, Green, Yellow,
        Blue, Magenta, Cyan, White,

        BrightGreen,
        BrightYellow,
        BrightBlue,
        BrightCyan
    };

    enum class Style {
        None      = 0,
        Bold      = 1 << 0,
        Italic    = 1 << 1,
        Underline = 1 << 2
    };
    
    std::string Paint(const std::string& text , FG f , BG b , Style s );

} 
