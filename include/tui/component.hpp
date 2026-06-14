#pragma once

#include "screen.hpp"
#include "colors.hpp"

#include <cstddef>
#include <string>

namespace Tui {
        class Component {
        
        public:
        
        Component();
        
        class Text{
            
            public:
            Text() = delete;
            Text(size_t row , size_t col , const std::string& text , FG f , BG b , Style s);
            Text(const std::string& text, FG f , BG b , Style s);
            void drawText( Screen& s);
            
            private:
            size_t t_row , t_col;
            FG t_fg;
            BG t_bg;
            Style t_style;
            std::string t_text;
        };
        
        class Box{
            
            public:
            
            Box(size_t row , size_t col , size_t height , size_t width, FG f , BG b );
            Box( size_t height , size_t width, FG f , BG b );
            
            void drawBox( Screen& s);
            
            private:
            size_t b_row , b_col , b_height , b_width;
            FG b_fg;
            BG b_bg;

        };
        class Square{
            
            public:
            
            Square(size_t row , size_t col , size_t side , FG f , BG b );
            Square( size_t side, FG f , BG b );
            
            void drawSquare( Screen& s);
            
            private:
            size_t s_row , s_col , s_side ;
            FG s_fg;
            BG s_bg;
        };
        
    };
}
