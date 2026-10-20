#include "koto/color.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <string_view>

namespace koto
{
    auto try_parse_color(std::string_view color) -> std::expected<uint32_t, ColorParseError>
    {
        if (color.empty())
        {
            return std::unexpected(ColorParseError::EmptyString);
        }

        if (color[0] == '#')
        {
            color.remove_prefix(1);
        }

        if (color.size() != 6 && color.size() != 3 && color.size() != 8)
        {
            return std::unexpected(ColorParseError::InvalidLength);
        }

        for (char c : color)
        {
            bool is_hex = (c >= '0' && c <= '9') ||
                          (c >= 'a' && c <= 'f') ||
                          (c >= 'A' && c <= 'F');
            if (!is_hex)
            {
                return std::unexpected(ColorParseError::InvalidHexDigit);
            }
        }

        // 3-digit shorthand hex: #RGB -> #RRGGBB
        if (color.size() == 3)
        {
            auto hex_val = [](char c) -> uint32_t {
                if (c >= '0' && c <= '9') return static_cast<uint32_t>(c - '0');
                if (c >= 'a' && c <= 'f') return static_cast<uint32_t>(c - 'a' + 10);
                return static_cast<uint32_t>(c - 'A' + 10);
            };
            uint32_t r = hex_val(color[0]);
            uint32_t g = hex_val(color[1]);
            uint32_t b = hex_val(color[2]);
            return Color::rgb(static_cast<uint8_t>(r * 17), static_cast<uint8_t>(g * 17), static_cast<uint8_t>(b * 17));
        }

        char buf[9] = {};
        size_t n = std::min(sizeof(buf) - 1, color.size());
        color.copy(buf, n);
        buf[n] = '\0';
        return static_cast<uint32_t>(std::strtoul(buf, nullptr, 16));
    }

    auto parse_color(std::string_view color) -> uint32_t
    {   
        return try_parse_color(color).value_or(COLOR_DEFAULT);
    }

    auto Color::try_from_hex(std::string_view hex) -> std::expected<uint32_t, ColorParseError>
    {
        return try_parse_color(hex);
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