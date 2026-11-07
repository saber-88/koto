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
        uint32_t val = static_cast<uint32_t>(std::strtoul(buf, nullptr, 16));
        if (color.size() == 8)
        {
            // #RRGGBBAA: extract RGB components, ignoring alpha (never collides with COLOR_DEFAULT)
            uint8_t r = static_cast<uint8_t>((val >> 24) & 0xFF);
            uint8_t g = static_cast<uint8_t>((val >> 16) & 0xFF);
            uint8_t b = static_cast<uint8_t>((val >> 8) & 0xFF);
            return Color::rgb(r, g, b);
        }
        return val & 0x00FFFFFF;
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
        if (c1 == COLOR_DEFAULT && c2 == COLOR_DEFAULT) return COLOR_DEFAULT;
        if (c1 == COLOR_DEFAULT)
        {
            if (t <= 0.0f) return COLOR_DEFAULT;
            if (t >= 1.0f) return c2;
            return (t < 0.5f) ? COLOR_DEFAULT : c2;
        }
        if (c2 == COLOR_DEFAULT)
        {
            if (t <= 0.0f) return c1;
            if (t >= 1.0f) return COLOR_DEFAULT;
            return (t >= 0.5f) ? COLOR_DEFAULT : c1;
        }
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

    auto rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b) noexcept -> uint8_t
    {
        // 1. Check for grayscale ramp (24 shades: indices 232..255)
        int diff_rg = std::abs(static_cast<int>(r) - static_cast<int>(g));
        int diff_gb = std::abs(static_cast<int>(g) - static_cast<int>(b));
        int diff_rb = std::abs(static_cast<int>(r) - static_cast<int>(b));

        if (diff_rg <= 4 && diff_gb <= 4 && diff_rb <= 4)
        {
            int avg = (static_cast<int>(r) + static_cast<int>(g) + static_cast<int>(b)) / 3;
            if (avg < 4) return 16;     // pure black
            if (avg > 247) return 231;  // bright white
            int gray_idx = 232 + (avg - 8) / 10;
            return static_cast<uint8_t>(std::clamp(gray_idx, 232, 255));
        }

        // 2. 6x6x6 color cube (indices 16..231)
        auto quantize = [](uint8_t v) -> int {
            if (v < 48) return 0;
            if (v < 115) return 1;
            if (v < 155) return 2;
            if (v < 195) return 3;
            if (v < 235) return 4;
            return 5;
        };

        int qr = quantize(r);
        int qg = quantize(g);
        int qb = quantize(b);

        return static_cast<uint8_t>(16 + (36 * qr) + (6 * qg) + qb);
    }

    auto rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b) noexcept -> uint8_t
    {
        // Standard 16 ANSI color reference RGB values
        static constexpr uint8_t palette[16][3] = {
            {0,   0,   0},   // 0: Black
            {128, 0,   0},   // 1: Red
            {0,   128, 0},   // 2: Green
            {128, 128, 0},   // 3: Yellow
            {0,   0,   128}, // 4: Blue
            {128, 0,   128}, // 5: Magenta
            {0,   128, 128}, // 6: Cyan
            {192, 192, 192}, // 7: Light Gray / White
            {128, 128, 128}, // 8: Dark Gray / Bright Black
            {255, 0,   0},   // 9: Bright Red
            {0,   255, 0},   // 10: Bright Green
            {255, 255, 0},   // 11: Bright Yellow
            {0,   0,   255}, // 12: Bright Blue
            {255, 0,   255}, // 13: Bright Magenta
            {0,   255, 255}, // 14: Bright Cyan
            {255, 255, 255}  // 15: Bright White
        };

        uint8_t best_idx = 0;
        uint32_t min_dist = UINT32_MAX;
        for (uint8_t i = 0; i < 16; ++i)
        {
            int dr = static_cast<int>(r) - static_cast<int>(palette[i][0]);
            int dg = static_cast<int>(g) - static_cast<int>(palette[i][1]);
            int db = static_cast<int>(b) - static_cast<int>(palette[i][2]);
            uint32_t dist = static_cast<uint32_t>(dr * dr + dg * dg + db * db);
            if (dist < min_dist)
            {
                min_dist = dist;
                best_idx = i;
            }
        }
        return best_idx;
    }

    auto detect_system_color_mode() noexcept -> ColorMode
    {
        // 1. NO_COLOR standard (https://no-color.org)
        const char* no_color = std::getenv("NO_COLOR");
        if (no_color != nullptr && no_color[0] != '\0')
        {
            return ColorMode::NoColor;
        }

        const char* term = std::getenv("TERM");
        if (term != nullptr && std::string_view(term) == "dumb")
        {
            return ColorMode::NoColor;
        }

        // 2. COLORTERM: "truecolor" or "24bit"
        const char* colorterm = std::getenv("COLORTERM");
        if (colorterm != nullptr)
        {
            std::string_view ct(colorterm);
            if (ct == "truecolor" || ct == "24bit")
            {
                return ColorMode::TrueColor;
            }
        }

        // 3. TERM direct or truecolor detection
        if (term != nullptr)
        {
            std::string_view t(term);
            if (t.ends_with("-direct") || t.find("truecolor") != std::string_view::npos)
            {
                return ColorMode::TrueColor;
            }

            // 4. 256 colors detection
            if (t.find("256color") != std::string_view::npos || t.find("256") != std::string_view::npos)
            {
                return ColorMode::Ansi256;
            }

            // 5. Basic ANSI 16 terminal
            return ColorMode::Ansi16;
        }

        // Default to TrueColor on modern systems
        return ColorMode::TrueColor;
    }
}
