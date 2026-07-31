#include "koto/frame.hpp"
#include "koto/cell.hpp"
#include "koto/layout.hpp"
#include "koto/terminal.hpp"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <unistd.h>
#include <vector>

namespace koto
{

    Frame::Frame(Terminal &t):
    m_term(t), 
    m_cols(m_term.get_cols()), 
    m_rows(m_term.get_rows()),
    m_front_buffer(m_cols * m_rows, Cell{}),
    m_back_buffer(m_cols * m_rows, Cell{})
    {}
    
    auto Frame::set_cell(uint32_t row, uint32_t col, Cell c) -> void
    {

        if (row >= m_rows || col >= m_cols) return;
        m_back_buffer[m_cols * row + col] = c;

    }

    auto Frame::clear_back_buffer() -> void
    {

        std::fill(m_back_buffer.begin(), m_back_buffer.end(), Cell{});

    }

    auto Frame::move_cursor(uint32_t row, uint32_t col) const -> void
    {

        char buf[32];
        int n = snprintf(buf, sizeof(buf), "\033[%u;%uH", row + 1, col + 1);
        write(STDOUT_FILENO, buf, n);

    }

    auto Frame::paint_cell(uint32_t row, uint32_t col, Cell c) -> void
    {

        char buf[256] = {};
        int  offset  = 0;

        if (c.fg != COLOR_DEFAULT)
        {

            uint8_t r = (c.fg >> 16) & 0xFF;
            uint8_t g = (c.fg >> 8)  & 0xFF;
            uint8_t b = (c.fg)       & 0xFF;
            offset += snprintf(buf + offset, sizeof(buf) - offset,
                            "\033[38;2;%u;%u;%um", r, g, b);
        }
        else
        {
            // reset foreground color
            offset += snprintf(buf + offset, sizeof(buf) - offset,
                            "\033[39m");    
        }

        if (c.bg != COLOR_DEFAULT)
        {

            uint8_t r = (c.bg >> 16) & 0xFF;
            uint8_t g = (c.bg >> 8)  & 0xFF;
            uint8_t b = (c.bg)       & 0xFF;
            offset += snprintf(buf + offset, sizeof(buf) - offset,
                            "\033[48;2;%u;%u;%um", r, g, b);
        }
        else
        {
            // reset background color
            offset += snprintf(buf + offset, sizeof(buf) - offset,
                            "\033[49m");    
        }
        char utf8[5] = {};
        uint32_t cp = c.ch;

        if (cp < 0x80) {
            utf8[0] = cp;
        } else if (cp < 0x800) {
            utf8[0] = 0xC0 | (cp >> 6);
            utf8[1] = 0x80 | (cp & 0x3F);
        } else if (cp < 0x10000) {
            utf8[0] = 0xE0 | (cp >> 12);
            utf8[1] = 0x80 | ((cp >> 6) & 0x3F);
            utf8[2] = 0x80 | (cp & 0x3F);
        } else {
            utf8[0] = 0xF0 | (cp >> 18);
            utf8[1] = 0x80 | ((cp >> 12) & 0x3F);
            utf8[2] = 0x80 | ((cp >> 6) & 0x3F);
            utf8[3] = 0x80 | (cp & 0x3F);
        }


        offset += snprintf(buf + offset, sizeof(buf) - offset,
    "%s\033[0m", utf8);

        move_cursor(row, col);
        write(STDOUT_FILENO, buf, offset);
    }

    auto Frame::render() -> void
    {

        for(uint32_t r = 0; r < m_rows; r++)
        {
            for(uint32_t c = 0; c < m_cols; c++)
            {
                uint32_t idx = m_cols * r + c;
                if (!(m_front_buffer[idx] == m_back_buffer[idx]))
                {
                    paint_cell(r, c, m_back_buffer[idx]);
                    m_front_buffer[idx] = m_back_buffer[idx];
                }
            }
        }

        fflush(stdout);
    }

    auto Frame::area() const -> Area
    {
        return {0,0,m_cols,m_rows};
    }

    auto Frame::handle_resize(uint32_t rows, uint32_t cols) -> void
    {
        if (rows < 3 || cols < 8)
        {
            return;
        }
        m_rows = rows;
        m_cols = cols;
        Cell def;
        def.ch = U'\xFF';
        m_front_buffer.assign(rows * cols, def);
        m_back_buffer.assign(rows * cols, def);
    }
}
