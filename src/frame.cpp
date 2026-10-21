#include "koto/frame.hpp"
#include "koto/cell.hpp"
#include "koto/layout.hpp"
#include "koto/terminal.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

namespace koto
{

    Frame::Frame(Terminal &t):
    m_term(&t), 
    m_rows(m_term->get_rows()),
    m_cols(m_term->get_cols()), 
    m_front_buffer(static_cast<size_t>(m_cols * m_rows), Cell{}),
    m_back_buffer(static_cast<size_t>(m_cols * m_rows), Cell{}),
    m_hit_buffer(static_cast<size_t>(m_cols * m_rows), TAG_NONE)
    {}

    Frame::Frame(int rows, int cols):
    m_term(nullptr),
    m_rows(rows < 0 ? 0 : rows),
    m_cols(cols < 0 ? 0 : cols),
    m_front_buffer(static_cast<size_t>(m_cols * m_rows), Cell{}),
    m_back_buffer(static_cast<size_t>(m_cols * m_rows), Cell{}),
    m_hit_buffer(static_cast<size_t>(m_cols * m_rows), TAG_NONE)
    {}

    auto Frame::get_cell(int row, int col) const -> Cell
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        {
            return Cell{};
        }
        return m_back_buffer[static_cast<size_t>(row * m_cols + col)];
    }

    auto Frame::operator[](int row, int col) -> Cell&
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        {
            static thread_local Cell s_dummy{};
            s_dummy = Cell{};
            return s_dummy;
        }
        return m_back_buffer[static_cast<size_t>(row * m_cols + col)];
    }

    auto Frame::operator[](int row, int col) const -> const Cell&
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_cols)
        {
            static const Cell s_empty{};
            return s_empty;
        }
        return m_back_buffer[static_cast<size_t>(row * m_cols + col)];
    }
    
    auto Frame::set_cell(int row, int col, Cell c, int tag_id) -> void
    {
        if (row >= m_rows || col >= m_cols || row < 0 || col < 0) return;

        size_t idx = static_cast<size_t>(m_cols * row + col);


        if (col > 0 && m_back_buffer[idx].continuation)
        {
            size_t owner = idx - 1;
            m_back_buffer[owner].ch = U' ';
            m_back_buffer[owner].width = 1;
        }

        if (m_back_buffer[idx].width == 2 && col + 1 < m_cols)
        {
            m_back_buffer[idx + 1] = Cell{};
        }

        int w = codepoint_width(c.ch);
        if (w == 2 && col + 1 >= m_cols) { c.ch = U' '; w = 1; }

        c.width = w;
        if (w == 2)
        {
            m_back_buffer[idx + 1] = Cell{.continuation = true};
            if (tag_id != TAG_NONE)
            {
                m_hit_buffer[idx + 1] = tag_id;
            }
        }
        m_back_buffer[idx] = c;
        if (tag_id != TAG_NONE)
        {
            m_hit_buffer[idx] = tag_id;
        }
    }

    auto Frame::fill(Area area, Cell c, int tag_id) -> void
    {
        if (!area.is_valid()) return;
        for (int r = area.row; r < area.bottom(); ++r)
        {
            for (int col = area.col; col < area.right(); ++col)
            {
                this->set_cell(r, col, c, tag_id);
            }
        }
    }

    auto Frame::fill(Area area, char32_t ch, uint32_t fg, uint32_t bg, int tag_id) -> void
    {
        this->fill(area, Cell{ch, fg, bg, 1, false}, tag_id);
    }

    auto Frame::set_cursor(int row, int col) -> void
    {
        m_cursor_visible = true;
        m_cursor_row = row;
        m_cursor_col = col;
    }

    auto Frame::clear_back_buffer() -> void
    {

        std::fill(m_back_buffer.begin(), m_back_buffer.end(), Cell{});
        std::fill(m_hit_buffer.begin(), m_hit_buffer.end(), TAG_NONE);
        m_cursor_visible = false;
        m_cursor_row = -1;
        m_cursor_col = -1;

    }

    auto Frame::move_cursor(int row, int col) -> void
    {

        char buf[32];
        int n = snprintf(buf, sizeof(buf), "\033[%d;%dH", row + 1, col + 1);
        m_output.append(buf, static_cast<size_t>(n));

    }

    auto Frame::paint_cell(Cell c) -> void
    {

        char buf[64] = {};
        int  offset  = 0;

        // only emit fg SGR when color actually changes
        if (c.fg != m_cur_fg)
        {
            if (c.fg != COLOR_DEFAULT)
            {
                uint8_t r = (c.fg >> 16) & 0xFF;
                uint8_t g = (c.fg >> 8)  & 0xFF;
                uint8_t b = (c.fg)       & 0xFF;
                offset += snprintf(buf + offset, sizeof(buf) - static_cast<size_t>(offset),
                                "\033[38;2;%u;%u;%um", r, g, b);
            }
            else
            {
                offset += snprintf(buf + offset, sizeof(buf) - static_cast<size_t>(offset),
                                "\033[39m");    
            }
            m_cur_fg = c.fg;
        }

        // only emit bg SGR when color actually changes
        if (c.bg != m_cur_bg)
        {
            if (c.bg != COLOR_DEFAULT)
            {
                uint8_t r = (c.bg >> 16) & 0xFF;
                uint8_t g = (c.bg >> 8)  & 0xFF;
                uint8_t b = (c.bg)       & 0xFF;
                offset += snprintf(buf + offset, sizeof(buf) - static_cast<size_t>(offset),
                                "\033[48;2;%u;%u;%um", r, g, b);
            }
            else
            {
                offset += snprintf(buf + offset, sizeof(buf) - static_cast<size_t>(offset),
                                "\033[49m");    
            }
            m_cur_bg = c.bg;
        }

        m_output.append(buf, static_cast<size_t>(offset));

        auto encoded = encode_utf8(c.ch);
        m_output.append(encoded.view());
    }

    auto Frame::render() -> void
    {
        m_output.clear();
        // reserve a reasonable amount to avoid repeated realloc
        m_output.reserve(static_cast<size_t>(m_rows * m_cols * 4));

        // DEC Mode 2026: Begin Synchronized Update (halts terminal rasterization for tear-free atomic render)
        m_output.append("\033[?2026h");

        // sentinel that can't match any real cell color,
        // forces the first dirty cell to always emit SGR
        m_cur_fg = ~uint32_t(0);
        m_cur_bg = ~uint32_t(0);
        int cursor_row = -1;
        int cursor_col = -1;

        for(int r = 0; r < m_rows; r++)
        {
            for(int c = 0; c < m_cols; c++)
            {
                size_t idx = static_cast<size_t>(m_cols * r + c);
                Cell& front = m_front_buffer[idx];
                Cell& back = m_back_buffer[idx];

                if (back.continuation)
                {
                    continue;
                }
                if (front == back)
                {
                    continue;
                }
                
                // skip cursor move if we're already at the right position
                if (r != cursor_row || c != cursor_col)
                {
                    this->move_cursor(r, c);
                }
                this->paint_cell(back);
                cursor_col = c + back.width;
                cursor_row = r;

                front = back;
                if (back.width == 2 && idx + 1 < m_front_buffer.size())
                {
                    m_front_buffer[idx + 1] = m_back_buffer[idx + 1];
                }
                
            }
        }

        // position hardware cursor if requested, otherwise keep hidden
        if (m_cursor_visible && m_cursor_row >= 0 && m_cursor_row < m_rows && m_cursor_col >= 0 && m_cursor_col < m_cols)
        {
            this->move_cursor(m_cursor_row, m_cursor_col);
            m_output.append("\033[?25h");
        }
        else
        {
            m_output.append("\033[?25l");
        }

        // DEC Mode 2026: End Synchronized Update (atomic render flush)
        m_output.append("\033[?2026l");

        // flush entire frame in one write if attached to a terminal
        if (m_term != nullptr && !m_output.empty())
        {
            write(STDOUT_FILENO, m_output.data(), m_output.size());
            fflush(stdout);
        }
    }

    auto Frame::area() const -> Area
    {
        return {0,0,m_cols,m_rows};
    }

    auto Frame::handle_resize(int rows, int cols) -> void
    {
        m_rows = (rows < 0) ? 0 : rows ;
        m_cols = (cols < 0) ? 0 : cols;
        size_t total = static_cast<size_t>(m_rows * m_cols);
        Cell def;
        def.ch = U'\xFF';
        def.continuation = false;
        m_front_buffer.assign(total, def);
        m_back_buffer.assign(total, def);
        m_hit_buffer.assign(total, TAG_NONE);
    }

    auto Frame::tag_cell(int row, int col, int tag_id) -> void
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) return;
        m_hit_buffer[static_cast<size_t>(row * m_cols + col)] = tag_id;
    }

    auto Frame::tag_area(Area area, int tag_id) -> void
    {
        if (!area.is_valid()) return;
        int r_end = std::min(area.bottom(), m_rows);
        int c_end = std::min(area.right(), m_cols);
        for (int r = std::max(0, area.row); r < r_end; ++r)
        {
            for (int c = std::max(0, area.col); c < c_end; ++c)
            {
                m_hit_buffer[static_cast<size_t>(r * m_cols + c)] = tag_id;
            }
        }
    }

    auto Frame::hit_at(int row, int col) const -> int
    {
        if (row < 0 || row >= m_rows || col < 0 || col >= m_cols) return TAG_NONE;
        return m_hit_buffer[static_cast<size_t>(row * m_cols + col)];
    }

    auto Frame::clear_hit_buffer(int default_tag) -> void
    {
        std::fill(m_hit_buffer.begin(), m_hit_buffer.end(), default_tag);
    }

    auto Frame::to_plain_text_row(int r) const -> std::string
    {
        if (r < 0 || r >= m_rows) return "";
        std::string row_str;
        row_str.reserve(static_cast<size_t>(m_cols * 4));

        for (int c = 0; c < m_cols; c++)
        {
            const Cell& cell = m_back_buffer[static_cast<size_t>(r * m_cols + c)];
            if (cell.continuation) continue;

            char utf8[5] = {};
            int len = encode_utf8(cell.ch == 0 ? U' ' : cell.ch, utf8);
            row_str.append(utf8, static_cast<size_t>(len));
        }

        // Trim trailing whitespace for clean snapshot comparison
        while (!row_str.empty() && row_str.back() == ' ')
        {
            row_str.pop_back();
        }

        return row_str;
    }

    auto Frame::to_plain_text() const -> std::string
    {
        std::string result;
        for (int r = 0; r < m_rows; r++)
        {
            result += to_plain_text_row(r);
            if (r + 1 < m_rows)
            {
                result += '\n';
            }
        }
        return result;
    }
}
