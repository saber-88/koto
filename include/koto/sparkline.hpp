#pragma once

#include "koto/widget.hpp"
#include "koto/color.hpp"
#include <span>
#include <vector>
#include <cstdint>
#include <string_view>

namespace koto
{
    class Sparkline : public Widget
    {
    public:
        Sparkline();

        Sparkline& data(std::span<const uint64_t> values);
        Sparkline& data(const std::vector<uint64_t>& values);
        Sparkline& max(uint64_t max_value);
        Sparkline& align_right(bool right = true);

        Sparkline& fg(std::string_view fg);
        Sparkline& fg(uint32_t fg);
        Sparkline& bg(std::string_view bg);
        Sparkline& bg(uint32_t bg);

        void draw(Frame& frame, Area area) override;

    private:
        std::span<const uint64_t> m_data;
        std::vector<uint64_t> m_owned_data;
        uint64_t m_max = 0;
        bool m_align_right = true;
        uint32_t m_fg = Color::Cyan;
        uint32_t m_bg = COLOR_DEFAULT;
    };
}
