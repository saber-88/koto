#pragma once

#include "colors.hpp"
#include <string>

struct Cell{

    std::string cellItem;
    Tui::FG fg = Tui::FG::Default;
    Tui::BG bg = Tui::BG::Default;
    Tui::Style style = Tui::Style::None;
    
    bool operator==(const Cell& ) const = default;
    
};