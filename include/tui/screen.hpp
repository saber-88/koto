#pragma once

#include "cell.hpp"
#include <cstddef>
#include <termios.h>
#include <vector>

namespace Tui {
    class Screen {
        
        public:
        
        Screen();
        ~Screen();
        
        void init(bool altBuffer);
        void exit();
        void clear();                                                   
        size_t getRows();
        size_t getCols();
        void updateSize();
        void checkResize();

        std::vector<Cell>& getBackBuffer();

        void hideCursor();
        void showCursor();
        void clearScreen();
        void render(size_t time);
        void render();
        
        private:
        
        static size_t m_rows;
        static size_t m_cols;
        termios og_tio ,raw_tio;
        bool m_altBuffer , m_initScreen;
        std::vector<Cell> frontBuffer;
        std::vector<Cell> backBuffer;
        
    };
    
}
