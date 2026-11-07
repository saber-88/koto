#pragma once

#include <cstdint>
#include <expected>
#include <string_view>

namespace koto
{

    inline constexpr uint32_t COLOR_DEFAULT = 0xFF000000;

    enum class ColorMode : uint8_t
    {
        NoColor = 0,
        Ansi16 = 1,
        Ansi256 = 2,
        TrueColor = 3
    };

    auto rgb_to_ansi256(uint8_t r, uint8_t g, uint8_t b) noexcept -> uint8_t;
    auto rgb_to_ansi16(uint8_t r, uint8_t g, uint8_t b) noexcept -> uint8_t;
    auto detect_system_color_mode() noexcept -> ColorMode;

    enum class ColorParseError
    {
        EmptyString,
        InvalidLength,
        InvalidHexDigit,
    };

    auto try_parse_color(std::string_view color) -> std::expected<uint32_t, ColorParseError>;
    auto parse_color(std::string_view color) -> uint32_t;

    struct Color
    {
        uint32_t value = COLOR_DEFAULT;

        constexpr Color() noexcept = default;
        constexpr Color(uint32_t val) noexcept : value(val) {}
        Color(std::string_view name_or_hex);
        Color(const char* name_or_hex);

        constexpr operator uint32_t() const noexcept { return value; }
        constexpr bool operator==(const Color& other) const noexcept = default;
        constexpr bool is_default() const noexcept { return value == COLOR_DEFAULT; }

        static constexpr uint32_t Default   = COLOR_DEFAULT;
        static constexpr uint32_t Reset     = COLOR_DEFAULT;
        static constexpr uint32_t Black     = 0x00000000;
        static constexpr uint32_t White     = 0x00FFFFFF;
        static constexpr uint32_t Red       = 0x00FF0000;
        static constexpr uint32_t Green     = 0x0000FF00;
        static constexpr uint32_t Blue      = 0x000000FF;
        static constexpr uint32_t Yellow    = 0x00FFFF00;
        static constexpr uint32_t Cyan      = 0x0000FFFF;
        static constexpr uint32_t Magenta   = 0x00FF00FF;
        static constexpr uint32_t Gray      = 0x00808080;
        static constexpr uint32_t DarkGray  = 0x00404040;
        static constexpr uint32_t LightGray = 0x00C0C0C0;
        static constexpr uint32_t Orange    = 0x00FFA500;
        static constexpr uint32_t Emerald   = 0x0050C878;
        static constexpr uint32_t Teal      = 0x00008080;
        static constexpr uint32_t Amber     = 0x00FFBF00;

        static constexpr auto rgb(uint8_t r, uint8_t g, uint8_t b) noexcept -> uint32_t
        {
            return (static_cast<uint32_t>(r) << 16) |
                   (static_cast<uint32_t>(g) << 8)  |
                   static_cast<uint32_t>(b);
        }

        static constexpr auto Rgb(uint8_t r, uint8_t g, uint8_t b) noexcept -> uint32_t
        {
            return rgb(r, g, b);
        }

        static auto try_from_hex(std::string_view hex) -> std::expected<uint32_t, ColorParseError>;
        static auto from_hex(std::string_view hex) -> uint32_t;
        static auto lerp(uint32_t c1, uint32_t c2, float t) noexcept -> uint32_t;
        static auto lerp(std::string_view hex1, std::string_view hex2, float t) -> uint32_t;
    };

}