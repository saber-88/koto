#include "koto/gauge.hpp"
#include "koto/frame.hpp"
#include "koto/utf8.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace koto
{
    static constexpr std::array<char32_t, 9> FRACTIONAL_BLOCKS = {
        U' ', U'▏', U'▎', U'▍', U'▌', U'▋', U'▊', U'▉', U'█'
    };

    Gauge::Gauge() = default;

    Gauge& Gauge::ratio(float r)
    {
        m_ratio = std::clamp(r, 0.0f, 1.0f);
        return *this;
    }

    Gauge& Gauge::percent(int p)
    {
        return ratio(static_cast<float>(p) / 100.0f);
    }

    Gauge& Gauge::label(std::string_view label)
    {
        m_label = std::string(label);
        return *this;
    }

    Gauge& Gauge::filled_char(char32_t ch)
    {
        m_filled_ch = ch;
        m_fractional = false;
        return *this;
    }

    Gauge& Gauge::empty_char(char32_t ch)
    {
        m_empty_ch = ch;
        return *this;
    }

    Gauge& Gauge::use_fractional_blocks(bool enable)
    {
        m_fractional = enable;
        return *this;
    }

    Gauge& Gauge::fg(std::string_view fg)
    {
        return filled_fg(fg);
    }

    Gauge& Gauge::fg(uint32_t fg)
    {
        return filled_fg(fg);
    }

    Gauge& Gauge::bg(std::string_view bg)
    {
        return empty_bg(bg);
    }

    Gauge& Gauge::bg(uint32_t bg)
    {
        return empty_bg(bg);
    }

    Gauge& Gauge::filled_fg(std::string_view fg)
    {
        m_filled_fg = parse_color(fg);
        return *this;
    }

    Gauge& Gauge::filled_fg(uint32_t fg)
    {
        m_filled_fg = fg;
        return *this;
    }

    Gauge& Gauge::filled_bg(std::string_view bg)
    {
        m_filled_bg = parse_color(bg);
        return *this;
    }

    Gauge& Gauge::filled_bg(uint32_t bg)
    {
        m_filled_bg = bg;
        return *this;
    }

    Gauge& Gauge::empty_fg(std::string_view fg)
    {
        m_empty_fg = parse_color(fg);
        return *this;
    }

    Gauge& Gauge::empty_fg(uint32_t fg)
    {
        m_empty_fg = fg;
        return *this;
    }

    Gauge& Gauge::empty_bg(std::string_view bg)
    {
        m_empty_bg = parse_color(bg);
        return *this;
    }

    Gauge& Gauge::empty_bg(uint32_t bg)
    {
        m_empty_bg = bg;
        return *this;
    }

    Gauge& Gauge::label_fg(std::string_view fg)
    {
        m_label_fg = parse_color(fg);
        return *this;
    }

    Gauge& Gauge::label_fg(uint32_t fg)
    {
        m_label_fg = fg;
        return *this;
    }

    void Gauge::draw(Frame& frame, Area area)
    {
        if (!area.is_valid() || area.width <= 0 || area.height <= 0)
        {
            return;
        }

        int total_subcells = area.width * 8;
        int filled_subcells = static_cast<int>(std::lround(m_ratio * static_cast<float>(total_subcells)));
        filled_subcells = std::clamp(filled_subcells, 0, total_subcells);

        int full_cells = filled_subcells / 8;
        int remainder = filled_subcells % 8;

        for (int r = area.row; r < area.bottom(); ++r)
        {
            for (int col_idx = 0; col_idx < area.width; ++col_idx)
            {
                int c = area.col + col_idx;

                if (col_idx < full_cells)
                {
                    frame.set_cell(r, c, {m_filled_ch, m_filled_fg, m_filled_bg});
                }
                else if (col_idx == full_cells && remainder > 0 && m_fractional)
                {
                    char32_t frac_ch = FRACTIONAL_BLOCKS[static_cast<size_t>(remainder)];
                    frame.set_cell(r, c, {frac_ch, m_filled_fg, m_empty_bg});
                }
                else
                {
                    frame.set_cell(r, c, {m_empty_ch, m_empty_fg, m_empty_bg});
                }
            }
        }

        // Draw centered label if provided
        if (!m_label.empty())
        {
            int label_w = text_width(m_label);

            if (label_w <= area.width)
            {
                int label_start_col = area.col + (area.width - label_w) / 2;
                int label_row = area.row + (area.height - 1) / 2;

                std::string_view remaining = m_label;
                int c = label_start_col;
                while (!remaining.empty() && c < area.right())
                {
                    char32_t cp = next_codepoint(remaining);
                    int w = codepoint_width(cp);

                    // Preserve existing background cell color under the label
                    Cell existing = frame.get_cell(label_row, c);
                    frame.set_cell(label_row, c, {cp, m_label_fg, existing.bg});
                    c += w;
                }
            }
        }
    }
}
