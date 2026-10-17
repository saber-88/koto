#include "koto/color.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace koto
{
    auto parse_color(std::string_view color) -> uint32_t
    {   
        if (color.empty())
        {
            return COLOR_DEFAULT;
        }

        if (color[0] == '#')
        {
            color.remove_prefix(1);
        }

        char buf[8] = {};
        size_t n = std::min(sizeof(buf) - 1,color.size());
        color.copy(buf,n);
        buf[n] = '\0';
        return static_cast<uint32_t>(std::strtoul(buf,nullptr,16));
    }

    auto Color::from_hex(std::string_view hex) -> uint32_t
    {
        return parse_color(hex);
    }

    auto Color::lerp(uint32_t c1, uint32_t c2, float t) noexcept -> uint32_t
    {
        if (c1 == COLOR_DEFAULT) return c2;
        if (c2 == COLOR_DEFAULT) return c1;
        if (t <= 0.0f) return c1;
        if (t >= 1.0f) return c2;

        float r1 = static_cast<float>((c1 >> 16) & 0xFF);
        float g1 = static_cast<float>((c1 >> 8)  & 0xFF);
        float b1 = static_cast<float>(c1 & 0xFF);

        float r2 = static_cast<float>((c2 >> 16) & 0xFF);
        float g2 = static_cast<float>((c2 >> 8)  & 0xFF);
        float b2 = static_cast<float>(c2 & 0xFF);

        float rf = std::clamp(r1 + (r2 - r1) * t, 0.0f, 255.0f);
        float gf = std::clamp(g1 + (g2 - g1) * t, 0.0f, 255.0f);
        float bf = std::clamp(b1 + (b2 - b1) * t, 0.0f, 255.0f);

        auto r = static_cast<uint8_t>(std::lround(rf));
        auto g = static_cast<uint8_t>(std::lround(gf));
        auto b = static_cast<uint8_t>(std::lround(bf));

        return rgb(r, g, b);
    }

    auto Color::lerp(std::string_view hex1, std::string_view hex2, float t) -> uint32_t
    {
        return lerp(parse_color(hex1), parse_color(hex2), t);
    }
}