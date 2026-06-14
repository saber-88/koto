#include "tui/colors.hpp"
#include <fmt/color.h>
#include <string>
namespace Tui {
    
    inline fmt::color fg_to_fmt(FG fg) {
        using C = fmt::color;
        switch (fg) {
            case FG::Black: return C::black;
            case FG::Red: return C::red;
            case FG::Green: return C::green;
            case FG::Yellow: return C::yellow;
            case FG::Blue: return C::blue;
            case FG::Magenta: return C::magenta;
            case FG::Cyan: return C::cyan;
            case FG::White: return C::white;
            case FG::BrightGreen: return C::light_green;
            case FG::BrightYellow: return C::light_yellow;
            case FG::BrightBlue: return C::light_blue;
            case FG::BrightCyan: return C::light_cyan;
            case FG::Default: return fmt::v12::color::white;
            default: return C::white;
        }
    }
    
    inline fmt::color bg_to_fmt(BG bg) {
        using C = fmt::color;
        switch (bg) {
            case BG::Black: return C::black;
            case BG::Red: return C::red;
            case BG::Green: return C::green;
            case BG::Yellow: return C::yellow;
            case BG::Blue: return C::blue;
            case BG::Magenta: return C::magenta;
            case BG::Cyan: return C::cyan;
            case BG::White: return C::white;
            case BG::BrightGreen: return C::light_green;
            case BG::BrightYellow: return C::light_yellow;
            case BG::BrightBlue: return C::light_blue;
            case BG::BrightCyan: return C::light_cyan;
            case BG::Default: return static_cast<C>(0);
            default: return static_cast<C>(0);
        }
    }
    
    inline fmt::emphasis style_to_fmt(Style s) {
        using E = fmt::emphasis;
        switch (s) {
            case Tui::Style::Bold: return E::bold;
            case Tui::Style::Underline: return E::underline;
            case Tui::Style::Italic: return E::italic;
            case Tui::Style::None: return static_cast<E>(0);
            default: return static_cast<E>(0);
        }
    }
    
    std::string Paint(const std::string& text , FG f , BG b , Style s) {
        
        if (b == Tui::BG::Default) {
            fmt::color fgColor = fg_to_fmt(f);
            fmt::emphasis emp = style_to_fmt(s);
            
            auto style = fmt::fg(fgColor) | emp;
            return  fmt::format("{}", fmt::styled(text, style));
        }
        else if (f == Tui::FG::Default) {
            fmt::color bgColor = bg_to_fmt(b);
            fmt::emphasis emp = style_to_fmt(s);
            
            auto style = fmt::bg(bgColor) | emp;
            return  fmt::format("{}", fmt::styled(text, style));
        }
        else {
        
            fmt::color fgColor = fg_to_fmt(f);
            fmt::color bgColor = bg_to_fmt(b);
            fmt::emphasis emp = style_to_fmt(s);
            auto style = fmt::fg(fgColor) | fmt::bg(bgColor) | emp ;
            return fmt::format("{}", fmt::styled(text, style));
        }
        
    }

    
}
