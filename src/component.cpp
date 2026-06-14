#include "tui/component.hpp"
#include "tui/colors.hpp"
#include <cstddef>
#include <fmt/base.h>
#include <termios.h>
#include <vector>


// --------------------------------------------
// CONSTRUCTORS
// --------------------------------------------

Tui::Component::Component(){};

// --------------------------------------------
// TEXT CONSTRUCTORS
// --------------------------------------------


Tui::Component::Text::Text(size_t row , size_t col , const std::string& text , FG f , BG b , Style s)
:t_row(row),t_col(col),t_text(text) , t_fg(f) , t_bg(b) , t_style(s) {}

Tui::Component::Text::Text(const std::string& text , FG f , BG b , Style s) 
:t_text(text) , t_fg(f) , t_bg(b) , t_style(s) {
    t_row = 99999;
    t_col = 99999;
};



// --------------------------------------------
// BOX CONSTRUCTORS
// --------------------------------------------

Tui::Component::Box::Box(size_t row , size_t col , size_t height , size_t width, FG f , BG b ) 
: b_row(row) , b_col(col) , b_height(height) , b_width(width) , b_fg(f) , b_bg(b){}


Tui::Component::Box::Box(size_t height , size_t width , FG f , BG b )
: b_height(height) , b_width(width) , b_fg(f) , b_bg(b){
    b_row = 99999;
    b_col = 99999;
}


// --------------------------------------------
// SQUARE CONSTRUCTORS
// --------------------------------------------
Tui::Component::Square::Square(size_t row , size_t col , size_t side , FG f , BG b ) 
: s_row(row) , s_col(col) , s_side(side) , s_fg(f) , s_bg(b){}


Tui::Component::Square::Square( size_t side , FG f , BG b ): s_side(side) , s_fg(f) , s_bg(b){
    s_row = 99999;
    s_col = 99999;
}


// --------------------------------------------
// DRAW TEXT
// --------------------------------------------


void Tui::Component::Text::drawText(Screen& s) {

    size_t m_cols = s.getCols();
    size_t m_rows = s.getRows();

    if (t_row == 99999 && t_col == 99999) {
        t_row = m_rows / 2;
        t_col = (m_cols - t_text.size()) / 2;
    }
    
    
    std::vector<Cell>& buff = s.getBackBuffer();
    
    auto putChar = [&](size_t idx, const std::string& str , size_t i) {
    
        buff.at(idx).cellItem = str[i];
        buff.at(idx).fg = this->t_fg;
        buff.at(idx).bg = this->t_bg;
        buff.at(idx).style = this->t_style;
        
    };
    
    for (size_t i {0}; i < t_text.size() && t_col + i < m_cols; ++i) {
        size_t idx = t_row * m_cols + t_col + i;
        putChar(idx, t_text , i );
    }
    
    
}

// --------------------------------------------
// DRAW BOX
// --------------------------------------------


void Tui::Component::Box::drawBox(Screen& s) {
    size_t m_cols = s.getCols();
    size_t m_rows = s.getRows();

    if (b_row == 99999 && b_col == 99999) {
        b_row = (m_rows - b_height) / 2;
        b_col = (m_cols - b_width) / 2;
    }

    auto& buff = s.getBackBuffer();

    auto putChar = [&](size_t r, size_t c, const std::string& ch) {
        size_t idx = r * m_cols + c;
        buff.at(idx).cellItem = ch;
        buff.at(idx).fg = b_fg;
        buff.at(idx).bg = b_bg;
        buff.at(idx).style = Style::None;
    };

    for (size_t r = 0; r < b_height; ++r) {
        for (size_t c = 0; c < b_width; ++c) {

            if (r == 0 && c == 0)
                putChar(b_row + r, b_col + c, "┌");

            else if (r == 0 && c == b_width - 1)
                putChar(b_row + r, b_col + c, "┐");

            else if (r == b_height - 1 && c == 0)
                putChar(b_row + r, b_col + c, "└");
            
            else if (r == b_height - 1 && c == b_width - 1)
                putChar(b_row + r, b_col + c, "┘");
            
            else if (r == 0 || r == b_height - 1)
                putChar(b_row + r, b_col + c, "─");

            else if (c == 0 || c == b_width - 1)
                putChar(b_row + r, b_col + c, "│");

            else
                putChar(b_row + r, b_col + c, " ");
        }
    }
}


// --------------------------------------------
// DRAW SQUARE
// --------------------------------------------

void Tui::Component::Square::drawSquare(Screen& s) {
    size_t m_cols = s.getCols();
    size_t m_rows = s.getRows();

    if (s_row == 99999 && s_col == 99999) {
        s_row = (m_rows - s_side) / 2;
        s_col = (m_cols - s_side * 2) / 2;
    }

    auto& buff = s.getBackBuffer();

    auto putChar = [&](size_t r, size_t c, const std::string& ch) {
        size_t idx = r * m_cols + c;
        buff.at(idx).cellItem = ch;
        buff.at(idx).fg = this->s_fg;
        buff.at(idx).bg = this->s_bg;
        buff.at(idx).style = Style::None;
    };
    size_t s_height = s_side;
    size_t s_width = s_side * 2;
    for (size_t r = 0; r < s_height; ++r) {
        for (size_t c = 0; c < s_width; ++c) {

            if (r == 0 && c == 0)
                putChar(s_row + r, s_col + c, "┌");

            else if (r == 0 && c == s_width - 1)
                putChar(s_row + r, s_col + c, "┐");

            else if (r == s_height - 1 && c == 0)
                putChar(s_row + r, s_col + c, "└");
            
            else if (r == s_height - 1 && c == s_width - 1)
                putChar(s_row + r, s_col + c, "┘");
            
            else if (r == 0 || r == s_height - 1)
                putChar(s_row + r, s_col + c, "─");

            else if (c == 0 || c == s_width - 1)
                putChar(s_row + r, s_col + c, "│");

            else
                putChar(s_row + r, s_col + c, " ");
        }
    }
}
