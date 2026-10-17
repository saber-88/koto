#include "koto/sparkline.hpp"
#include "koto/frame.hpp"
#include <algorithm>
#include <array>

namespace koto
{
    static constexpr std::array<char32_t, 8> SPARKLINE_BARS = {
        U' ', U'▂', U'▃', U'▄', U'▅', U'▆', U'▇', U'█'
    };

    Sparkline::Sparkline() = default;

    Sparkline& Sparkline::data(std::span<const uint64_t> values)
    {
        m_data = values;
        m_owned_data.clear();
        return *this;
    }

    Sparkline& Sparkline::data(const std::vector<uint64_t>& values)
    {
        m_owned_data = values;
        m_data = std::span<const uint64_t>(m_owned_data);
        return *this;
    }

    Sparkline& Sparkline::max(uint64_t max_value)
    {
        m_max = max_value;
        return *this;
    }

    Sparkline& Sparkline::align_right(bool right)
    {
        m_align_right = right;
        return *this;
    }

    Sparkline& Sparkline::fg(std::string_view fg)
    {
        m_fg = parse_color(fg);
        return *this;
    }

    Sparkline& Sparkline::fg(uint32_t fg)
    {
        m_fg = fg;
        return *this;
    }

    Sparkline& Sparkline::bg(std::string_view bg)
    {
        m_bg = parse_color(bg);
        return *this;
    }

    Sparkline& Sparkline::bg(uint32_t bg)
    {
        m_bg = bg;
        return *this;
    }

    void Sparkline::draw(Frame& frame, Area area)
    {
        if (!area.is_valid() || area.width <= 0 || area.height <= 0)
        {
            return;
        }

        // Draw sparkline at bottom row of area
        int target_row = area.bottom() - 1;

        // Clear any rows above target_row
        for (int r = area.row; r < target_row; ++r)
        {
            for (int c = area.col; c < area.right(); ++c)
            {
                frame.set_cell(r, c, {U' ', m_fg, m_bg});
            }
        }

        if (m_data.empty())
        {
            for (int c = area.col; c < area.right(); ++c)
            {
                frame.set_cell(target_row, c, {U' ', m_fg, m_bg});
            }
            return;
        }

        uint64_t effective_max = m_max;
        if (effective_max == 0)
        {
            for (uint64_t val : m_data)
            {
                if (val > effective_max) effective_max = val;
            }
            if (effective_max == 0) effective_max = 1;
        }

        auto num_points = static_cast<int>(m_data.size());
        int available_cols = area.width;

        // If data exceeds width, take the latest `available_cols` items
        int start_idx = 0;
        int points_to_show = num_points;
        if (num_points > available_cols)
        {
            start_idx = num_points - available_cols;
            points_to_show = available_cols;
        }

        int blank_prefix = 0;
        if (m_align_right && points_to_show < available_cols)
        {
            blank_prefix = available_cols - points_to_show;
        }

        for (int col_idx = 0; col_idx < available_cols; ++col_idx)
        {
            int c = area.col + col_idx;

            if (col_idx < blank_prefix || col_idx >= blank_prefix + points_to_show)
            {
                frame.set_cell(target_row, c, {U' ', m_fg, m_bg});
            }
            else
            {
                int data_idx = start_idx + (col_idx - blank_prefix);
                uint64_t val = m_data[static_cast<size_t>(data_idx)];

                char32_t bar_ch = U' ';
                if (val > 0)
                {
                    uint64_t bar_level = (val * 8) / effective_max;
                    if (bar_level >= 8)
                    {
                        bar_level = 7;
                    }
                    bar_ch = SPARKLINE_BARS[static_cast<size_t>(bar_level)];
                }

                frame.set_cell(target_row, c, {bar_ch, m_fg, m_bg});
            }
        }
    }
}
