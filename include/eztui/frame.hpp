#pragma once

#include "eztui/cell.hpp"
#include "eztui/layout.hpp"
#include "eztui/terminal.hpp"
#include <cstdint>
#include <termios.h>
#include <vector>

namespace eztui {
    class Frame {

        private:
            Terminal &term;
            uint32_t m_cols , m_rows;
            std::vector<Cell> m_front_buffer;
            std::vector<Cell> m_back_buffer;
            void move_cursor(uint32_t row, uint32_t col);
            void paint_cell(uint32_t row, uint32_t col, Cell c);

        public:

            void set_cell(uint32_t row, uint32_t col, Cell c);
            void clear_back_buffer();
            void render();
            Rect area();
            Frame(Terminal& t);
            ~Frame() = default;
    };
}