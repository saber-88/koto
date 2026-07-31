#pragma once

#include "koto/cell.hpp"
#include "koto/layout.hpp"
#include "koto/terminal.hpp"
#include <cstdint>
#include <vector>

namespace koto
{
    class Frame
    {

        private:

            Terminal& m_term;
            uint32_t m_cols , m_rows;
            std::vector<Cell> m_front_buffer;
            std::vector<Cell> m_back_buffer;
            auto move_cursor(uint32_t row, uint32_t col) const -> void;
            auto paint_cell(uint32_t row, uint32_t col, Cell c) -> void;

        public:

            auto handle_resize(uint32_t rows, uint32_t cols) -> void;
            auto set_cell(uint32_t row, uint32_t col, Cell c) -> void;
            auto clear_back_buffer() -> void;
            auto render() -> void;
            auto area() const -> Area;
            Frame(Terminal& t);
            ~Frame() = default;
    };
}