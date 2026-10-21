#pragma once

#include "koto/cell.hpp"
#include "koto/id.hpp"
#include "koto/layout.hpp"
#include "koto/terminal.hpp"
#include <string>
#include <vector>

namespace koto
{
    class Frame
    {

        public:
        
            Frame(Terminal& t);
            Frame(int rows, int cols);
            ~Frame() = default;

            auto handle_resize(int rows, int cols) -> void;
            auto set_cell(int row, int col, Cell c, Id tag_id = Id::none()) -> void;
            auto get_cell(int row, int col) const -> Cell;
            auto operator[](int row, int col) -> Cell&;
            auto operator[](int row, int col) const -> const Cell&;
            auto fill(Area area, Cell c, Id tag_id = Id::none()) -> void;
            auto fill(Area area, char32_t ch, uint32_t fg = COLOR_DEFAULT, uint32_t bg = COLOR_DEFAULT, Id tag_id = Id::none()) -> void;
            auto clear_back_buffer() -> void;
            auto render() -> void;
            auto set_cursor(int row, int col) -> void;
            auto area() const -> Area;

            // Spatial Tag / Hit-Buffer API
            auto tag_cell(int row, int col, Id tag_id) -> void;
            auto tag_area(Area area, Id tag_id) -> void;
            auto hit_at(int row, int col) const -> Id;
            auto clear_hit_buffer(Id default_tag = Id::none()) -> void;

            // Snapshot testing & inspection
            auto to_plain_text() const -> std::string;
            auto to_plain_text_row(int r) const -> std::string;
            auto cursor_row() const -> int { return m_cursor_row; }
            auto cursor_col() const -> int { return m_cursor_col; }
            auto cursor_visible() const -> bool { return m_cursor_visible; }
            
        private:

            Terminal* m_term = nullptr;
            int m_rows , m_cols;
            std::vector<Cell> m_front_buffer;
            std::vector<Cell> m_back_buffer;
            std::vector<int> m_hit_buffer;
            std::string m_output;              // output buffer for batched write
            uint32_t m_cur_fg = COLOR_DEFAULT;  // tracked SGR fg
            uint32_t m_cur_bg = COLOR_DEFAULT;  // tracked SGR bg
            bool m_cursor_visible = false;
            int m_cursor_row = -1;
            int m_cursor_col = -1;
            auto move_cursor(int row, int col) -> void;
            auto paint_cell(Cell c) -> void;
    };
}